/*!
DC1942B
LTC6804-2: Battery stack monitor

@verbatim
NOTES
 Setup:
   Set the terminal baud rate to 115200 and select the newline terminator.
   Ensure all jumpers on the demo board are installed in their default positions from the factory.
   Refer to Demo Manual D1894B.


 Menu Entry 1: Write Configuration
   Writes the configuration register of the LTC6804. This command can be used to turn on the reference.

 Menu Entry 2: Read Configuration
   Reads the configuration register of the LTC6804, the read configuration can differ from the written configuration.
   The GPIO pins will reflect the state of the pin

 Menu Entry 3: Start Cell voltage conversion
    Starts a LTC6804 cell channel adc conversion.

 Menu Entry 4: Read cell voltages
    Reads the LTC6804 cell voltage registers and prints the results to the serial port.

 Menu Entry 5: Start Auxiliary voltage conversion
    Starts a LTC6804 GPIO channel adc conversion.

 Menu Entry 6: Read Auxiliary voltages
    Reads the LTC6804 axiliary registers and prints the GPIO voltages to the serial port.

 Menu Entry 7: Start cell voltage measurement loop
    The command will continuously measure the LTC6804 cell voltages and print the results to the serial port.
    The loop can be exited by sending the MCU a 'm' character over the serial link.

USER INPUT DATA FORMAT:
 decimal : 1024
 hex     : 0x400
 octal   : 02000  (leading 0)
 binary  : B10000000000
 float   : 1024.0

@endverbatim

http://www.linear.com/product/LTC6804-1

http://www.linear.com/product/LTC6804-1#demoboards


Copyright 2018(c) Analog Devices, Inc.

All rights reserved.

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

Copyright 2013 Linear Technology Corp. (LTC)
 */


/*! @file
    @ingroup LTC68042
*/

#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h"
#include "LT_SPI.h"
#include "UserInterface.h"
#include "LTC68042.h"
#include <SPI.h>

const uint8_t TOTAL_IC = 4;//!<number of ICs in the isoSPI network LTC6804-2 ICs must be addressed in ascending order starting at 0.
const uint8_t FIRST_IC_ADDR = 2; //!<lowest address.  All additional ICs must be sequentially numbered.

/******************************************************
 *** Global Battery Variables received from 6804 commands
 These variables store the results from the LTC6804
 register reads and the array lengths must be based
 on the number of ICs on the stack
 ******************************************************/
uint16_t cell_codes[TOTAL_IC][12];
/*!<
  The cell codes will be stored in the cell_codes[][12] array in the following format:

  |  cell_codes[0][0]| cell_codes[0][1] |  cell_codes[0][2]|    .....     |  cell_codes[0][11]|  cell_codes[1][0] | cell_codes[1][1]|  .....   |
  |------------------|------------------|------------------|--------------|-------------------|-------------------|-----------------|----------|
  |IC1 Cell 1        |IC1 Cell 2        |IC1 Cell 3        |    .....     |  IC1 Cell 12      |IC2 Cell 1         |IC2 Cell 2       | .....    |
****/

uint16_t aux_codes[TOTAL_IC][6];
/*!<
 The GPIO codes will be stored in the aux_codes[][6] array in the following format:

 |  aux_codes[0][0]| aux_codes[0][1] |  aux_codes[0][2]|  aux_codes[0][3]|  aux_codes[0][4]|  aux_codes[0][5]| aux_codes[1][0] |aux_codes[1][1]|  .....    |
 |-----------------|-----------------|-----------------|-----------------|-----------------|-----------------|-----------------|---------------|-----------|
 |IC1 GPIO1        |IC1 GPIO2        |IC1 GPIO3        |IC1 GPIO4        |IC1 GPIO5        |IC1 Vref2        |IC2 GPIO1        |IC2 GPIO2      |  .....    |
*/

uint8_t tx_cfg[TOTAL_IC][6];
/*!<
  tx_cfg[][6] stores the LTC6804 configuration data to be written
  to the LTC6804 ICs. The 6 byte array should have the following format:

 |  tx_cfg[0][0]| tx_cfg[0][1] |  tx_cfg[0][2]|  tx_cfg[0][3]|  tx_cfg[0][4]|  tx_cfg[0][5]|
 |--------------|--------------|--------------|--------------|--------------|--------------|
 |IC1 CFGR0     |IC1 CFGR1     |IC1 CFGR2     |IC1 CFGR3     |IC1 CFGR4     |IC1 CFGR5     |

*/

uint8_t rx_cfg[TOTAL_IC][8];
/*!<
  the rx_cfg[][8] array stores the 8 byte configuration data read back from the LTC6804:

|rx_config[0][0]|rx_config[0][1]|rx_config[0][2]|rx_config[0][3]|rx_config[0][4]|rx_config[0][5]|rx_config[0][6]  |rx_config[0][7] |
|---------------|---------------|---------------|---------------|---------------|---------------|-----------------|----------------|
|IC1 CFGR0      |IC1 CFGR1      |IC1 CFGR2      |IC1 CFGR3      |IC1 CFGR4      |IC1 CFGR5      |IC1 PEC High     |IC1 PEC Low     |
*/

//JTS Move to hardware IO file
#define BATT_CURRENT_PIN  A0
#define FANOEM_LO_PIN     A1
#define FANOEM_HI_PIN     A2
#define TEMP_YEL_PIN      A3
#define TEMP_GRN_PIN      A4
#define TEMP_WHT_PIN      A5
#define TEMP_BLU_PIN      A6
#define VPIN_IN_PIN       A7
#define TURNOFF_LIBCM_PIN A8
#define DEBUG_FET_PIN     A9
#define DEBUG_IO1_PIN    A10
#define DEBUG_IO2_PIN    A11
#define LED1_PIN         A12
#define LED2_PIN         A13
#define LED3_PIN         A14
#define LED4_PIN         A15

#define BATTSCI_DIR_PIN 2
#define METSCI_DIR_PIN  3
#define VPIN_OUT_PIN    4
#define SPI_EXT_CS_PIN  5 
#define TEMP_EN_PIN     6
#define CONNE_PWM_PIN   7
#define GRIDPWM_PIN     8
#define GRIDSENSE_PIN   9
#define GRIDEN_PIN     10
#define FAN_PWM_PIN    11
#define LOAD5V_PIN     12
#define KEY_ON_PIN     13

//Serial3
#define HLINE_TX_PIN 14
#define HLINE_RX_PIN 15

//Serial2
#define METSCI_TX_PIN 16
#define METSCI_RX_PIN 17

//Serial1
#define BATTSCI_TX_PIN 18
#define BATTSCI_RX_PIN 19

#define DEBUG_SDA_PIN 20
#define DEBUG_CLK_PIN 21


/*!**********************************************************************
 \brief  Inititializes hardware and variables
 ***********************************************************************/
void setup()
{
  Serial.begin(115200);
  LTC6804_initialize();  //Initialize LTC6804 hardware
  init_cfg();        //initialize the 6804 configuration array to be written
  print_menu();

  pinMode(GRIDPWM_PIN,  OUTPUT); //JTS Move this to init code
  pinMode(GRIDSENSE_PIN, INPUT);
  pinMode(GRIDEN_PIN,   OUTPUT);
  pinMode(FAN_PWM_PIN,  OUTPUT);
}

/*!*********************************************************************
  \brief main loop

***********************************************************************/
void loop()
{

  if (Serial.available())           // Check for user input
  {
    uint8_t user_command;
    user_command = read_int();      // Read the user command
    Serial.println(user_command);
    run_command(user_command);
  }
}


/*!*****************************************
  \brief executes the user inputted command

  Menu Entry 1: Write Configuration \n
   Writes the configuration register of the LTC6804. This command can be used to turn on the reference
   and increase the speed of the ADC conversions.

 Menu Entry 2: Read Configuration \n
   Reads the configuration register of the LTC6804, the read configuration can differ from the written configuration.
   The GPIO pins will reflect the state of the pin

 Menu Entry 3: Start Cell voltage conversion \n
   Starts a LTC6804 cell channel adc conversion.

 Menu Entry 4: Read cell voltages
    Reads the LTC6804 cell voltage registers and prints the results to the serial port.

 Menu Entry 5: Start Auxiliary voltage conversion
    Starts a LTC6804 GPIO channel adc conversion.

 Menu Entry 6: Read Auxiliary voltages
    Reads the LTC6804 axiliary registers and prints the GPIO voltages to the serial port.

 Menu Entry 7: Start cell voltage measurement loop
    The command will continuously measure the LTC6804 cell voltages and print the results to the serial port.
    The loop can be exited by sending the MCU a 'm' character over the serial link.

*******************************************/
void run_command(uint8_t cmd)
{
  int8_t error = 0;

  char input = 0;

  switch (cmd)
  {
    case 1:
      wakeup_sleep();
      LTC6804_wrcfg(TOTAL_IC,tx_cfg,FIRST_IC_ADDR);
      print_config();
      break;

    case 2:
      wakeup_sleep();
      error = LTC6804_rdcfg(TOTAL_IC,rx_cfg,FIRST_IC_ADDR);
      if (error == -1)
      {
        Serial.println(F("A PEC error was detected in the received data"));
      }
      print_rxconfig();
      break;

    case 3:
      wakeup_sleep();
      LTC6804_adcv();
      delay(3);
      Serial.println(F("cell conversion completed"));
      Serial.println();
      break;

    case 4:
      wakeup_sleep();
      error = LTC6804_rdcv(0, TOTAL_IC,cell_codes,FIRST_IC_ADDR); // Set to read back all cell voltage registers
      if (error == -1)
      {
        Serial.println(F("A PEC error was detected in the received data"));
      }
      print_cells();
      break;

    case 5:
      wakeup_sleep();
      LTC6804_adax();
      delay(3);
      Serial.println(F("aux conversion completed"));
      Serial.println();
      break;

    case 6:
      wakeup_sleep();
      error = LTC6804_rdaux(0,TOTAL_IC,aux_codes,FIRST_IC_ADDR); // Set to read back all aux registers
      if (error == -1)
      {
        Serial.println(F("A PEC error was detected in the received data"));
      }
      print_aux();
      break;

    case 7:
      Serial.println(F("transmit 'm' to quit"));
      wakeup_sleep();
      LTC6804_wrcfg(TOTAL_IC,tx_cfg,FIRST_IC_ADDR);
      while (input != 'm')
      {
        if (Serial.available() > 0)
        {
          input = read_char();
        }
        wakeup_idle();
        LTC6804_adcv();
        delay(10);
        wakeup_idle();
        error = LTC6804_rdcv(0, TOTAL_IC,cell_codes,FIRST_IC_ADDR);
        if (error == -1)
        {
          Serial.println(F("A PEC error was detected in the received data"));
        }
        print_cells();
        
        //check for full cell(s)   
        uint16_t cellVoltage_highest = 0;   
        for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
          {
            for (int i=0; i<12; i++)
            {
              if( cell_codes[current_ic][i] > cellVoltage_highest )
              {
                cellVoltage_highest = cell_codes[current_ic][i];
              }
              if( cell_codes[current_ic][i] > 39500 ) //10 mV per count, e.g. "39000" = 3.900 volts
              {
                Serial.println("Cell " + String(current_ic * 12 + i + 1) + " is full." );
                turnGridCharger_Off();
                //tx_cfg[current_ic][i]
              }
            }
          }
        Serial.println("Highest cell voltage is: " + String(cellVoltage_highest , DEC) );
        delay(500);
      }
      print_menu();
      break;

    case 8: //JTS toggle grid charger
    {
      uint8_t gridEnabled      =   digitalRead(GRIDEN_PIN    );
      uint8_t gridPowerPresent = !(digitalRead(GRIDSENSE_PIN));

      Serial.println("GRIDEN   : " + String(gridEnabled     ) );
      Serial.println("GRIDSENSE: " + String(gridPowerPresent) );
  
      if( gridEnabled || !(gridPowerPresent) ) //charger is enabled, or unplugged
      {
        turnGridCharger_Off();
      }
      if( !(gridEnabled) && gridPowerPresent ) //charger off & plugged in 
      {
        digitalWrite(GRIDEN_PIN,HIGH); //Turn charger on
        delay(100);
        for(int ii=255; ii>=0; ii--)
        {
          analogWrite(GRIDPWM_PIN,ii);
          delay(2);
        }
        Serial.println(F("Charger turned on"));
      }
      break;
    }

    case 9: //JTS Toggle Fans
    {
      Serial.println(F("Toggling Fans")); 
      if( digitalRead(FAN_PWM_PIN) ) //fans are on
      {
        for(int ii=255; ii>=50; ii--)
        {
          analogWrite(FAN_PWM_PIN,ii);
          delay(10);
        }
        analogWrite(FAN_PWM_PIN,0);
        Serial.println(F("Fans turned off"));
      } else { //fans are off
        for(int ii=75; ii<=255; ii++)
        {
          analogWrite(FAN_PWM_PIN,ii);
          delay(10);
        }
        Serial.println(F("Fans turned on"));
      }
      break;
    }

    default:
      Serial.println(F("Incorrect Option"));
      break;
  }
}

/*!***********************************
 \brief Initializes the configuration array
 **************************************/
//JTS2do: This doesn't need to be a 2D array.  Data identical on all LTC, except DCC12:1.
void init_cfg()
{
  for (int i = 0; i<TOTAL_IC; i++)
  {                        // BIT7    BIT6    BIT5    BIT4    BIT3    BIT2    BIT1   BIT0 
    tx_cfg[i][0] = 0xF6 ;  //GPIO5   GPIO4   GPIO3   GPIO2   GPIO1   REFON   SWTRD  ADCOPT //Enable GPIO pulldown, etc
    tx_cfg[i][1] = 0x00 ;  //VUV[7]  VUV[6]  VUV[5]  VUV[4]  VUV[3]  VUV[2]  VUV[1] VUV[0] //Undervoltage comparison voltage
    tx_cfg[i][2] = 0x00 ;  //VOV[3]  VOV[2]  VOV[1]  VOV[0]  VUV[11] VUV[10] VUV[9] VUV[8] 
    tx_cfg[i][3] = 0x00 ;  //VOV[11] VOV[10] VOV[9]  VOV[8]  VOV[7]  VOV[6]  VOV[5] VOV[4] //Overvoltage comparison voltage 
    tx_cfg[i][4] = 0x00 ;  //DCC8    DCC7    DCC6    DCC5    DCC4    DCC3    DCC2   DCC1   //Enables discharge on cells 8:1
    tx_cfg[i][5] = 0x00 ;  //DCTO[3] DCTO[2] DCTO[1] DCTO[0] DCC12   DCC11   DCC10  DCC9   //Discharge timer and cells 12:9
  }
}

/*!*********************************
  \brief Prints the main menu
***********************************/
void print_menu()
{
  Serial.println(F("Please enter LTC6804 Command"));
  Serial.println(F("Write Configuration:           1"));
  Serial.println(F("Read Configuration:            2"));
  Serial.println(F("Start Cell Voltage Conversion: 3"));
  Serial.println(F("Read Cell Voltages:            4"));
  Serial.println(F("Start Aux Voltage Conversion:  5"));
  Serial.println(F("Read Aux Voltages:             6"));
  Serial.println(F("loop cell voltages:            7"));
  Serial.println(F("Toggle Grid Charger:           8"));
  Serial.println(F("Toggle Fans:                   9"));
  Serial.println(F("Please enter command: "));
  Serial.println();
}



/*!************************************************************
  \brief Prints Cell Voltage Codes to the serial port
 *************************************************************/
void print_cells()
{


  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print( (current_ic + FIRST_IC_ADDR) ,DEC);
    for (int i=0; i<12; i++)
    {
      Serial.print(F(" C"));
      Serial.print(i+1,DEC);
      Serial.print(F(":"));
      Serial.print(cell_codes[current_ic][i]*0.0001,4);
      Serial.print(",");
    }
    Serial.println();
  }
  Serial.println();
}

/*!****************************************************************************
  \brief Prints GPIO Voltage Codes and Vref2 Voltage Code onto the serial port
 *****************************************************************************/
void print_aux()
{

  for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print( (current_ic + FIRST_IC_ADDR) ,DEC);
    for (int i=0; i < 5; i++)
    {
      Serial.print(F(" GPIO-"));
      Serial.print(i+1,DEC);
      Serial.print(F(":"));
      Serial.print(aux_codes[current_ic][i]*0.0001,4);
      Serial.print(",");
    }
    Serial.print(F(" Vref2"));
    Serial.print(F(":"));
    Serial.print(aux_codes[current_ic][5]*0.0001,4);
    Serial.println();
  }
  Serial.println();
}
/*!******************************************************************************
 \brief Prints the Configuration data that is going to be written to the LTC6804
 to the serial port.
 ********************************************************************************/
void print_config()
{
  int cfg_pec;

  Serial.println(F("Written Configuration: "));
  for (int current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print( (current_ic + FIRST_IC_ADDR) ,DEC);
    Serial.print(": ");
    Serial.print(F("0x"));
    serial_print_hex(tx_cfg[current_ic][0]);
    Serial.print(F(", 0x"));
    serial_print_hex(tx_cfg[current_ic][1]);
    Serial.print(F(", 0x"));
    serial_print_hex(tx_cfg[current_ic][2]);
    Serial.print(F(", 0x"));
    serial_print_hex(tx_cfg[current_ic][3]);
    Serial.print(F(", 0x"));
    serial_print_hex(tx_cfg[current_ic][4]);
    Serial.print(F(", 0x"));
    serial_print_hex(tx_cfg[current_ic][5]);
    Serial.print(F(", Calculated PEC: 0x"));
    cfg_pec = pec15_calc(6,&tx_cfg[current_ic][0]);
    serial_print_hex((uint8_t)(cfg_pec>>8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(cfg_pec));
    Serial.println();
  }
  Serial.println();
}

/*!*****************************************************************
 \brief Prints the Configuration data that was read back from the
 LTC6804 to the serial port.
 *******************************************************************/
void print_rxconfig()
{
  Serial.println(F("Received Configuration "));
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print( (current_ic + FIRST_IC_ADDR) ,DEC);
    Serial.print(F(": 0x"));
    serial_print_hex(rx_cfg[current_ic][0]);
    Serial.print(F(", 0x"));
    serial_print_hex(rx_cfg[current_ic][1]);
    Serial.print(F(", 0x"));
    serial_print_hex(rx_cfg[current_ic][2]);
    Serial.print(F(", 0x"));
    serial_print_hex(rx_cfg[current_ic][3]);
    Serial.print(F(", 0x"));
    serial_print_hex(rx_cfg[current_ic][4]);
    Serial.print(F(", 0x"));
    serial_print_hex(rx_cfg[current_ic][5]);
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(rx_cfg[current_ic][6]);
    Serial.print(F(", 0x"));
    serial_print_hex(rx_cfg[current_ic][7]);
    Serial.println();
  }
  Serial.println();
}

void serial_print_hex(uint8_t data)
{
  if (data< 16)
  {
    Serial.print(F("0"));
    Serial.print((byte)data,HEX);
  }
  else
    Serial.print((byte)data,HEX);
}

void turnGridCharger_Off(void)
{
  analogWrite(GRIDPWM_PIN,255); //Set power to zero
  delay(100);
  digitalWrite(GRIDEN_PIN,LOW); //Turn charger off
  Serial.println(F("Charger turned off"));
}
