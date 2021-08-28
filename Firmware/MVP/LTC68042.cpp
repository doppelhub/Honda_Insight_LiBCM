/*!
Copyright 2021(c) John Sullivan
Copyright 2018(c) Analog Devices, Inc.
Copyright 2013 Linear Technology Corp. (LTC)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the
   distribution.
 - Neither the name of Analog Devices, Inc. nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.
 - The use of this software may or may not infringe the patent rights
   of one or more patent holders.  This license does not release you
   from the requirement that you obtain separate licenses from these
   patent holders to use this software.
 - Use of the software either in source or binary form, must be run
   on or directly connected to an Analog Devices Inc. component.

THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "libcm.h"

//JTS2doLater: Split into multiple files:
//-LTC68042_configure.c
//-LTC68042_retrieve.c
//-LTC68042_process.c

//-LTC68042_celLVoltage.c
//-LTC68042_GPIO.c
//-LTC68042_lowLevel.c

uint8_t LTC_isDataValid=0;
uint8_t isoSPI_errorCount = 0;
uint32_t lastTimeDataSent_millis = 0; //LTC idle timer resets each time data is transferred

//conversion command variables.
uint8_t ADCV[2]; //Cell Voltage conversion command.
uint8_t ADAX[2]; //GPIO conversion command.

const uint8_t TOTAL_IC = 4; //number of ICs in the isoSPI network
const uint8_t FIRST_IC_ADDR = 2; //lowest address.  All additional IC addresses must be sequential

uint16_t maxEverCellVoltage; //since last key event
uint16_t minEverCellVoltage; //since last key event

//Stores returned cell voltages
//Note: cells & ICs are 1-indexed, whereas array is 0-indexed
//cell_codes[0][0]  is IC1 Cell 01
//cell_codes[3][11] is IC4 Cell 12
uint16_t cell_codes[TOTAL_IC][12];

//Stores returned aux voltage
//aux_codes[n][0] = GPIO1
//aux_codes[n][1] = GPIO2
//aux_codes[n][2] = GPIO3
//aux_codes[n][3] = GPIO4
//aux_codes[n][4] = GPIO5
//aux_codes[n][5] = Vref
uint16_t aux_codes[TOTAL_IC][6];

//Stores configuration data to be written to IC
//tx_cfg[n][0] = CFGR0
//tx_cfg[n][1] = CFGR1
//tx_cfg[n][2] = CFGR2
//tx_cfg[n][3] = CFGR3
//tx_cfg[n][4] = CFGR4
//tx_cfg[n][5] = CFGR5
uint8_t tx_cfg[TOTAL_IC][6];

//Stores returned configuration data
//rx_cfg[n][0] = CFGR0
//rx_cfg[n][1] = CFGR1
//rx_cfg[n][2] = CFGR2
//rx_cfg[n][3] = CFGR3
//rx_cfg[n][4] = CFGR4
//rx_cfg[n][5] = CFGR5
//rx_cfg[n][6] = PEC HIGH
//rx_cfg[n][7] = PEC LOW
uint8_t rx_cfg[TOTAL_IC][8];

//---------------------------------------------------------------------------------------

//Initializes the configuration array
//JTS2doLater: This doesn't need to be a 2D array.  Data identical on all LTC, except DCC12:1.
void LTC6804_init_cfg()
{
  for (int i = 0; i<TOTAL_IC; i++)
  {                        // BIT7    BIT6    BIT5    BIT4    BIT3    BIT2    BIT1   BIT0
    tx_cfg[i][0] = 0b11110110 ;  //GPIO5   GPIO4   GPIO3   GPIO2   GPIO1   REFON   SWTRD  ADCOPT //Enable GPIO pulldown, REFON
    tx_cfg[i][1] = 0x00       ;  //VUV[7]  VUV[6]  VUV[5]  VUV[4]  VUV[3]  VUV[2]  VUV[1] VUV[0] //Undervoltage comparison voltage
    tx_cfg[i][2] = 0x00       ;  //VOV[3]  VOV[2]  VOV[1]  VOV[0]  VUV[11] VUV[10] VUV[9] VUV[8]
    tx_cfg[i][3] = 0x00       ;  //VOV[11] VOV[10] VOV[9]  VOV[8]  VOV[7]  VOV[6]  VOV[5] VOV[4] //Overvoltage comparison voltage
    tx_cfg[i][4] = 0x00       ;  //DCC8    DCC7    DCC6    DCC5    DCC4    DCC3    DCC2   DCC1   //Enables discharge on cells 8:1
    tx_cfg[i][5] = 0x00       ;  //DCTO[3] DCTO[2] DCTO[1] DCTO[0] DCC12   DCC11   DCC10  DCC9   //Discharge timer and cells 12:9
  }
}

//---------------------------------------------------------------------------------------

void LTC6804_initialize()
{
  spi_enable(SPI_CLOCK_DIV64);
  set_adc(MD_NORMAL,DCP_DISABLED,CELL_CH_ALL,AUX_CH_GPIO1);
  LTC6804_init_cfg();        //initialize the 6804 configuration array to be written
  Serial.print(F("\nLTC6804 BEGIN"));
}

//---------------------------------------------------------------------------------------

void LTC6804_startCellVoltageConversion()
{
  wakeup_sleep();
  LTC6804_adcv(); //JTS2doNow: only run if all cell voltages have been read back
}

//---------------------------------------------------------------------------------------

//JTS2doNow: rename LTC6804_readNextCellVoltageRegister()
//Each call updates QTY3 cell voltages 
void LTC6804_readCellVoltages()
{
  wakeup_sleep();

  //round-robin state handlers
  static uint8_t chipAddress = FIRST_IC_ADDR;
  static char cellVoltageRegister = 'A'; //QTY3 cell voltages per register (e.g. A=cell01/cell02/cell03)

  uint8_t error = LTC6804_rdcv_process(chipAddress, cellVoltageRegister); 
  //JTS2doNow: Figure out why PEC error returned, even though cell voltages valid.

  //increment cellVoltageRegister & chipAddress
  cellVoltageRegister++;
  if(cellVoltageRegister == 'E' ) //LTC6804 only has registers A,B,C,D... reset to A
  { 
    cellVoltageRegister = 'A'; //reset
    chipAddress++;
    if(chipAddress == (FIRST_IC_ADDR + TOTAL_IC) ) { chipAddress = FIRST_IC_ADDR; } //reset  
  }

  if (error != 0)
  {
    LTC_isDataValid = 0;
    isoSPI_errorCount++;
    Serial.print(F("\nisoSPI error"));
    lcd_printNumErrors(isoSPI_errorCount); //JTS2doLater: Add LCD I2C update to round-robin (multiple errors could slow Superloop)
  } else {
    LTC_isDataValid = 1;
  }

  #ifdef PRINT_ALL_CELL_VOLTAGES_TO_USB
    #warning (Printing all cell voltages to USB severely reduces Superloop rate)
    printCellVoltage_all();
  #endif

  LTC6804_printCellVoltage_max_min(); //JTS2doNow: only calculate max/min after all cell voltages read.
}

//---------------------------------------------------------------------------------------

//JTS2doNow: better function name
//Reads and parses cell voltages from specified LTC6804 address and "cell voltage register" into 'cell_codes' array.
uint8_t LTC6804_rdcv_process(uint8_t chipAddress, char cellVoltageRegister)
{
  //Each LTC6804 has QTY4 "Cell Voltage Registers" (A/B/C/D)
  //Each CVR contains QTY3 cell voltages.  Each cell voltage is 2B
  const uint8_t NUM_BYTES_IN_REG        = 6; //QTY3 cells * 2B
  const uint8_t NUM_RX_BYTES            = 8; //NUM_BYTES_IN_REG + PEC (2B)

  int8_t pec_error = 0;

  uint8_t *returnedData;
  returnedData = (uint8_t *) malloc( NUM_RX_BYTES * sizeof(uint8_t) ); //previously 'cell_data'

  //Read single cell voltage register (QTY3 cell voltages) from specified IC
  LTC6804_rdcv_query(chipAddress, cellVoltageRegister, returnedData); //result stored in returnedData

  //parse 16b cell voltages from returnedData (from LTC6804)
  uint16_t cellX_Voltage_counts = returnedData[0] + (returnedData[1]<<8); //(lower byte)(upper byte)
  uint16_t cellY_Voltage_counts = returnedData[2] + (returnedData[3]<<8);
  uint16_t cellZ_Voltage_counts = returnedData[4] + (returnedData[5]<<8);
  uint16_t received_pec         = (returnedData[6]<<8) + returnedData[7]; //PEC0 PEC1

  uint16_t calculated_pec = pec15_calc(NUM_BYTES_IN_REG, &returnedData[0]);
  if (received_pec != calculated_pec) { pec_error = 1; } //PEC error occurred
  //JTS2doNow: Figure out why PEC values don't match (always errors)
  //JTS2doNow: only store result if PEC is valid

  //select which LTC cell voltages were read into returnedData
  uint8_t cellX=0; //1st cell in returnedData (LTC cell 1, 4, 7, or 10)
  uint8_t cellY=0; //2nd cell in returnedData (LTC cell 2, 5, 8, or 11) 
  uint8_t cellZ=0; //3rd cell in returnedData (LTC cell 3, 6, 9, or 12)
  switch(cellVoltageRegister)  //LUT to prevent QTY3 multiplies & QTY12 adds per call
  {
    case 'A': cellX=0;  cellY=1;  cellZ=2; break; //cells  1/ 2/ 3 (LTC 1-indexed, array 0-indexed)
    case 'B': cellX=3;  cellY=4;  cellZ=5; break; //cells  4/ 5/ 6
    case 'C': cellX=6;  cellY=7;  cellZ=8; break; //cells  7/ 8/ 9
    case 'D': cellX=9; cellY=10; cellZ=11; break; //cells 10/11/12
  }

  cell_codes[chipAddress - FIRST_IC_ADDR][cellX] = cellX_Voltage_counts;
  cell_codes[chipAddress - FIRST_IC_ADDR][cellY] = cellY_Voltage_counts;
  cell_codes[chipAddress - FIRST_IC_ADDR][cellZ] = cellZ_Voltage_counts;
  
  free(returnedData);

  return(pec_error);
}

//---------------------------------------------------------------------------------------

//Read a single "cell voltage register" and store the results in *data
//This function is rarely used outside of the LTC6804_rdcv_process() command.
//JTS2doNow: combine with LTC6804_rdcv_query
void LTC6804_rdcv_query(uint8_t chipAddress,
                         char cellVoltageRegister,
                         uint8_t *data //Unparsed cell codes
                        )
{
  uint8_t cmd[4];
  uint16_t temp_pec;

  cmd[0] = 0x80 + (chipAddress<<3); //configure LTC address (p46:Table33)

  switch(cellVoltageRegister) //chose which "cell voltage register" to read
  {
    case 'A': cmd[1] = 0x04; break;
    case 'B': cmd[1] = 0x06; break;
    case 'C': cmd[1] = 0x08; break;
    case 'D': cmd[1] = 0x0A; break;
  }

  temp_pec = pec15_calc(2, cmd);
  cmd[2] = (uint8_t)(temp_pec >> 8);
  cmd[3] = (uint8_t)(temp_pec);

  wakeup_idle (); //Guarantees LTC6804 isoSPI port is awake.
  digitalWrite(PIN_SPI_CS,LOW);
  spi_write_read(cmd,4,&data[0],8);
  digitalWrite(PIN_SPI_CS,HIGH);
}

//---------------------------------------------------------------------------------------

//JTS2doLater: Only output a few cells per call?
void printCellVoltage_all()
{ // t=42 milliseconds... blocking (serial transmit buffer filled)
  Serial.print('\n');
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F("IC "));
    Serial.print( (current_ic + FIRST_IC_ADDR) ,DEC);
    for (int i=0; i<12; i++)
    {
      Serial.print(F(" C"));
      Serial.print(i+1,DEC);
      Serial.print(':');
      Serial.print( (cell_codes[current_ic][i] * 0.0001), 4 );
      Serial.print(',');
    }
    Serial.print('\n');
  }
}

//---------------------------------------------------------------------------------------

//JTS2doNow: Roll for loops into LTC6804_printCellVoltage_max_min(); this fcn should just recall latest summed value
uint8_t LTC6804_getStackVoltage()
{
  uint32_t stackVoltage_RAW = 0; //Multiply by 0.0001 for volts

  //JTS2doNow: only update stack voltage if LTC_isDataValid
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    for (int i=0; i<12; i++)
    {
     stackVoltage_RAW += cell_codes[current_ic][i];
    }
  }

  uint8_t stackVoltage = (uint8_t)(stackVoltage_RAW * 0.0001);

  return stackVoltage;
}

//---------------------------------------------------------------------------------------

//JTS2doNow: rename LTC6804_calculateVoltages()
void LTC6804_printCellVoltage_max_min()
{
  uint16_t lowCellVoltage = 65535;
  uint16_t highCellVoltage = 0;

  if( LTC_isDataValid )
  {
    //JTS2doLater: find high/low while retrieving data from LTC6804
    //find high/low voltage
    for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
    {
      for (int i=0; i<12; i++)
      {
        if( cell_codes[current_ic][i] < lowCellVoltage )
        {
          lowCellVoltage = cell_codes[current_ic][i];
        }
        if( cell_codes[current_ic][i] > highCellVoltage )
        {
          highCellVoltage = cell_codes[current_ic][i];
        }
      }
    }

    lcd_printCellVoltage_hi(highCellVoltage);
    lcd_printCellVoltage_lo(lowCellVoltage);
    lcd_printCellVoltage_delta(highCellVoltage, lowCellVoltage);

    debugUSB_cellHI_counts(highCellVoltage);
    debugUSB_cellLO_counts(lowCellVoltage);
    
    //////////////////////////////////////////////////////////////////////////////////
  
    if( highCellVoltage > maxEverCellVoltage )
    {
      maxEverCellVoltage = highCellVoltage;
      lcd_printMaxEverVoltage(maxEverCellVoltage);
    }

    if( lowCellVoltage < minEverCellVoltage )
    {
      minEverCellVoltage = lowCellVoltage;
      lcd_printMinEverVoltage(minEverCellVoltage);
    }
  }

  //////////////////////////////////////////////////////////////////////////////////

  //JTS2doLater: make this its own function (in gridCharger.c)
  //grid charger handling
  if( (highCellVoltage <= 39000) && !(digitalRead(PIN_GRID_SENSE)) ) //Battery not full and grid charger is plugged in
  {
    if( highCellVoltage <= 38950) //hysteresis to prevent rapid cycling
    {
      digitalWrite(PIN_GRID_EN,1); //enable grid charger
      analogWrite(PIN_FAN_PWM,125); //enable fan
    }
  }
  else
  {  //battery is full or grid charger isn't plugged in
    digitalWrite(PIN_GRID_EN,0); //disable grid charger
    analogWrite(PIN_FAN_PWM,0); //disable fan
  }
}

//---------------------------------------------------------------------------------------

// |command    |  10   |   9   |   8   |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
// |-----------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
// |ADCV:      |   0   |   1   | MD[1] | MD[2] |   1   |   1   |  DCP  |   0   | CH[2] | CH[1] | CH[0] |
// |ADAX:      |   1   |   0   | MD[1] | MD[2] |   1   |   1   |  DCP  |   0   | CHG[2]| CHG[1]| CHG[0]|
void set_adc(uint8_t MD, //ADC Conversion Mode (LPF corner frequency)
             uint8_t DCP,//Discharge permitted during cell conversions?
             uint8_t CH, //Cell Channels to measure during ADC conversion command
             uint8_t CHG //GPIO Channels to measure during Auxiliary conversion command
            )
{
  uint8_t md_bits;

  md_bits = (MD & 0x02) >> 1; //set bit 8 (MD[1]) in ADCV[0]... MD[0] is in ADCV[1]
  ADCV[0] = md_bits + 0x02;  //set bit 9 true
  md_bits = (MD & 0x01) << 7;
  ADCV[1] =  md_bits + 0x60 + (DCP<<4) + CH;

  //JTS2doNow: Divide set_adc() into two functions.  New one does code below:
  md_bits = (MD & 0x02) >> 1;
  ADAX[0] = md_bits + 0x04;
  md_bits = (MD & 0x01) << 7;
  ADAX[1] = md_bits + 0x60 + CHG ;
}

//---------------------------------------------------------------------------------------

//Start cell voltage conversion
void LTC6804_adcv()
{
  uint8_t cmd[4];
  uint16_t temp_pec;

  //Load ADCV command into cmd array
  cmd[0] = ADCV[0];
  cmd[1] = ADCV[1];

  //Calculate adcv cmd PEC and load pec into cmd array
  temp_pec = pec15_calc(2, ADCV);
  cmd[2] = (uint8_t)(temp_pec >> 8);
  cmd[3] = (uint8_t)(temp_pec);

  wakeup_idle (); //Guarantees LTC6804 isoSPI port is awake.

  //send broadcast adcv command to LTC6804 stack
  digitalWrite(PIN_SPI_CS,LOW);
  spi_write_array(4,cmd);
  digitalWrite(PIN_SPI_CS,HIGH);
}

//---------------------------------------------------------------------------------------

//Start GPIO conversion
void LTC6804_adax()
{
  uint8_t cmd[4];
  uint16_t temp_pec;


  //Load adax command into cmd array
  cmd[0] = ADAX[0];
  cmd[1] = ADAX[1];

  //Calculate adax cmd PEC and load pec into cmd array
  temp_pec = pec15_calc(2, ADAX);
  cmd[2] = (uint8_t)(temp_pec >> 8);
  cmd[3] = (uint8_t)(temp_pec);

  wakeup_idle (); //Guarantees LTC6804 isoSPI port is awake.

  //send broadcast adax command to LTC6804 stack
  digitalWrite(PIN_SPI_CS,LOW);
  spi_write_array(4,cmd);
  digitalWrite(PIN_SPI_CS,HIGH);
}

//---------------------------------------------------------------------------------------


//Reads and parses aux voltages from LTC6804 registers into 'aux_codes' variable.
int8_t LTC6804_rdaux(uint8_t reg, //controls which aux voltage register to read (0=all, 1=A, 2=B)
                     uint8_t total_ic,
                     uint16_t aux_codes[][6],
                     uint8_t addr_first_ic
                    )
{
  const uint8_t NUM_RX_BYTES = 8;
  const uint8_t NUM_BYTES_IN_REG = 6;
  const uint8_t GPIO_IN_REG = 3;

  uint8_t *data;
  uint8_t data_counter = 0;
  int8_t pec_error = 0;
  uint16_t received_pec;
  uint16_t data_pec;
  data = (uint8_t *) malloc((NUM_RX_BYTES*total_ic)*sizeof(uint8_t));

  if (reg == 0)
  { //Read GPIO voltage registers A-B for every IC in the stack
    for (uint8_t gpio_reg = 1; gpio_reg<3; gpio_reg++) //executes once for each aux voltage register
    {
      data_counter = 0;
      LTC6804_rdaux_reg(gpio_reg, total_ic,data, addr_first_ic);
      for (uint8_t current_ic = 0 ; current_ic < total_ic; current_ic++) //executes once for each LTC6804
      {
        //Parse raw GPIO voltage data in aux_codes array
        for (uint8_t current_gpio = 0; current_gpio< GPIO_IN_REG; current_gpio++) //Parses GPIO voltage stored in the register
        {
          aux_codes[current_ic][current_gpio +((gpio_reg-1)*GPIO_IN_REG)] = data[data_counter] + (data[data_counter+1]<<8);
          data_counter=data_counter+2;
        }
        //Verify PEC matches calculated value for each read register command
        received_pec = (data[data_counter]<<8)+ data[data_counter+1];
        data_pec = pec15_calc(NUM_BYTES_IN_REG, &data[current_ic*NUM_RX_BYTES]);
        if (received_pec != data_pec)
        {
          pec_error = 1;
        }
        data_counter=data_counter+2;
      }
    }
  } else {
    //Read single GPIO voltage register for all ICs in stack
    LTC6804_rdaux_reg(reg, total_ic, data, addr_first_ic);
    for (int current_ic = 0 ; current_ic < total_ic; current_ic++) // executes for every LTC6804 in the stack
    {
      //Parse raw GPIO voltage data in cell_codes array
      for (int current_gpio = 0; current_gpio<GPIO_IN_REG; current_gpio++)  // This loop parses the read back data. Loops
      {
        // once for each aux voltage in the register
        aux_codes[current_ic][current_gpio +((reg-1)*GPIO_IN_REG)] = 0x0000FFFF & (data[data_counter] + (data[data_counter+1]<<8));
        data_counter=data_counter+2;
      }
      //Verify PEC matches calculated value for each read register command
      received_pec = (data[data_counter]<<8) + data[data_counter+1];
      data_pec = pec15_calc(6, &data[current_ic*8]);
      if (received_pec != data_pec)
      {
        pec_error = 1;
      }
    }
  }
  free(data);
  return (pec_error);
}

//---------------------------------------------------------------------------------------


/***********************************************//**
 \brief Read the raw data from the LTC6804 auxiliary register

 The function reads a single GPIO voltage register and stores the read data
 in the *data point as a byte array. This function is rarely used outside of
 the LTC6804_rdaux() command.

 @param[in] uint8_t reg; This controls which GPIO voltage register is read back.

          1: Read back auxiliary group A

          2: Read back auxiliary group B


 @param[in] uint8_t total_ic; This is the number of ICs in the stack

 @param[out] uint8_t *data; An array of the unparsed aux codes
 *************************************************/
void LTC6804_rdaux_reg(uint8_t reg,
                       uint8_t total_ic,
                       uint8_t *data,
                       uint8_t addr_first_ic
                      )
{
  uint8_t cmd[4];
  uint16_t cmd_pec;

  //1
  if (reg == 1)
  {
    cmd[1] = 0x0C;
    cmd[0] = 0x00;
  }
  else if (reg == 2)
  {
    cmd[1] = 0x0e;
    cmd[0] = 0x00;
  }
  else
  {
    cmd[1] = 0x0C;
    cmd[0] = 0x00;
  }
  //2
  cmd_pec = pec15_calc(2, cmd);
  cmd[2] = (uint8_t)(cmd_pec >> 8);
  cmd[3] = (uint8_t)(cmd_pec);

  //3
  wakeup_idle (); //This will guarantee that the LTC6804 isoSPI port is awake, this command can be removed.
  //4
  for (int current_ic = 0; current_ic<total_ic; current_ic++)
  {
    cmd[0] = 0x80 + ( (current_ic + addr_first_ic) << 3); //Setting address
    cmd_pec = pec15_calc(2, cmd);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);
    digitalWrite(PIN_SPI_CS,LOW);
    spi_write_read(cmd,4,&data[current_ic*8],8);
    digitalWrite(PIN_SPI_CS,HIGH);
  }
}
/*
  LTC6804_rdaux_reg Function Process:
  1. Determine Command and initialize command array
  2. Calculate Command PEC
  3. Wake up isoSPI, this step is optional
  4. Send Global Command to LTC6804 stack
*/

//---------------------------------------------------------------------------------------


//JTS: Not used anywhere.  Use as prototype for other similar calls.
/********************************************************//**
 \brief Clears the LTC6804 cell voltage registers

 The command clears the cell voltage registers and intiallizes
 all values to 1. The register will read back hexadecimal 0xFF
 after the command is sent.
************************************************************/
void LTC6804_clrcell()
{
  uint8_t cmd[4];
  uint16_t cmd_pec;

  //1
  cmd[0] = 0x07;
  cmd[1] = 0x11;

  //2
  cmd_pec = pec15_calc(2, cmd);
  cmd[2] = (uint8_t)(cmd_pec >> 8);
  cmd[3] = (uint8_t)(cmd_pec );

  //3
  wakeup_idle (); //This will guarantee that the LTC6804 isoSPI port is awake. This command can be removed.

  //4
  digitalWrite(PIN_SPI_CS,LOW);
  spi_write_read(cmd,4,0,0);
  digitalWrite(PIN_SPI_CS,HIGH);
}
/*
  LTC6804_clrcell Function sequence:

  1. Load clrcell command into cmd array
  2. Calculate clrcell cmd PEC and load pec into cmd array
  3. wakeup isoSPI port, this step can be removed if isoSPI status is previously guaranteed
  4. send broadcast clrcell command to LTC6804 stack
*/

//---------------------------------------------------------------------------------------

//JTS: Not used anywhere.  Use as prototype for other similar calls.
/***********************************************************//**
 \brief Clears the LTC6804 Auxiliary registers

 The command clears the Auxiliary registers and intiallizes
 all values to 1. The register will read back hexadecimal 0xFF
 after the command is sent.
***************************************************************/
void LTC6804_clraux()
{
  uint8_t cmd[4];
  uint16_t cmd_pec;

  //1
  cmd[0] = 0x07;
  cmd[1] = 0x12;

  //2
  cmd_pec = pec15_calc(2, cmd);
  cmd[2] = (uint8_t)(cmd_pec >> 8);
  cmd[3] = (uint8_t)(cmd_pec);

  //3
  wakeup_idle (); //This will guarantee that the LTC6804 isoSPI port is awake.This command can be removed.
  //4
  digitalWrite(PIN_SPI_CS,LOW);
  spi_write_read(cmd,4,0,0);
  digitalWrite(PIN_SPI_CS,HIGH);
}
/*
  LTC6804_clraux Function sequence:

  1. Load clraux command into cmd array
  2. Calculate clraux cmd PEC and load pec into cmd array
  3. wakeup isoSPI port, this step can be removed if isoSPI status is previously guaranteed
  4. send broadcast clraux command to LTC6804 stack
*/

//---------------------------------------------------------------------------------------

/*****************************************************//**
 \brief Write the LTC6804 configuration register

 This command will write the configuration registers of the stacks
 connected in a stack stack. The configuration is written in descending
 order so the last device's configuration is written first.


@param[in] uint8_t total_ic; The number of ICs being written.

@param[in] uint8_t *config an array of the configuration data that will be written, the array should contain the 6 bytes for each
 IC in the stack. The lowest IC in the stack should be the first 6 byte block in the array. The array should
 have the following format:
 |  config[0]| config[1] |  config[2]|  config[3]|  config[4]|  config[5]| config[6] |  config[7]|  config[8]|  .....    |
 |-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|
 |IC1 CFGR0  |IC1 CFGR1  |IC1 CFGR2  |IC1 CFGR3  |IC1 CFGR4  |IC1 CFGR5  |IC2 CFGR0  |IC2 CFGR1  | IC2 CFGR2 |  .....    |

 The function will calculate the needed PEC codes for the write data
 and then transmit data to the ICs on a stack.
********************************************************/
void LTC6804_wrcfg(uint8_t total_ic, uint8_t config[][6], uint8_t addr_first_ic)
{
  const uint8_t BYTES_IN_REG = 6;
  const uint8_t CMD_LEN = 4+(8*total_ic);
  uint8_t *cmd;
  uint16_t temp_pec;
  uint8_t cmd_index; //command counter

  cmd = (uint8_t *)malloc(CMD_LEN*sizeof(uint8_t));
  //1
  cmd[0] = 0x00;
  cmd[1] = 0x01;
  cmd[2] = 0x3d;
  cmd[3] = 0x6e;

  //2
  cmd_index = 4;
  for (uint8_t current_ic = 0; current_ic < total_ic ; current_ic++)       // executes for each LTC6804 in stack,
  {
    for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) // executes for each byte in the CFGR register
    {
      // i is the byte counter

      cmd[cmd_index] = config[current_ic][current_byte];    //adding the config data to the array to be sent
      cmd_index = cmd_index + 1;
    }
    //3
    temp_pec = (uint16_t)pec15_calc(BYTES_IN_REG, &config[current_ic][0]);// calculating the PEC for each board
    cmd[cmd_index] = (uint8_t)(temp_pec >> 8);
    cmd[cmd_index + 1] = (uint8_t)temp_pec;
    cmd_index = cmd_index + 2;
  }

  //4
  wakeup_idle ();                                //This will guarantee that the LTC6804 isoSPI port is awake.This command can be removed.
  //5
  for (int current_ic = 0; current_ic<total_ic; current_ic++)
  {
    cmd[0] = 0x80 + ( (current_ic + addr_first_ic) << 3); //Setting address
    temp_pec = pec15_calc(2, cmd);
    cmd[2] = (uint8_t)(temp_pec >> 8);
    cmd[3] = (uint8_t)(temp_pec);
    digitalWrite(PIN_SPI_CS,LOW);
    spi_write_array(4,cmd);
    spi_write_array(8,&cmd[4+(8*current_ic)]);
    digitalWrite(PIN_SPI_CS,HIGH);
  }
  free(cmd);
}
/*
  1. Load cmd array with the write configuration command and PEC
  2. Load the cmd with LTC6804 configuration data
  3. Calculate the pec for the LTC6804 configuration data being transmitted
  4. wakeup isoSPI port, this step can be removed if isoSPI status is previously guaranteed
  5. Write configuration of each LTC6804 on the stack
*/

//---------------------------------------------------------------------------------------


/*!******************************************************
 \brief Reads configuration registers of a LTC6804 stack

@param[out] uint8_t *r_config: array that the function will write configuration data to. The configuration data for each IC
is stored in blocks of 8 bytes with the configuration data of the lowest IC on the stack in the first 8 bytes
of the array, the second IC in the second 8 byte etc. Below is an table illustrating the array organization:

|r_config[0]|r_config[1]|r_config[2]|r_config[3]|r_config[4]|r_config[5]|r_config[6]  |r_config[7] |r_config[8]|r_config[9]|  .....    |
|-----------|-----------|-----------|-----------|-----------|-----------|-------------|------------|-----------|-----------|-----------|
|IC1 CFGR0  |IC1 CFGR1  |IC1 CFGR2  |IC1 CFGR3  |IC1 CFGR4  |IC1 CFGR5  |IC1 PEC High |IC1 PEC Low |IC2 CFGR0  |IC2 CFGR1  |  .....    |


@return int8_t PEC Status.
  0: Data read back has matching PEC

  -1: Data read back has incorrect PEC
********************************************************/
int8_t LTC6804_rdcfg(uint8_t total_ic, uint8_t r_config[][8], uint8_t addr_first_ic)
{
  const uint8_t BYTES_IN_REG = 8;

  uint8_t cmd[4];
  uint8_t *rx_data;
  int8_t pec_error = 0;
  uint16_t data_pec;
  uint16_t received_pec;
  rx_data = (uint8_t *) malloc((8*total_ic)*sizeof(uint8_t));
  //1
  cmd[0] = 0x00;
  cmd[1] = 0x02;
  cmd[2] = 0x2b;
  cmd[3] = 0x0A;

  //2
  wakeup_idle (); //This will guarantee that the LTC6804 isoSPI port is awake. This command can be removed.
  //3
  for (int current_ic = 0; current_ic<total_ic; current_ic++)
  {
    cmd[0] = 0x80 + ( (current_ic + addr_first_ic) <<3); //Setting address
    data_pec = pec15_calc(2, cmd);
    cmd[2] = (uint8_t)(data_pec >> 8);
    cmd[3] = (uint8_t)(data_pec);
    digitalWrite(PIN_SPI_CS,LOW);
    spi_write_read(cmd,4,&rx_data[current_ic*8],8);
    digitalWrite(PIN_SPI_CS,HIGH);
  }

  for (uint8_t current_ic = 0; current_ic < total_ic; current_ic++) //executes for each LTC6804 in the stack
  {
    //4.a
    for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++)
    {
      r_config[current_ic][current_byte] = rx_data[current_byte + (current_ic*BYTES_IN_REG)];
    }
    //4.b
    received_pec = (r_config[current_ic][6]<<8) + r_config[current_ic][7];
    data_pec = pec15_calc(6, &r_config[current_ic][0]);
    if (received_pec != data_pec)
    {
      pec_error = 1;
    }
  }
  free(rx_data);
  //5
  return(pec_error);
}
/*
  1. Load cmd array with the write configuration command and PEC
  2. wakeup isoSPI port, this step can be removed if isoSPI status is previously guaranteed
  3. read configuration of each LTC6804 on the stack
  4. For each LTC6804 in the stack
    a. load configuration data into r_config array
    b. calculate PEC of received data and compare against calculated PEC
  5. Return PEC Error
*/

//---------------------------------------------------------------------------------------


/*!****************************************************
  \brief Wake isoSPI up from idle state
 Generic wakeup command to wake isoSPI up out of idle
 *****************************************************/
void wakeup_idle()
{
  const uint8_t T_IDLE_isoSPI_millis = 4; //'tidle' (absMIN) = 4.3 ms

  if( (millis() - lastTimeDataSent_millis) > T_IDLE_isoSPI_millis )
  { //LTC6804 isoSPI might be asleep
    digitalWrite(PIN_SPI_CS,LOW);
    delayMicroseconds(10); //Guarantees the isoSPI will be in ready mode
    digitalWrite(PIN_SPI_CS,HIGH);
  }
}

//---------------------------------------------------------------------------------------


/*!****************************************************
  \brief Wake the LTC6804 from the sleep state

 Generic wakeup command to wake the LTC6804 from sleep
 *****************************************************/
void wakeup_sleep()
{
  const uint16_t T_SLEEP_WATCHDOG_MILLIS = 1800; //'tsleep' (absMIN) = 1800 ms 

  if( (millis() - lastTimeDataSent_millis) > T_SLEEP_WATCHDOG_MILLIS )
  { //LTC6804 core might be asleep
    digitalWrite(PIN_SPI_CS,LOW);
    delay(1); // Guarantees the LTC6804 will be in standby
    digitalWrite(PIN_SPI_CS,HIGH);
  }
}

//---------------------------------------------------------------------------------------


/*!**********************************************************
 \brief calaculates  and returns the CRC15


@param[in]  uint8_t len: the length of the data array being passed to the function

@param[in]  uint8_t data[] : the array of data that the PEC will be generated from


@return  The calculated pec15 as an unsigned int16_t
***********************************************************/
uint16_t pec15_calc(uint8_t len, uint8_t *data)
{
  uint16_t remainder,addr;

  remainder = 16;//initialize the PEC
  for (uint8_t i = 0; i<len; i++) // loops for each byte in data array
  {
    addr = ((remainder>>7)^data[i])&0xff;//calculate PEC table address
    remainder = (remainder<<8)^crc15Table[addr];
  }
  return(remainder*2);//The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
}

//---------------------------------------------------------------------------------------


/*!
 \brief Writes an array of bytes out of the SPI port

 @param[in] uint8_t len length of the data array being written on the SPI port
 @param[in] uint8_t data[] the data array to be written on the SPI port

*/
void spi_write_array(uint8_t len, // Option: Number of bytes to be written on the SPI port
                     uint8_t data[] //Array of bytes to be written on the SPI port
                    )
{
  for (uint8_t i = 0; i < len; i++)
  {
    spi_write((char)data[i]);
  }

  lastTimeDataSent_millis = millis(); //all SPI writes occur here
}

//---------------------------------------------------------------------------------------


/*!
 \brief Writes and read a set number of bytes using the SPI port.

@param[in] uint8_t tx_data[] array of data to be written on the SPI port
@param[in] uint8_t tx_len length of the tx_data array
@param[out] uint8_t rx_data array that read data will be written too.
@param[in] uint8_t rx_len number of bytes to be read from the SPI port.
*/
void spi_write_read(uint8_t tx_Data[],//array of data to be written on SPI port
                    uint8_t tx_len, //length of the tx data arry
                    uint8_t *rx_data,//Input: array that will store the data read by the SPI port
                    uint8_t rx_len //Option: number of bytes to be read from the SPI port
                   )
{
  for (uint8_t i = 0; i < tx_len; i++)
  {
    spi_write(tx_Data[i]);
  }

  for (uint8_t i = 0; i < rx_len; i++)
  {
    rx_data[i] = (uint8_t)spi_read(0xFF);
  }
}

//---------------------------------------------------------------------------------------

void LTC6804_handleKeyOff(void)
{
  isoSPI_errorCount = 0;
  maxEverCellVoltage = 0;
  minEverCellVoltage = 65535;  
}