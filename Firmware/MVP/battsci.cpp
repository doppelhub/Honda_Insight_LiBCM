//BATTSCI Serial Functions

/************************************************************************************************************************
 * The BCM constantly sends two different 12 Byte frames to the MCM.
 *
 * The 1st frame has the following syntax:
 *
 * The 2nd frame has the following syntax:
 ************************************************************************************************************************/
#include "libcm.h"

uint8_t BATTSCI_state = STOPPED;

uint8_t packVoltageToSend = 0;
int16_t packCurrentToSend = 0; //JTS2doLater spoofed pack current can probably be int8_t (+127 A)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_begin()
{
  pinMode(PIN_BATTSCI_DE, OUTPUT);
  digitalWrite(PIN_BATTSCI_DE,LOW);

  pinMode(PIN_BATTSCI_REn, OUTPUT);
  digitalWrite(PIN_BATTSCI_REn,HIGH);

  Serial2.begin(9600,SERIAL_8E1);
  Serial.print(F("\nBATTSCI BEGIN"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_enable()
{
  digitalWrite(PIN_BATTSCI_DE,HIGH);
  BATTSCI_state = RUNNING;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_disable()
{
  digitalWrite(PIN_BATTSCI_DE,LOW);
  BATTSCI_state = STOPPED;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_bytesAvailableForWrite()
{
  return Serial2.availableForWrite();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_writeByte(uint8_t data)
{
  Serial2.write(data);
  return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_setPackVoltage(uint8_t packVoltage)
{
  packVoltageToSend = packVoltage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_setPackCurrent(int16_t packCurrent) //JTS2doLater spoofed pack current can probably be int8_t (+127 A)
{
  packCurrentToSend = packCurrent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void BATTSCI_sendFrames()
{ //t=80 microseconds max
  static uint8_t frame2send = 0x87;
  static uint32_t previousMillis = 0;

  if(    (BATTSCI_bytesAvailableForWrite() > BATTSCI_BYTES_IN_FRAME)  //Verify serial send ring buffer has room (prevent delay)
      && (BATTSCI_state == RUNNING)                                   //JTS2doLater: Replace with keyON()?
      && ((millis() - previousMillis) >= 100)                         //Send a frame every 100 ms
    )
  {
    previousMillis = millis();

    //Convert battery current (unit: amps) into BATTSCI format (unit: 50 mA per count)
    int16_t batteryCurrent_toBATTSCI = 2048 - packCurrentToSend*20;

    if(frame2send == 0x87)
    {
      //Place 0x87 frame into serial send buffer
      uint8_t frameSum_87 = 0; //this will overflow, which is ok for CRC
      frameSum_87 += BATTSCI_writeByte( 0x87 );                                          //Never changes
      frameSum_87 += BATTSCI_writeByte( 0x40 );                                          //Never changes
      frameSum_87 += BATTSCI_writeByte( (packVoltageToSend >> 1) );                      //Half Vbatt (e.g. 0x40 = d64 = 128 V)

      //JTS2doNow: This should look at max/min cell voltage, not pack voltage
      //JTS2doLater: Change SoC setpoints to different voltages
      //JTS2doNow: packVoltageToSend is spoofed voltage... not actual voltage.  Need to form cell voltage in LTC6804, then pipe it in here.
      //JTS2doLater: Need to add hysteresis (or maybe current-based cell voltage shift)
      //             pseudocode: packVoltageToSend = packVoltageActual + batteryCurrent_toBATTSCI * VoffsetPerAmp
      if (packVoltageToSend > 180) {                                                      
        // No regen 80%
        frameSum_87 += BATTSCI_writeByte( 0x16 );                                         //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x20 );                                         //Battery SoC (lower byte)
      } else if (packVoltageToSend > 160) {                                               
        // Regen and Assist but no BG Regen 75.1%
        frameSum_87 += BATTSCI_writeByte( 0x15 );                                         //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x6F );                                         //Battery SoC (lower byte)
      } else if (packVoltageToSend >= 150) {                                              
        // Regen and Assist with BG Regen 60%
        frameSum_87 += BATTSCI_writeByte( 0x14 );                                         //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x58 );                                         //Battery SoC (lower byte)
      } else if (packVoltageToSend >= 144) {                                              
        // Regen and Assist with BG Regen 40%
        frameSum_87 += BATTSCI_writeByte( 0x13 );                                         //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x10 );                                         //Battery SoC (lower byte)
      } else {
        // No assist 20%
        frameSum_87 += BATTSCI_writeByte( 0x11 );                                         //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x48 );                                         //Battery SoC (lower byte)
      }

      frameSum_87 += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F ); //Battery Current (upper byte)
      frameSum_87 += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F ); //Battery Current (lower byte)
      frameSum_87 += BATTSCI_writeByte( 0x32 );                                           //Almost always 0x32
      frameSum_87 += BATTSCI_writeByte( 0x3A );                                           //max temp: degC*2 (e.g. 0x3A = 58d = 29 degC
      frameSum_87 += BATTSCI_writeByte( 0x3A );                                           //min temp: degC*2 (e.g. 0x3A = 58d = 29 degC
      frameSum_87 += BATTSCI_writeByte( METSCI_getPacketB3() );                           //MCM's sanity check that BCM isn't getting behind
                     BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_87) );         //Send Checksum. sum(byte0:byte11) should equal 0
      frame2send = 0xAA;
    }
    else if( frame2send == 0xAA )
    {
      //Place 0xAA frame into serial send buffer
      uint8_t frameSum_AA = 0; //this will overflow, which is ok for CRC
      frameSum_AA += BATTSCI_writeByte( 0xAA );                                           //Never changes
      frameSum_AA += BATTSCI_writeByte( 0x10 );                                           //Never changes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                           //Never changes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                           //Never changes unless P codes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                           //Never changes unless P codes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                           //0x00 enable all, 0x20 no regen, 0x10 no assist, 0x30 disables both
      frameSum_AA += BATTSCI_writeByte( 0x40 );                                           //Charge request.  0x20=charge@4bars, 0x00=charge@background
      frameSum_AA += BATTSCI_writeByte( 0x61 );                                           //Never changes
      frameSum_AA += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F ); //Battery Current (upper byte)
      frameSum_AA += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F ); //Battery Current (lower byte)
      frameSum_AA += BATTSCI_writeByte( METSCI_getPacketB4() );                           //MCM's sanity check that BCM isn't getting behind
                     BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_AA) );         //Send Checksum. sum(byte0:byte11) should equal 0
      frame2send = 0x87;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_calculateChecksum( uint8_t frameSum )
{
  uint8_t twosComplement = (~frameSum) + 1;
  return (twosComplement & 0x7F);
}
