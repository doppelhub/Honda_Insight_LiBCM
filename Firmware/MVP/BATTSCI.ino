//BATTSCI Serial Functions

/************************************************************************************************************************
 * The BCM constantly sends two different 12 Byte frames to the MCM. 
 * 
 * The 1st frame has the following syntax:
 * 
 * The 2nd frame has the following syntax:
 ************************************************************************************************************************/
 
#define BATTSCI_BYTES_IN_FRAME 12

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_begin()
{
  pinMode(PIN_BATTSCI_DIR, OUTPUT);
  digitalWrite(PIN_BATTSCI_DIR,LOW);
  Serial2.begin(9600,SERIAL_8E1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_enable()
{
  digitalWrite(PIN_BATTSCI_DIR,HIGH); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_disable()
{
  digitalWrite(PIN_BATTSCI_DIR,LOW);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline uint8_t BATTSCI_bytesAvailableForWrite()
{
  return Serial1.availableForWrite();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline uint8_t BATTSCI_writeByte(uint8_t data)
{
  Serial1.write(data);
  return data;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void BATTSCI_sendFrames(struct packetTypes METSCI_Packets, uint8_t stackVoltage, int16_t batteryCurrent_Amps)
{
  //Verify we have at least 24B free in the serial send ring buffer
  if( BATTSCI_bytesAvailableForWrite() > BATTSCI_BYTES_IN_FRAME << 1 )
  {
    //Convert battery current (unit: amps) into BATTSCI format (unit: 50 mA per count)
    int16_t batteryCurrent_toBATTSCI = batteryCurrent_Amps*20 + 2048;
  
    //Place 0x87 frame into serial send buffer
    uint8_t frameSum_87 = 0; //this will overflow, which is ok for CRC
    frameSum_87 += BATTSCI_writeByte( 0x87 );                                            //Never changes
    frameSum_87 += BATTSCI_writeByte( 0x40 );                                            //Never changes
    frameSum_87 += BATTSCI_writeByte( (stackVoltage >> 1) );                             //Half battery voltage (e.g. 0x40 = d64 = 128 V
    frameSum_87 += BATTSCI_writeByte( 0x15 );                                            //Battery SoC (upper byte)
    frameSum_87 += BATTSCI_writeByte( 0x6F );                                            //Battery SoC (lower byte)
    frameSum_87 += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F );  //Battery Current (upper byte)
    frameSum_87 += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F );  //Battery Current (lower byte)
    frameSum_87 += BATTSCI_writeByte( 0x32 );                                            //Almost always 0x32
    frameSum_87 += BATTSCI_writeByte( 0x3A );                                            //max temp. Syntax: degC*2 (e.g. 0x3A = 58d = 29 degC
    frameSum_87 += BATTSCI_writeByte( 0x3A );                                            //min temp. Syntax: degC*2 (e.g. 0x3A = 58d = 29 degC
    frameSum_87 += BATTSCI_writeByte( METSCI_Packets.latestB3Packet_engine );            //MCM's sanity check that BCM isn't getting behind
                   BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_87) );          //Send Checksum. sum(byte0:byte11) should equal 0

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
    frameSum_AA += BATTSCI_writeByte( METSCI_Packets.latestB4Packet_engine );            //MCM's sanity check that BCM isn't getting behind
                   BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_AA) );           //Send Checksum. sum(byte0:byte11) should equal 0
  } 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_calculateChecksum( uint8_t frameSum )
{
  uint8_t twosComplement = (~frameSum) + 1;
  return (twosComplement & 0x7F);
}
