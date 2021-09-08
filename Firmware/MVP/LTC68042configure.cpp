//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LTC6804 configuration functions

#include "libcm.h"

uint32_t lastTimeDataSent_millis = 0; //LTC idle timer resets each time data is transferred

//---------------------------------------------------------------------------------------

//Stores configuration register data to write to IC
//tx_cfg[n][0] = CFGR0
//tx_cfg[n][1] = CFGR1
//tx_cfg[n][2] = CFGR2
//tx_cfg[n][3] = CFGR3
//tx_cfg[n][4] = CFGR4
//tx_cfg[n][5] = CFGR5
uint8_t tx_cfg[TOTAL_IC][6];

//---------------------------------------------------------------------------------------

//Initialize the configuration array
//JTS2doNow: This doesn't need to be a 2D array.  keyOn data identical on all ICs.
//resets if LTC watchdog timer expires (i.e. when LTC LEDs turn off (after ~2s))
void LTC6804configure_keyOn_configurationRegisterValues()
{
  for (int i = 0; i<TOTAL_IC; i++)
  {                              // BIT7    BIT6    BIT5    BIT4    BIT3    BIT2    BIT1   BIT0
                                 ///////////////////////////////////////////////////////////////
    tx_cfg[i][0] = 0b11111111 ;  //GPIO5   GPIO4   GPIO3   GPIO2   GPIO1   REFON   SWTRD  ADCOPT //same for all ICs
    tx_cfg[i][1] = 0x00       ;  //VUV[7]  VUV[6]  VUV[5]  VUV[4]  VUV[3]  VUV[2]  VUV[1] VUV[0] //same for all ICs
    tx_cfg[i][2] = 0x00       ;  //VOV[3]  VOV[2]  VOV[1]  VOV[0]  VUV[11] VUV[10] VUV[9] VUV[8] //same for all ICs
    tx_cfg[i][3] = 0x00       ;  //VOV[11] VOV[10] VOV[9]  VOV[8]  VOV[7]  VOV[6]  VOV[5] VOV[4] //same for all ICs
    tx_cfg[i][4] = 0xFF       ;  //DCC8    DCC7    DCC6    DCC5    DCC4    DCC3    DCC2   DCC1
    tx_cfg[i][5] = 0x0F       ;  //DCTO[3] DCTO[2] DCTO[1] DCTO[0] DCC12   DCC11   DCC10  DCC9
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

//JTS2doNow: Only write one LTC6804's configuration registers
//Write each LTC6804's configuration registers
//written in descending order (last device's configuration is written first)

//uint8_t *config is configuration data array to write. 6 bytes for each IC
//The lowest IC in the pack should be the first 6 byte block in the array. Array format:
// |  config[0]| config[1] |  config[2]|  config[3]|  config[4]|  config[5]| config[6] |  config[7]|  config[8]|  .....    |
// |-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|
// |IC1 CFGR0  |IC1 CFGR1  |IC1 CFGR2  |IC1 CFGR3  |IC1 CFGR4  |IC1 CFGR5  |IC2 CFGR0  |IC2 CFGR1  | IC2 CFGR2 |  .....    |

void LTC6804_wrcfg(uint8_t total_ic, uint8_t addr_first_ic)
{
  const uint8_t BYTES_IN_REG = 6;
  const uint8_t CMD_LEN = 4+(8*total_ic);
  uint8_t *cmd;
  uint16_t temp_pec;
  uint8_t cmd_index; //command counter

  cmd = (uint8_t *)malloc(CMD_LEN*sizeof(uint8_t));

  //Load cmd array with the write configuration command and PEC
  cmd[0] = 0x00;
  cmd[1] = 0x01;
  cmd[2] = 0x3d; //PEC
  cmd[3] = 0x6e; //PEC

  //Load the cmd with LTC6804 configuration data
  cmd_index = 4;
  for (uint8_t current_ic = 0; current_ic < total_ic ; current_ic++)       // executes for each LTC6804 in pack,
  {
    for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) // executes for each byte in the CFGR register
    {
      cmd[cmd_index] = tx_cfg[current_ic][current_byte];    //adding the config data to the array to be sent
      cmd_index = cmd_index + 1;
    }
    //Calculate the pec for the LTC6804 configuration data being transmitted
    temp_pec = (uint16_t)LTC68042configure_calcPEC15(BYTES_IN_REG, & tx_cfg[current_ic][0]);// calculating the PEC for each IC
    cmd[cmd_index] = (uint8_t)(temp_pec >> 8); //upper PEC byte
    cmd[cmd_index + 1] = (uint8_t)temp_pec; //lower PEC byte
    cmd_index = cmd_index + 2;
  }

  LTC68042configure_wakeupIsoSPI();

  //Write configuration of each LTC6804 on the pack
  for (int current_ic = 0; current_ic<total_ic; current_ic++)
  {
    cmd[0] = 0x80 + ( (current_ic + addr_first_ic) << 3); //Setting address
    temp_pec = LTC68042configure_calcPEC15(2, cmd);
    cmd[2] = (uint8_t)(temp_pec >> 8);
    cmd[3] = (uint8_t)(temp_pec);
    digitalWrite(PIN_SPI_CS,LOW);
    LTC68042configure_spiWrite(4,cmd);
    LTC68042configure_spiWrite(8,&cmd[4+(8*current_ic)]);
    digitalWrite(PIN_SPI_CS,HIGH);
  }
  free(cmd);
}

//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------

void LTC68042configure_initialize()
{
  spi_enable(SPI_CLOCK_DIV64); //JTS2doLater: See how fast we can use SPI without transmission errors
  //LTC6804configure_calculate_CFGRn();        //JTS2doNow: Doesn't actually send configuration to LTC ICs.
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