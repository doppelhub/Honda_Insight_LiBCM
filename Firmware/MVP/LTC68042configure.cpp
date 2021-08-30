//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LTC6804 configuration functions

#include "libcm.h"

uint32_t lastTimeDataSent_millis = 0; //LTC idle timer resets each time data is transferred

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