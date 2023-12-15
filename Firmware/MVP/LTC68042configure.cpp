//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LTC6804 configuration functions

//JTS2doLater: Add 2nd reference measurement (to verify 1st reference is working properly)

#include "libcm.h"

uint32_t lastTimeDataSent_millis = 0; //LTC idle timer resets each time data is transferred

//---------------------------------------------------------------------------------------

//Stores configuration register data to write to IC
uint8_t configurationRegisterData[6]; //[CFGR0, CFGR1, CFGR2, CFGR3, CFGR4, CFGR5]

//---------------------------------------------------------------------------------------

//Write LTC6804 configuration registers
//if(icAddress == BROADCAST_TO_ALL_ICS), this function broadcasts the same data to all LTC6804 ICs 
//
// | config[0] | config[1] | config[2] | config[3] | config[4] | config[5] |
// |-----------|-----------|-----------|-----------|-----------|-----------|
// | IC CFGR0  | IC CFGR1  | IC CFGR2  | IC CFGR3  | IC CFGR4  | IC CFGR5  |

void LTC68042configure_writeConfigRegisters(uint8_t icAddress)
{
  const uint8_t BYTES_IN_REG = 6;
  const uint8_t CMD_LENGTH = 2+2+6+2; //("Write Configuration Registers" command) + (PEC) + ("configuration register" data) + (PEC)
  uint8_t cmd[CMD_LENGTH];

  //Load cmd array with WRCFG command and PEC
  if(icAddress == BROADCAST_TO_ALL_ICS) { cmd[0] = 0x00; } //0b00000xxx indicates this is a broadcast command
  else                                  { cmd[0] = 0x80 + (icAddress << 3); } //see datasheet Tables 33 & 34

  cmd[1] = 0x01; //send "write configuration registers" command ('WRCFG')
  
  uint16_t temp_pec = LTC68042configure_calcPEC15(2, cmd); //calculate PEC

  cmd[2] = (uint8_t)(temp_pec >> 8); //upper PEC byte
  cmd[3] = (uint8_t)(temp_pec); //lower PEC byte

  uint8_t cmd_index = 4; //stored byte index in the cmd array

  //add the "configuration register" bytes (CFGR0:5) to the cmd array
  for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) { cmd[cmd_index++] = configurationRegisterData[current_byte]; }

  //Calculate the PEC for the LTC6804 configuration register bytes
  temp_pec = LTC68042configure_calcPEC15(BYTES_IN_REG, &configurationRegisterData[0]);// calculate the PEC
  cmd[cmd_index++] = (uint8_t)(temp_pec >> 8); //upper PEC byte
  cmd[cmd_index++] = (uint8_t)temp_pec; //lower PEC byte

  LTC68042configure_spiWrite(CMD_LENGTH,cmd);
}

//---------------------------------------------------------------------------------------

//configure discharge resistor states on a single LTC6804 IC (CFGR4:5)
void LTC68042configure_setBalanceResistors(uint8_t icAddress, uint16_t cellBitmap, uint8_t softwareTimeout)
{

  //Each bit in cellBitmap corresponds to a specific cell's DCCn discharge bit
  //Example: cellBitmap = 0b0000 1000 0000 0011 enables discharge on cells 12, 2, and 1 //LSB is cell01
  //Example: cellBitmap = 0b0000 1111 1111 1111 enables discharge on all cells
  //See Table36
  configurationRegisterData[4] = (uint8_t)(cellBitmap); //LSByte
  configurationRegisterData[5] = ( ((uint8_t)(cellBitmap >> 8)) | softwareTimeout ); //MSByte's lower nibble

  LTC68042configure_writeConfigRegisters(icAddress);
}

//---------------------------------------------------------------------------------------

//program configuration register values onto each LTC6804 IC
//CFGR0:3 are reset when LTC watchdog timer expires (~2000 milliseconds)
//CFGR4:5 are reset when LTC watchdog timer expires, unless software timer is set (and hasn't expired)
void LTC68042configure_programVolatileDefaults(void)
{                                              // BIT7    BIT6    BIT5    BIT4    BIT3    BIT2    BIT1   BIT0                
                                               ///////////////////////////////////////////////////////////////
  configurationRegisterData[0] = 0b11111111 ;  //GPIO5   GPIO4   GPIO3   GPIO2   GPIO1   REFON   SWTRD  ADCOPT
  configurationRegisterData[1] = 0x00       ;  //VUV[7]  VUV[6]  VUV[5]  VUV[4]  VUV[3]  VUV[2]  VUV[1] VUV[0]
  configurationRegisterData[2] = 0x00       ;  //VOV[3]  VOV[2]  VOV[1]  VOV[0]  VUV[11] VUV[10] VUV[9] VUV[8]
  configurationRegisterData[3] = 0x00       ;  //VOV[11] VOV[10] VOV[9]  VOV[8]  VOV[7]  VOV[6]  VOV[5] VOV[4]
  configurationRegisterData[4] = 0x00       ;  //DCC8    DCC7    DCC6    DCC5    DCC4    DCC3    DCC2   DCC1
  configurationRegisterData[5] = 0x00       ;  //DCTO[3] DCTO[2] DCTO[1] DCTO[0] DCC12   DCC11   DCC10  DCC9
  //Above values turn off all discharge FETs, turns reference on, and configure ADC LPF to '2 kHz mode' (1.7 kHz LPF)

  LTC68042configure_writeConfigRegisters(BROADCAST_TO_ALL_ICS);
}
//see Table36 (p51) for more info:
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

bool LTC68042configure_doesActualPackSizeMatchUserConfig(void)
{
  bool helper_doesActualPackSizeMatchUserConfig = true;

  if(gpio_keyStateNow() == GPIO_KEY_OFF) //we don't have time to run this test if the key is on when LiBCM first boots
  {
    LTC6804_adax(); //send any broadcast command
    delay(6); //wait for all LTC6804 ICs to process this command

    //read data back from either QTY4 ICs (if user selects PACK_IS_48S in config.h), or QTY5 ICs (if user selects PACK_IS_60S in config.h)
    //we don't care about the actual data; only that the PEC error count doesn't increment 
    uint8_t errorCount_allUserSpecifiedLTC6804s = LTC6804_rdaux(0,TOTAL_IC,FIRST_IC_ADDR); 

    if(errorCount_allUserSpecifiedLTC6804s != 0) { helper_doesActualPackSizeMatchUserConfig = false; } //at least one IC had data transmissions errors

    if(TOTAL_IC == 4)
    {
      uint8_t errorCount_onlyFifthLTC6804 = LTC6804_rdaux(0,1,FIRST_IC_ADDR+4); 

      //the fifth LTC6804 (cells 49:60) should be unpowered (i.e. it should return errors)
      if(errorCount_onlyFifthLTC6804 == 0) { helper_doesActualPackSizeMatchUserConfig = false; }
    }

    if(helper_doesActualPackSizeMatchUserConfig == false)
    {
      Serial.print(F("\nError: measured cell count disagrees with user specified cell count in config.h"));

      lcd_begin();
      delay(50); //delay doesn't matter because this is a fatal error
      lcd_turnDisplayOnNow();
      delay(50); //delay doesn't matter because this is a fatal error
      lcd_displayWarning(LCD_WARN_CELL_COUNT);

      delay(5000); //allow time for user to read screen //delay doesn't matter because this is a fatal error

      buzzer_requestTone(BUZZER_REQUESTOR_USER, BUZZER_HIGH); //buzzer stays on forever
    }
  }
  return helper_doesActualPackSizeMatchUserConfig;
}



//---------------------------------------------------------------------------------------

void LTC68042configure_initialize(void)
{
  spi_enable(SPI_CLOCK_DIV64); //JTS2doLater: increase clock speed //DIV16 & DIV32 work on bench

  LTC68042configure_doesActualPackSizeMatchUserConfig();
}

//---------------------------------------------------------------------------------------

//wake up LTC core if watchdog timed out
bool LTC68042configure_wakeupCore(void)
{
  const uint16_t T_SLEEP_WATCHDOG_MILLIS = 1800; //'tsleep' = 1800 (min) to 2200 (max) ms 

  bool wasCoreAlreadyAwake = LTC6804_CORE_ALREADY_AWAKE;

  if( (uint32_t)(millis() - lastTimeDataSent_millis) > T_SLEEP_WATCHDOG_MILLIS )
  { 
    //LTC6804 core (probably) asleep
    digitalWrite(PIN_SPI_CS,LOW); //wake up core
    delayMicroseconds(300); // Guarantees the LTC6804 is in standby (tWake = 300 us max)  
    digitalWrite(PIN_SPI_CS,HIGH);
    lastTimeDataSent_millis = millis();
    wasCoreAlreadyAwake = LTC6804_CORE_JUST_WOKE_UP;
  }

  return wasCoreAlreadyAwake;
}

//---------------------------------------------------------------------------------------

//wake up isoSPI if timed out
void LTC68042configure_wakeupIsoSPI(void)
{
  const uint8_t T_IDLE_isoSPI_MILLIS = 4; //'tIDLE' = 4.3 (min) to 6.7 (max) ms

  if( (uint32_t)(millis() - lastTimeDataSent_millis) > T_IDLE_isoSPI_MILLIS )
  { 
    //LTC6804 isoSPI might be asleep (tIDLE elapsed)
    digitalWrite(PIN_SPI_CS,LOW);
    delayMicroseconds(10); //Guarantees isoSPI is in ready mode (tWAKE = 10 us max)
    digitalWrite(PIN_SPI_CS,HIGH);
    lastTimeDataSent_millis = millis();
  }
}

//---------------------------------------------------------------------------------------

bool LTC68042configure_wakeup(void)
{
  bool wasCoreAlreadyAwake = LTC68042configure_wakeupCore();

  if(wasCoreAlreadyAwake == LTC6804_CORE_ALREADY_AWAKE) { LTC68042configure_wakeupIsoSPI(); }
  
  return wasCoreAlreadyAwake;
}

//---------------------------------------------------------------------------------------

uint16_t LTC68042configure_calcPEC15(uint8_t len, //data array length
                                     uint8_t const data[] ) //data array to generate PEC from
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
                                uint8_t const data[] )//array of bytes to be written on the SPI port
{
  LTC68042configure_wakeup();

  digitalWrite(PIN_SPI_CS,LOW);
  for (uint8_t i = 0; i < len; i++) { spi_write((char)data[i]); } //all SPI writes occur here
  digitalWrite(PIN_SPI_CS,HIGH);

  lastTimeDataSent_millis = millis();
}

//---------------------------------------------------------------------------------------

void LTC68042configure_spiWriteRead(uint8_t tx_Data[],//array of data to be written on SPI port
                    uint8_t tx_len, //length of the tx data arry
                    uint8_t *rx_data,//Input: array that will store the data read by the SPI port
                    uint8_t rx_len )//Option: number of bytes to be read from the SPI port
{
  LTC68042configure_wakeup();

  digitalWrite(PIN_SPI_CS,LOW);
  for (uint8_t i = 0; i < tx_len; i++) { spi_write(tx_Data[i]); }
  for (uint8_t i = 0; i < rx_len; i++) { rx_data[i] = (uint8_t)spi_read(0xFF); }
  digitalWrite(PIN_SPI_CS,HIGH);
}

//---------------------------------------------------------------------------------------

void LTC68042configure_handleKeyStateChange(void)
{
  LTC68042result_errorCount_set(0);
  LTC68042result_maxEverCellVoltage_set(0);
  LTC68042result_minEverCellVoltage_set(65535);
}