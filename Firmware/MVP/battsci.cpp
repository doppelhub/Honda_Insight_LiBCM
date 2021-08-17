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


void BATTSCI_sendFrames(uint8_t stackVoltage, int16_t batteryCurrent_Amps)
{
  static uint8_t frame2send = 0x87;

  //Verify we have at least 12B free in the serial send ring buffer
  if( (BATTSCI_bytesAvailableForWrite() > BATTSCI_BYTES_IN_FRAME) && BATTSCI_state == RUNNING )
  {
    //Convert battery current (unit: amps) into BATTSCI format (unit: 50 mA per count)
    int16_t batteryCurrent_toBATTSCI = 2048 - batteryCurrent_Amps*20;

    if(frame2send == 0x87)
    {
      //Place 0x87 frame into serial send buffer
      uint8_t frameSum_87 = 0; //this will overflow, which is ok for CRC
      frameSum_87 += BATTSCI_writeByte( 0x87 );                                            //Never changes
      frameSum_87 += BATTSCI_writeByte( 0x40 );                                            //Never changes
      frameSum_87 += BATTSCI_writeByte( (stackVoltage >> 1) );                             //Half battery voltage (e.g. 0x40 = d64 = 128 V
//      frameSum_87 += BATTSCI_writeByte( 0x16 );                                            //Battery SoC (upper byte)
//      frameSum_87 += BATTSCI_writeByte( 0x20 );                                            //Battery SoC (lower byte)
      if (stackVoltage > 180) {                                                       // 180 = 3.75 volts per cell
        // No regen 80%
        frameSum_87 += BATTSCI_writeByte( 0x16 );                                        //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x20 );                                        //Battery SoC (lower byte)
      } else if (stackVoltage > 160) {                                                // 160 = 3.33 volts per cell
        // Regen and Assist but no BG Regen 75.1%
        frameSum_87 += BATTSCI_writeByte( 0x15 );                                        //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x6F );                                        //Battery SoC (lower byte)
      } else if (stackVoltage >= 150) {                                               // 150 = 3.125 volts per cell
        // Regen and Assist with BG Regen 60%
        frameSum_87 += BATTSCI_writeByte( 0x14 );                                        //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x58 );                                        //Battery SoC (lower byte)
      } else if (stackVoltage >= 144) {                                               // 144 = 3.00 volts per cell
        // Regen and Assist with BG Regen 40%
        frameSum_87 += BATTSCI_writeByte( 0x13 );                                        //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x10 );                                        //Battery SoC (lower byte)
      } else {
        // No assist 20%
        frameSum_87 += BATTSCI_writeByte( 0x11 );                                        //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x48 );                                        //Battery SoC (lower byte)
      }
      /*
      0x11 0x48 == 20.0
      0x13 0x10 == 40.0
      0x14 0x58 == 60.0;
      0x15 0x6F == 75.1; // DEFAULT
      0x16 0x20 == 80.0;
      */
      frameSum_87 += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F );  //Battery Current (upper byte)
      frameSum_87 += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F );  //Battery Current (lower byte)
      frameSum_87 += BATTSCI_writeByte( 0x32 );                                            //Almost always 0x32
      frameSum_87 += BATTSCI_writeByte( 0x3A );                                            //max temp. Syntax: degC*2 (e.g. 0x3A = 58d = 29 degC
      frameSum_87 += BATTSCI_writeByte( 0x3A );                                            //min temp. Syntax: degC*2 (e.g. 0x3A = 58d = 29 degC
      frameSum_87 += BATTSCI_writeByte( METSCI_getPacketB3() );                            //MCM's sanity check that BCM isn't getting behind
                     BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_87) );          //Send Checksum. sum(byte0:byte11) should equal 0
      frame2send = 0xAA;
    } else if( frame2send == 0xAA )
    {
      //Place 0xAA frame into serial send buffer
      uint8_t frameSum_AA = 0; //this will overflow, which is ok for CRC
      frameSum_AA += BATTSCI_writeByte( 0xAA );                                            //Never changes
      frameSum_AA += BATTSCI_writeByte( 0x10 );                                            //Never changes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                            //Never changes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                            //Never changes unless P codes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                            //Never changes unless P codes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                            //0x00 enable all, 0x20 no regen, 0x10 no assist, 0x30 disables both
      frameSum_AA += BATTSCI_writeByte( 0x40 );                                            //Charge request.  0x20=charge@4bars, 0x00=charge@background
      frameSum_AA += BATTSCI_writeByte( 0x61 );                                            //Never changes
      frameSum_AA += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F );  //Battery Current (upper byte)
      frameSum_AA += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F );  //Battery Current (lower byte)
      frameSum_AA += BATTSCI_writeByte( METSCI_getPacketB4() );                            //MCM's sanity check that BCM isn't getting behind
                     BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_AA) );          //Send Checksum. sum(byte0:byte11) should equal 0
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
