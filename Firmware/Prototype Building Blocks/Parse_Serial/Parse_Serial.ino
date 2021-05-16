//Need to build this code into METSCI.c file
//Search and remove "DEBUG"

#define METSCI_DIR_PIN 3

uint16_t idleTimeDEBUG = 0;

void setup()
{  
  METSCI_begin();
  Serial.begin(115200); //USB
  pinMode(METSCI_DIR_PIN, OUTPUT);
  digitalWrite(METSCI_DIR_PIN,LOW);
}
  

void loop()
{
  
  if( Serial2.available() > 6)
  {
    idleTimeDEBUG = 0;
    Serial.println();
    Serial.print("E6 data packet is: ");
    Serial.print( METSCI_getNextE6Packet(), HEX );
    Serial.print(".  Engine packet type is: ");
    uint8_t enginePacketType = METSCI_getNextEnginePacket_Type();
    Serial.print( enginePacketType, HEX );
    Serial.print(".  Engine packet data is: ");
    Serial.print( METSCI_getNextEnginePacket_Data(enginePacketType), HEX );
  } else { //do everything else
    idleTimeDEBUG++;
    if( idleTimeDEBUG > 1000 ) //prints '.' every 1000th time this section runs
    {
      Serial.print(".");
      idleTimeDEBUG = 0;
    }
  }
}


/*
 * A METSCI packet contains 3 bytes:
 * (byte1): Packet type (E1=SoC, E6=Assist/Regen, B3=Engine Status, B4=Engine Status)
 * (byte2): Data
 * (byte3): Checksum. Sum(byte1:byte2), take 2's complement, then AND with 0x7F.  Sum(byte1:byte3) ANDed with 0x7f should equal zero. 
 *
 * A packet is sent every 100 ms (plinus 3 ms).  There are 10 packets in a complete message.  The 30 byte message repeats every second.  Example:
 * (packet0)(packet1)    e6405a e1336c    assist  SoC
 * (packet2)(packet3)    e6405a b4004c    assist  engineB4
 * (packet4)(packet5)    e6405a b4004c    assist  engineB4
 * (packet6)(packet7)    e6405a b30449    assist  engineB3
 * (packet8)(packet9)    e6405a b4004c    assist  engineB4
 */

//Returns next valid 'E6' data packet, or 0 if checksum fails
//'E6' is assist/regen gauge state (e.g. kW)
uint8_t METSCI_getNextE6Packet()
{
  if( METSCI_readByte() != 0xE6 ) //True only after data corruption and/or at startup
  { 
    while( METSCI_readByte() != 0xE6 ) {Serial.println("\nResynchronizing buffer");} //Resynchronize buffer after METSCI data corruption 
  }

  //Next two bytes are E6's Data and Checksum
  uint8_t E6_Data     = METSCI_readByte();
  uint8_t E6_Checksum = METSCI_readByte();

  return METSCI_verifyChecksum( 0xE6, E6_Data, E6_Checksum );
}

//Call after METSCI_getNextE6Packet() to ensure serial read buffer is in correct location
uint8_t METSCI_getNextEnginePacket_Type()
{
  uint8_t packetType = METSCI_readByte();
  switch( packetType )
  {
    case 0xE1:    
    case 0xB4:
    case 0xB3:
    return packetType; //All valid packet types
    break; 

    default:
    return 0; //Invalid packet type identified
    break;
  }
}

//Call after METSCI_getNextEnginePacket_Type() to ensure serial read buffer is in correct location
//Returns next valid engine data packet, or 0 if checksum fails
uint8_t METSCI_getNextEnginePacket_Data(uint8_t enginePacketType)
{
  uint8_t engineData     = METSCI_readByte();
  uint8_t engineChecksum = METSCI_readByte();

  return METSCI_verifyChecksum( enginePacketType, engineData, engineChecksum );
}

uint8_t METSCI_verifyChecksum( uint8_t type, uint8_t data, uint8_t checksum )
{
  if( ( (type + data + checksum) & 0x7F ) == 0  )
  {
    return data; //data is valid
  } else {
    Serial.println("Bad Checksum");
    return 0; //data invalid
  }
}

void METSCI_begin()
{
  Serial2.begin(9600,SERIAL_8E1);
}

//Production
inline uint8_t METSCI_readByte()
{
  return Serial2.read();
}

//Debug
/*
uint8_t METSCI_readByte()
{
  if( Serial2.available() )
  {
    uint8_t METSCI_byte = Serial2.read();
    Serial.print(METSCI_byte,HEX);
    if( METSCI_byte != -1 )
    {
      return METSCI_byte;
    }
  }
}
*/
