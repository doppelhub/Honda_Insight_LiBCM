//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LTC6804 configuration functions

#include "libcm.h"

uint32_t lastTimeDataSent_millis = 0; //LTC idle timer resets each time data is transferred

//---------------------------------------------------------------------------------------

//Stores configuration register data to write to IC
uint8_t configurationRegisterData[6]; //[CFGR0, CFGR1, CFGR2, CFGR3, CFGR4, CFGR5]

//---------------------------------------------------------------------------------------

//Write one LTC6804's configuration registers
// | config[0] | config[1] | config[2] | config[3] | config[4] | config[5] |
// |-----------|-----------|-----------|-----------|-----------|-----------|
// | IC CFGR0  | IC CFGR1  | IC CFGR2  | IC CFGR3  | IC CFGR4  | IC CFGR5  |

void LTC68042configure_writeConfigurationRegisters(uint8_t icAddress)
{
  LTC68042configure_wakeupCore();

  const uint8_t BYTES_IN_REG = 6;
  const uint8_t CMD_LEN = 4+6+2;
  uint8_t *cmd;

  //JTS2doNow: Replace malloc with array init
  cmd = (uint8_t *)malloc(CMD_LEN*sizeof(uint8_t));

  //Load cmd array with the write configuration command and PEC
  cmd[0] = 0x00;
  cmd[1] = 0x01;
  cmd[2] = 0x3d; //PEC
  cmd[3] = 0x6e; //PEC

  //Load the cmd with LTC6804 configuration data
  uint8_t cmd_index = 4; //number of bytes to send

  for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++)
  {
    cmd[cmd_index] = configurationRegisterData[current_byte];    //add the config data byte to the array to send
    cmd_index++;
  }

  //Calculate the pec for the LTC6804 configuration data being transmitted
  uint16_t temp_pec = (uint16_t)LTC68042configure_calcPEC15(BYTES_IN_REG, &configurationRegisterData[0]);// calculating the PEC for each IC
  cmd[cmd_index] = (uint8_t)(temp_pec >> 8); //upper PEC byte
  cmd[cmd_index + 1] = (uint8_t)temp_pec; //lower PEC byte
  cmd_index = cmd_index + 2;

  LTC68042configure_wakeupIsoSPI();

  //Write configuration data to LTC6804
  cmd[0] = 0x80 + (icAddress << 3); //set address
  temp_pec = LTC68042configure_calcPEC15(2, cmd);
  cmd[2] = (uint8_t)(temp_pec >> 8);
  cmd[3] = (uint8_t)(temp_pec);
  digitalWrite(PIN_SPI_CS,LOW);
  LTC68042configure_spiWrite(4,cmd);
  LTC68042configure_spiWrite(8,&cmd[4]);
  digitalWrite(PIN_SPI_CS,HIGH);

  free(cmd);
}

//---------------------------------------------------------------------------------------

void LTC68042configure_cellBalancing_setCells(uint8_t icAddress, uint16_t cellBitmap)
{
  //Each bit in cellBitmap corresponds to a cell's DCCn discharge bit
  //Example: cellBitmap = 0b0000 1000 0000 0011 will enable discharge on cells 12,2,1 //LSB is cell01
  //Example: cellBitmap = 0b0000 1111 1111 1111 will enable discharge on all cells
  configurationRegisterData[4] = (uint8_t)(cellBitmap); //LSByte
  configurationRegisterData[5] = (( (uint8_t)(cellBitmap >> 8) ) & 0b00001111); //MSByte's lower nibble

  LTC68042configure_writeConfigurationRegisters(icAddress);
}

//---------------------------------------------------------------------------------------

//Turn off discharge FETs, turn reference on and configure ADC LPF to '2 kHz mode' (1.7 kHz LPF)
//configuration register data resets if LTC watchdog timer expires (~2000 milliseconds)
void LTC68042configure_cellBalancing_disable()
{                                              // BIT7    BIT6    BIT5    BIT4    BIT3    BIT2    BIT1   BIT0                
                                               ///////////////////////////////////////////////////////////////
  configurationRegisterData[0] = 0b11111111 ;  //GPIO5   GPIO4   GPIO3   GPIO2   GPIO1   REFON   SWTRD  ADCOPT
  configurationRegisterData[1] = 0x00       ;  //VUV[7]  VUV[6]  VUV[5]  VUV[4]  VUV[3]  VUV[2]  VUV[1] VUV[0]
  configurationRegisterData[2] = 0x00       ;  //VOV[3]  VOV[2]  VOV[1]  VOV[0]  VUV[11] VUV[10] VUV[9] VUV[8]
  configurationRegisterData[3] = 0x00       ;  //VOV[11] VOV[10] VOV[9]  VOV[8]  VOV[7]  VOV[6]  VOV[5] VOV[4]
  configurationRegisterData[4] = 0x00       ;  //DCC8    DCC7    DCC6    DCC5    DCC4    DCC3    DCC2   DCC1
  configurationRegisterData[5] = 0x00       ;  //DCTO[3] DCTO[2] DCTO[1] DCTO[0] DCC12   DCC11   DCC10  DCC9

  //t=2.2 milliseconds (ICs initially off)
  for(int ii = FIRST_IC_ADDR; ii < (FIRST_IC_ADDR + TOTAL_IC); ii++)
  {
    LTC68042configure_writeConfigurationRegisters(ii); //t=450 us
  }
}
//see p51 for more info:
//DCTO  = set discharge timer
//DCC   = control cell discharge FET (1=on)
//VUV   = undervoltage comparison voltage ((VUV+1) * 16 * 100uV)
//VOV   = over voltage comparison voltage ((VUV  ) * 16 * 100uV)
//GPIO  = read to get pinState, write 0/1 to enable/disable pull-down (disabled by default)
//REFON = keep ADC reference powered whenever IC awake (reduces ADC delay)
//SWTRD = (read only) is SWTEN (PIN_SOFTWARE_TIMER_ENABLE) high or low (high=SW timer allowed)
//ADCOPT= sets adc fast/normal/slow LPF cutoff frequency values (0: 27k/7k/26 Hz)(1: 14k/3k/2k Hz)
            //Note: fast, normal, or slow is configured in ADCV command

//---------------------------------------------------------------------------------------

void LTC68042configure_initialize()
{
  spi_enable(SPI_CLOCK_DIV64); //JTS2doLater: See how fast we can use SPI without transmission errors
  //LTC6804configure_calculate_CFGRn();
  Serial.print(F("\nLTC6804 BEGIN"));
}

//---------------------------------------------------------------------------------------

void LTC68042configure_wakeupIsoSPI()
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
void LTC68042configure_wakeupCore()
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

uint16_t LTC68042configure_calcPEC15(uint8_t len, //data array length
                                     uint8_t *data ) //data array to generate PEC from
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

void LTC68042configure_spiWrite(uint8_t len, // bytes to be written on the SPI port
                                uint8_t data[] )//array of bytes to be written on the SPI port
{
  for (uint8_t i = 0; i < len; i++)
  {
    spi_write((char)data[i]);
  }

  lastTimeDataSent_millis = millis(); //all SPI writes occur here
}

//---------------------------------------------------------------------------------------

void LTC68042configure_spiWriteRead(uint8_t tx_Data[],//array of data to be written on SPI port
                    uint8_t tx_len, //length of the tx data arry
                    uint8_t *rx_data,//Input: array that will store the data read by the SPI port
                    uint8_t rx_len )//Option: number of bytes to be read from the SPI port
{
  for (uint8_t i = 0; i < tx_len; i++) { spi_write(tx_Data[i]); }
  for (uint8_t i = 0; i < rx_len; i++) { rx_data[i] = (uint8_t)spi_read(0xFF); }
}

//---------------------------------------------------------------------------------------

void LTC6804configure_handleKeyOff(void)
{
  LTC68042result_errorCount_set(0);
  LTC68042result_maxEverCellVoltage_set(0);
  LTC68042result_minEverCellVoltage_set(65535); 
}