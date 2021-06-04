//METSCI Serial Functions

/************************************************************************************************************************
 * The MCM constantly sends a 30 Byte message with the following syntax:
 * ---------------------------------------------------------------------------------------------------------------------
 * Frame:   | Packet:            | Byte:                                            | Example:      | Packet Type:     |
 * (frame0) | (packet0)(packet1) | (Byte00)(Byte01)(Byte02)(Byte03)(Byte04)(Byte05) | E6405A E1336C | assist  SoC      |
 * (frame1) | (packet2)(packet3) | (Byte06)(Byte07)(Byte08)(Byte09)(Byte10)(Byte11) | E6405A B4004C | assist  engineB4 |
 * (frame2) | (packet4)(packet5) | (Byte12)(Byte13)(Byte14)(Byte15)(Byte16)(Byte17) | E6405A B4004C | assist  engineB4 |
 * (frame3) | (packet6)(packet7) | (Byte18)(Byte19)(Byte20)(Byte21)(Byte22)(Byte23) | E6405A B30449 | assist  engineB3 |
 * (frame4) | (packet8)(packet9) | (Byte24)(Byte25)(Byte26)(Byte27)(Byte28)(Byte29) | E6405A B4004C | assist  engineB4 |
 * ---------------------------------------------------------------------------------------------------------------------
 * 
 * As shown above, each 30 byte message consists of ten packets.
 * A single packet is sent every 100 ms, hence a complete message is sent every second.
 * Every other packet always starts with 0xE6.
 * Storing the complete message isn't important, as each frame is self-contained. 
 * 
 * At the frame level, the above table reduces to:
 * ---------------------------------------------------------
 * Frame:  | Packet:            | Byte:                    |
 * (frame) | (packet0)(packet1) | (Byte0)(Byte1)(Byte2)(Byte3)(Byte4)(Byte5) | 
 * ---------------------------------------------------------
 * 
 * Where the byte order for each frame is always:
 * -Byte0: ALWAYS 0xE6
 * -Byte1: Number of assist/regen bars displayed on instrument panel**
 * -Byte2: Checksum (Byte0+Byte1)
 * -Byte3: Is either 0xE1, 0xB4, or 0xB3***
 * -Byte4: Data (linked to Byte3 value)
 * -Byte5: Checksum (Byte3+Byte4)
 * 
 * Given the above, we know the following:
 * -Byte0 never changes, so we don't need to store it.
 * -Byte2 & Byte5 are checksums.  After verifying the checksum, we don't need to store them.
 * -Therefore, we only return Byte1/Byte3/Byte4.
 * 
 * ---------------------------------------------------------------------------------------------------------------------
 * 
 * **The number of assist/regen bars is:
 * Byte0 Byte1 Byte2
 * -----------------
 * E6 40 5A = No Assist or Regen JTS
 * E6 41 59 = 01 Bars Assist
 * E6 42 58 = 02 Bars Assist
 * E6 43 57 = 03 Bars Assist
 * E6 44 56 = 04 Bars Assist
 * E6 45 55 = 05 Bars Assist
 * E6 46 54 = 06 Bars Assist
 * E6 47 53 = 07 Bars Assist
 * E6 48 52 = 08 Bars Assist
 * E6 49 51 = 09 Bars Assist
 * E6 4A 50 = 10 Bars Assist
 * E6 4B 4F = 11 Bars Assist
 * E6 4C 4E = 12 Bars Assist
 * E6 4D 4D = 13 Bars Assist
 * E6 4E 4C = 14 Bars Assist
 * E6 4F 4B = 15 Bars Assist
 * E6 50 4A = 16 Bars Assist
 * E6 51 49 = 17 Bars Assist
 * E6 52 48 = 18 Bars Assist
 * E6 53 47 = 19 Bars Assist
 * E6 54 46 = 20 Bars Assist
 * E6 21 79 = 01 Bars Regen
 * E6 22 78 = 02 Bars Regen
 * E6 23 77 = 03 Bars Regen 
 * E6 24 76 = 04 Bars Regen
 * E6 25 75 = 05 Bars Regen
 * E6 26 74 = 06 Bars Regen
 * E6 27 73 = 07 Bars Regen
 * E6 28 72 = 08 Bars Regen
 * E6 29 71 = 09 Bars Regen
 * E6 2A 70 = 10 Bars Regen
 * E6 2B 6F = 11 Bars Regen
 * E6 2C 6E = 12 Bars Regen
 * E6 2D 6D = 13 Bars Regen
 * E6 2E 6C = 14 Bars Regen
 * E6 2F 6B = 15 Bars Regen
 * E6 30 6A = 16 Bars Regen
 * E6 31 69 = 17 Bars Regen
 * E6 32 68 = 18 Bars Regen
 * E6 33 67 = 19 Bars Regen
 * E6 34 66 = 20 Bars Regen
 * 
 * ---------------------------------------------------------------------------------------------------------------------
 * 
 * ***Where:
 * If Byte3 = 0xE1, then Byte4 indicates IMA state of charge:
 * Byte3 Byte4 Byte5
 * -----------------
 * E1 20 7F = 0 Bars
 * E1 21 7E = 1 Bars
 * E1 22 7D = 2 Bars
 * E1 23 7C = 3 Bars
 * E1 24 7B = 4 Bars
 * E1 25 7A = 5 Bars
 * E1 26 79 = 6 Bars 
 * E1 27 78 = 7 Bars
 * E1 28 77 = 8 Bars
 * E1 29 76 = 9 Bars
 * E1 2A 75 = 10 Bars
 * E1 2B 74 = 11 Bars  
 * E1 2C 73 = 12 Bars
 * E1 2D 72 = 13 Bars
 * E1 2E 71 = 14 Bars
 * E1 2F 70 = 15 Bars
 * E1 30 6F = 16 Bars
 * E1 31 6E = 17 Bars  
 * E1 32 6D = 18 Bars
 * E1 33 6C = 19 Bars
 * E1 34 6B = 20 Bars
 * 
 * If Byte3 = 0xB3, then Byte4 indicates engine status.  Details not fully understood.  Peter knows the most about this.
 * If Byte3 = 0xB4, then Byte4 indicates engine status.  Details not fully understood.  Peter knows the most about this.
 ************************************************************************************************************************/
 
#define METSCI_BYTES_IN_FRAME 6

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void METSCI_begin()
{
  pinMode(PIN_METSCI_DIR, OUTPUT);
  digitalWrite(PIN_METSCI_DIR,LOW);
  Serial2.begin(9600,SERIAL_8E1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline uint8_t METSCI_readByte()
{
  return Serial2.read();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline uint8_t METSCI_bytesAvailable()
{
  return Serial2.available();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//This is a non-blocking function:
//If a new frame isn't available in the serial receive buffer,
//then the previous frame is returned immediately.
//No time to wait around for 9600 baud frames!
struct triByte METSCI_getLatestFrame(void)
{
  //Serial.print("\nMETSCI_getLatestFrame Ran. ");

  //Check if we have more than one complete frame in queue (because we've somehow fallen behind)
  while( METSCI_bytesAvailable() > (METSCI_BYTES_IN_FRAME << 1) ) //True if two or more full frames are stored in serial ring buffer
  {
    Serial.print(F("\nMETSCI stale.  Discarding frame: "));
    for(int ii=0; ii < METSCI_BYTES_IN_FRAME; ii++) //delete complete frame
    {
      Serial.print(String(METSCI_readByte(), HEX) ); //Display discarded, outdated frame
    }
  }
  
  //At this point we should have ONLY the latest complete frame in queue

  //if everything is in sync, guarantees a complete frame is waiting in serial receive ring buffer
  if( METSCI_bytesAvailable() > METSCI_BYTES_IN_FRAME) 
  {
    uint8_t packetType, packetData, packetCRC;
 
    while( METSCI_readByte() != 0xE6 ) //throw away data until the next 0xE6 byte occurs
    {
      Serial.print( F("\nMETSCI synchronizing buffer\n") ); //Resynchronize buffer after METSCI data corruption 
    }
    
    packetType = 0xE6;              //Byte0 (always 0xE6) (we discarded it above)
    packetData = METSCI_readByte(); //Byte1 (always number of bars assist/regen)
    packetCRC  = METSCI_readByte(); //Byte2 (checksum)
    if( METSCI_isChecksumValid( packetType, packetData, packetCRC) )
    {
      METSCI_Frame.dashAssistLevel = packetData;
    }
    
    packetType = METSCI_readByte(); //Byte3 (either 0xE1, 0xB3, or 0xB4)
    packetData = METSCI_readByte(); //Byte4 (data)
    packetCRC  = METSCI_readByte(); //Byte5 (checksum)
    if( METSCI_isChecksumValid( packetType, packetData, packetCRC ) )
    {
      METSCI_Frame.data = packetData; 
      METSCI_Frame.dataType = packetType;  
    }
  }
  //Serial.print("\nMETSCI_Frame inside METSCI.c is: " + String(METSCI_Frame.dashAssistLevel,HEX) + "|" + String(METSCI_Frame.dataType,HEX) + "|" + String(METSCI_Frame.data,HEX) ); 
  return METSCI_Frame;
}

uint8_t METSCI_isChecksumValid( uint8_t type, uint8_t data, uint8_t checksum )
{
  if( ( (type + data + checksum) & 0x7F ) == 0  )
  {
    return 1; //data is valid
  } else {
    Serial.println(F("METSCI Bad Checksum"));
    return 0; //data invalid
  }
}
