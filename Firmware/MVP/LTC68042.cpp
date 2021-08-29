/*!
Copyright 2021(c) John Sullivan, for Honda Insight LiBCM (BCM Replacement)
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

//-LTC68042_cellVoltage.c
//-LTC68042_GPIO.c
//-LTC68042_configure.c

uint8_t isoSPI_errorCount = 0; //JTS2doNow: turn into function
uint32_t lastTimeDataSent_millis = 0; //LTC idle timer resets each time data is transferred

//conversion command variables. //JTS2doLater: Bring these inside each function that uses them (as temp)
uint8_t ADCV[2]; //Cell Voltage conversion command.
uint8_t ADAX[2]; //GPIO conversion command.

const uint8_t TOTAL_IC = 4; //number of ICs in the isoSPI network
const uint8_t FIRST_IC_ADDR = 2; //lowest address.  All additional IC addresses must be sequential
const uint8_t CELLS_PER_IC = 12; //Each LTC6804 measures QTY12 cells

uint16_t maxEverCellVoltage; //since last key event
uint16_t minEverCellVoltage; //since last key event

//Stores returned cell voltages
//Note: cell# & IC# are 1-indexed, whereas array data is 0-indexed
//  cellVoltages_counts[0][ 3] is IC1 cell  4
//  cellVoltages_counts[3][11] is IC4 cell 12
//There are twice as many array elements as there are cell voltages (e.g. with QTY48 cells, there are QTY96 elements)
//Either the top or bottom half is a buffer containing the last completely read back frame.
//The other half is always where the incoming voltages are stored as they're read back from each LTC6804.
//Therefore, cell voltage comparisons use ADC data that was acquired by the ADC at roughly the same time.
//cellVoltages_counts[0][               8] is IC1 Cell 9 when cellVoltages_lastCompleteFrame == BOTTOM
//cellVoltages_counts[0][CELLS_PER_IC + 8] is IC1 Cell 9 when cellVoltages_lastCompleteFrame == TOP
uint16_t cellVoltages_counts[TOTAL_IC][CELLS_PER_IC * 2]; //Twice as many rows as cells, so we can ping-pong between complete frames

//JTS2doNow: Move to header file
#define COMPLETE_FRAME_IS_TOP    2
#define COMPLETE_FRAME_IS_BOTTOM 1
#define COMPLETE_FRAME_IS_NONE   0

//JTS2doNow: create LTC6804_getLastCompleteFrame() & LTC6804_setLastCompleteFrame()
//Determines which half (top or bottom) of cellVoltages_counts[][] contains the last complete frame
uint8_t cellVoltages_lastCompleteFrame = COMPLETE_FRAME_IS_NONE;

//Stores configuration data to be written to IC
//tx_cfg[n][0] = CFGR0
//tx_cfg[n][1] = CFGR1
//tx_cfg[n][2] = CFGR2
//tx_cfg[n][3] = CFGR3
//tx_cfg[n][4] = CFGR4
//tx_cfg[n][5] = CFGR5
uint8_t tx_cfg[TOTAL_IC][6];

//---------------------------------------------------------------------------------------

//Initializes the configuration array
//JTS2doLater: Write separate function to control discharge FETs (cell balancing)
//JTS2doLater: This doesn't need to be a 2D array.  Data identical on all LTC, except DCC12:1.
//JTS2doLater: Figure out where tx_cfg is actually written to LTC ICs
//JTS2doNow: Are we properly configuring LTC ADC?
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
  setADC_cells(MD_NORMAL,DCP_DISABLED,CELL_CH_ALL);
  LTC6804_init_cfg();        //initialize the 6804 configuration array to be written
  Serial.print(F("\nLTC6804 BEGIN"));
  //JTS2doLater: If P-code occurs, read back 1st QTY3 cells and spoof returned voltage on all cellVoltages_counts[][] elements
}

//---------------------------------------------------------------------------------------

//all cell voltages are measured
void LTC68042voltage_startCellConversion()
{
  wakeup_sleep();

  uint8_t cmd[4];
  uint16_t temp_pec;

  //Load 'ADCV' command into cmd array
  cmd[0] = ADCV[0];
  cmd[1] = ADCV[1];

  //Calculate adcv cmd PEC and load pec into cmd array
  temp_pec = pec15_calc(2, ADCV);
  cmd[2] = (uint8_t)(temp_pec >> 8);
  cmd[3] = (uint8_t)(temp_pec);

  wakeup_isoSPI(); //Guarantees LTC6804 isoSPI port is awake.

  //send broadcast adcv command to LTC6804 stack
  digitalWrite(PIN_SPI_CS,LOW);
  spi_write_array(4,cmd);
  digitalWrite(PIN_SPI_CS,HIGH); 
}

//---------------------------------------------------------------------------------------

//Each call updates QTY3 cell voltages
//Results stored in file-scoped "cellVoltages_counts[][]"" array
void LTC68042voltage_getNextCVR()
{
  wakeup_sleep();

  //round-robin state handlers
  static uint8_t chipAddress = (FIRST_IC_ADDR);
  static char cellVoltageRegister = 'A'; //QTY3 cell voltages per register

  //only true first time called after reset 
  if(cellVoltages_lastCompleteFrame == COMPLETE_FRAME_IS_NONE)
  { //First run
    LTC68042voltage_startCellConversion();
    delayMicroseconds(5000);  //wait for initial ADC conversion to complete //JTS2doNow: Figure out minimum delay
  }

  //on first run (after reset), while loop repeats until all cells measured
  //after first run, while loop always runs exactly once (i.e. only three cells retrieved per call)
  do 
  {
    //Each "cell voltage register" contains three 16b cell voltages
    LTC68042voltage_validateCVR(chipAddress, cellVoltageRegister); 

    //determine which cell voltage register to read next 
    cellVoltageRegister++;
    if(cellVoltageRegister >= 'E' ) //LTC6804 only has registers A,B,C,D... so reset back to A
    { 
      cellVoltageRegister = 'A'; //reset
      chipAddress++; //move to next LTC6804 IC

      if(chipAddress >= (FIRST_IC_ADDR + TOTAL_IC) ) //when we get to last address, reset back to first CVR
      {
        chipAddress = FIRST_IC_ADDR; //reset back to first IC
        LTC68042voltage_startCellConversion(); //start new conversion (all ICs will measure all cells)

        //set which half of cellVoltages_counts[][] (top or bottom) we just completed storing all cell voltages into
        switch(cellVoltages_lastCompleteFrame)
        {
          case COMPLETE_FRAME_IS_TOP   : cellVoltages_lastCompleteFrame = COMPLETE_FRAME_IS_BOTTOM; break; //just finished writing bottom
          case COMPLETE_FRAME_IS_BOTTOM: cellVoltages_lastCompleteFrame = COMPLETE_FRAME_IS_TOP;    break; //just finished writing top
          case COMPLETE_FRAME_IS_NONE  : cellVoltages_lastCompleteFrame = COMPLETE_FRAME_IS_TOP;    break;
        }
      }
    }
  } while (cellVoltages_lastCompleteFrame == COMPLETE_FRAME_IS_NONE);

  switch(cellVoltages_lastCompleteFrame)
    {
      case COMPLETE_FRAME_IS_TOP   : break;
      case COMPLETE_FRAME_IS_BOTTOM: break; 
      case COMPLETE_FRAME_IS_NONE  : break;
    }

  #ifdef PRINT_ALL_CELL_VOLTAGES_TO_USB
    #warning (Printing all cell voltages to USB severely reduces Superloop rate)
    printCellVoltage_all();
  #endif

  LTC6804_printCellVoltage_max_min(); //JTS2doNow: only calculate max/min after all cell voltages read.
}

//---------------------------------------------------------------------------------------

//determine which half of cellVoltages_counts[][] (top or bottom) contains updated array elements for all pack cell voltages
//frameOffset == 0            means we read array elements 00:11 (bottom)
//frameOffset == CELLS_PER_IC means we read array elements 12:23 (top)
uint8_t LTC6804_getFrameOffset_Read(void)
{
  uint8_t frameOffset=0;

  switch(cellVoltages_lastCompleteFrame)
  {  
    case COMPLETE_FRAME_IS_TOP   : frameOffset = CELLS_PER_IC; break;
    case COMPLETE_FRAME_IS_BOTTOM: frameOffset = 0           ; break; 
       //COMPLETE_FRAME_IS_NONE never makes it here 
  }

  return frameOffset;
}

//---------------------------------------------------------------------------------------

//Validate QTY3 cell voltages from specified LTC6804's specified "cell voltage register"
//Valid results stored in 'cellVoltages_counts' array.
void LTC68042voltage_validateCVR(uint8_t chipAddress, char cellVoltageRegister)
{
  //Each LTC6804 has QTY4 "Cell Voltage Registers" (A/B/C/D)
  //Each CVR contains QTY3 cell voltages.  Each cell voltage is 2B
  const uint8_t NUM_BYTES_IN_REG        = 6; //QTY3 cells * 2B
  const uint8_t NUM_RX_BYTES            = 8; //NUM_BYTES_IN_REG + PEC (2B)

  uint8_t *returnedData; //previously named 'cell_data'
  returnedData = (uint8_t *) malloc( NUM_RX_BYTES * sizeof(uint8_t) );

  uint8_t attemptCounter = 0;
  uint16_t received_pec;
  uint16_t calculated_pec;
  uint16_t cellX_Voltage_counts;
  uint16_t cellY_Voltage_counts;
  uint16_t cellZ_Voltage_counts;
  do //repeats until PECs match (no transmission error) 
  {
    //Read single cell voltage register (QTY3 cell voltages) from specified IC
    LTC68042voltage_transmitCVR(chipAddress, cellVoltageRegister, returnedData); //result stored in returnedData

    //parse 16b cell voltages from returnedData (from LTC6804)
    cellX_Voltage_counts = returnedData[0] + (returnedData[1]<<8); //(lower byte)(upper byte)
    cellY_Voltage_counts = returnedData[2] + (returnedData[3]<<8);
    cellZ_Voltage_counts = returnedData[4] + (returnedData[5]<<8);
    
    received_pec   = (returnedData[6]<<8) + returnedData[7]; //PEC0 PEC1
    calculated_pec = pec15_calc(NUM_BYTES_IN_REG, &returnedData[0]);

    attemptCounter++; //prevent while loop hang

  } while( (received_pec != calculated_pec) && (attemptCounter < 4) ); //stops when PECs match or timeout occurs   

  if (attemptCounter > 1)
  { //PEC error occurred
    isoSPI_errorCount++;
    Serial.print(F("\nerr:LTC"));
    lcd_printNumErrors(isoSPI_errorCount); //JTS2doLater: Add LCD I2C update to round-robin (multiple errors could slow Superloop)
  } 
  else //PEC matches (data transmitted correctly)
  {
    //Determine which LTC cell voltages were read into returnedData
    uint8_t cellX=0; //1st cell in returnedData (LTC cell 1, 4, 7, or 10)
    uint8_t cellY=0; //2nd cell in returnedData (LTC cell 2, 5, 8, or 11) 
    uint8_t cellZ=0; //3rd cell in returnedData (LTC cell 3, 6, 9, or 12)
    switch(cellVoltageRegister)  //LUT to prevent QTY3 multiplies & QTY12 adds per call
    {
      case 'A': cellX=0;  cellY=1;  cellZ=2 ; break; //cells  1/ 2/ 3 (LTC 1-indexed, array 0-indexed)
      case 'B': cellX=3;  cellY=4;  cellZ=5 ; break; //cells  4/ 5/ 6
      case 'C': cellX=6;  cellY=7;  cellZ=8 ; break; //cells  7/ 8/ 9
      case 'D': cellX=9;  cellY=10; cellZ=11; break; //cells 10/11/12
    }

    //determine which half of cellVoltages_counts is "work in progress"
    uint8_t frameOffset=0;
    switch(cellVoltages_lastCompleteFrame)
      {  
        case COMPLETE_FRAME_IS_TOP   : frameOffset = 0            ; break;
        case COMPLETE_FRAME_IS_BOTTOM: frameOffset = CELLS_PER_IC ; break;
        case COMPLETE_FRAME_IS_NONE  : frameOffset = 0            ; break;
      }
    
    //store cell voltages in the proper location
    cellVoltages_counts[chipAddress - FIRST_IC_ADDR][cellX + frameOffset] = cellX_Voltage_counts;
    cellVoltages_counts[chipAddress - FIRST_IC_ADDR][cellY + frameOffset] = cellY_Voltage_counts;
    cellVoltages_counts[chipAddress - FIRST_IC_ADDR][cellZ + frameOffset] = cellZ_Voltage_counts;
  }
  
  free(returnedData);
}


//---------------------------------------------------------------------------------------

//JTS2doNow: Roll for loops into LTC6804_printCellVoltage_max_min(); this fcn should just recall latest summed value
uint8_t LTC6804_getStackVoltage()
{
  uint32_t stackVoltage_RAW = 0; //Multiply by 0.0001 for volts

  //Returns 0           when reading from bottom half of cellVoltages array
  //Returns CELL_PER_IC when reading from top    half of cellVoltages array
  uint8_t frameOffset = LTC6804_getFrameOffset_Read();

  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    for (int i=0; i<12; i++)
    {
     stackVoltage_RAW += cellVoltages_counts[current_ic][i+frameOffset];
    }
  }

  uint8_t stackVoltage = (uint8_t)(stackVoltage_RAW * 0.0001);

  return stackVoltage;
}

void setStackVoltage_volts (uint8_t stackVoltage)
{
  //JTS2doNow: implement
}

//---------------------------------------------------------------------------------------

//JTS2doNow: rename LTC6804_calculateVoltages()
//JTS2doNow: combine with other functions
void LTC6804_printCellVoltage_max_min()
{
  uint16_t lowCellVoltage = 65535;
  uint16_t highCellVoltage = 0;

  //Returns 0           when reading from bottom half of cellVoltages array
  //Returns CELL_PER_IC when reading from top    half of cellVoltages array
  uint8_t frameOffset = LTC6804_getFrameOffset_Read();

  //JTS2doLater: find high/low while retrieving data from LTC6804
  //find high/low voltage
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    for (int i=0; i<12; i++)
    {
      if( cellVoltages_counts[current_ic][i+frameOffset] < lowCellVoltage )
      {
        lowCellVoltage = cellVoltages_counts[current_ic][i+frameOffset];
      }
      if( cellVoltages_counts[current_ic][i+frameOffset] > highCellVoltage )
      {
        highCellVoltage = cellVoltages_counts[current_ic][i+frameOffset];
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

//Read a single "cell voltage register" and store the results in *data
//This function is ONLY used by LTC68042voltage_validateCVR().
void LTC68042voltage_transmitCVR( uint8_t chipAddress, char cellVoltageRegister, uint8_t *data ) //data: Unparsed cellVoltage_counts
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

  wakeup_isoSPI(); //Guarantees LTC6804 isoSPI port is awake.
  digitalWrite(PIN_SPI_CS,LOW);
  spi_write_read(cmd,4,&data[0],8);
  digitalWrite(PIN_SPI_CS,HIGH);
}

//---------------------------------------------------------------------------------------

void printCellVoltage_all()
{ // VERY SLOW & blocking (serial transmit buffer filled)
  // Use only for debug purposes
  Serial.print('\n');

  //Returns 0           when reading from bottom half of cellVoltages array
  //Returns CELL_PER_IC when reading from top    half of cellVoltages array
  uint8_t frameOffset = LTC6804_getFrameOffset_Read();

  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F("IC "));
    Serial.print( (current_ic + FIRST_IC_ADDR) ,DEC);
    for (int i=0; i<(CELLS_PER_IC); i++)
    {
      const uint8_t NUM_DECIMAL_PLACES = 4; //JTS2doNow: change back to 4 digits
      Serial.print(F(" C"));
      Serial.print(i+1,DEC); //Note: cell voltages always reported back C1:C12 (not C13:C24)
      Serial.print(':');
      Serial.print( (cellVoltages_counts[current_ic][i + frameOffset] * 0.0001), NUM_DECIMAL_PLACES ); 
    }
    Serial.print('\n');
  }
}

//---------------------------------------------------------------------------------------

// |command    |  10   |   9   |   8   |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
// |-----------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
// |ADCV:      |   0   |   1   | MD[1] | MD[2] |   1   |   1   |  DCP  |   0   | CH[2] | CH[1] | CH[0] |
// |ADAX:      |   1   |   0   | MD[1] | MD[2] |   1   |   1   |  DCP  |   0   | CHG[2]| CHG[1]| CHG[0]|

//JTS2doLater: Change corner frequency to reduce noise
//ADC LPF Fcorner:       Total conversion time (QTY12 cells/IC)
//ADCOPT(CFGR0[0] = 0)    
// MD = 01 27000 Hz        1.2 ms
// MD = 10  7000 Hz        2.5 ms (default)
// MD = 11    26 Hz      213.5 ms
//
//ADCOPT(CFGR0[0] = 1)
// MD = 01 14000 Hz        1.3 ms
// MD = 10  3000 Hz        3.0 ms
// MD = 11  2000 Hz        4.4 ms
void setADC_cells(uint8_t adcFilterMode, //ADC Conversion Mode 'MD' (LPF corner frequency)
             uint8_t isDischargeAllowedWhileDigitizing, //Discharge permitted during cell conversions? 'DCP'
             uint8_t cellChannelsToDigitize ) //Cell Channels 'CELL_CH' to measure during ADC conversion command        
{
  uint8_t md_temp;

  //JTS2doLater: Replace magic numbers with #defines
  md_temp = (adcFilterMode & 0x02) >> 1; //set bit 8 (MD[1]) in ADCV[0]... MD[0] is in ADCV[1]
  ADCV[0] = md_temp + 0x02;  //set bit 9 true

  md_temp = (adcFilterMode & 0x01) << 7;
  ADCV[1] = md_temp + 0x60 + (isDischargeAllowedWhileDigitizing<<4) + cellChannelsToDigitize;
}

//---------------------------------------------------------------------------------------

void wakeup_isoSPI()
{
  const uint8_t T_IDLE_isoSPI_millis = 4; //'tidle' (absMIN) = 4.3 ms

  if( (millis() - lastTimeDataSent_millis) > T_IDLE_isoSPI_millis )
  { //LTC6804 isoSPI might be asleep
    digitalWrite(PIN_SPI_CS,LOW);
    delayMicroseconds(10); //Guarantees isoSPI is in ready mode
    digitalWrite(PIN_SPI_CS,HIGH);
  }
}

//---------------------------------------------------------------------------------------

//wake up LTC ICs after watchdog timeout
void wakeup_sleep()
{
  const uint16_t T_SLEEP_WATCHDOG_MILLIS = 1800; //'tsleep' (absMIN) = 1800 ms 

  if( (millis() - lastTimeDataSent_millis) > T_SLEEP_WATCHDOG_MILLIS )
  { //LTC6804 core might be asleep
    digitalWrite(PIN_SPI_CS,LOW);
    delayMicroseconds(300); // Guarantees the LTC6804 is in standby (tWake) //JTS2doNow: Do we need to wait tRefup (4400 us)?  
    digitalWrite(PIN_SPI_CS,HIGH);
  }
}

//---------------------------------------------------------------------------------------

uint16_t pec15_calc(uint8_t len, //data array length
                    uint8_t *data) //data array to generate PEC from
{
  uint16_t remainder,addr;

  remainder = 16;//initialize the PEC
  for (uint8_t i = 0; i<len; i++) // loops for each byte in data array
  {
    addr = ( (remainder>>7)^data[i] ) & 0xff;//calculate PEC table address
    remainder = (remainder<<8) ^ crc15Table[addr];
  }
  return(remainder<<1);//The CRC15 LSB is 0, so multiply by 2
}

//---------------------------------------------------------------------------------------

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

void spi_write_read(uint8_t tx_Data[],//array of data to be written on SPI port
                    uint8_t tx_len, //length of the tx data arry
                    uint8_t *rx_data,//Input: array that will store the data read by the SPI port
                    uint8_t rx_len //Option: number of bytes to be read from the SPI port
                   )
{
  for (uint8_t i = 0; i < tx_len; i++) { spi_write(tx_Data[i]); }
  for (uint8_t i = 0; i < rx_len; i++) { rx_data[i] = (uint8_t)spi_read(0xFF); }
}

//---------------------------------------------------------------------------------------

void LTC6804_handleKeyOff(void)
{
  isoSPI_errorCount = 0;
  maxEverCellVoltage = 0;
  minEverCellVoltage = 65535; 
  cellVoltages_lastCompleteFrame = COMPLETE_FRAME_IS_NONE; //JTS2doNow: necessary to reinit? 
}