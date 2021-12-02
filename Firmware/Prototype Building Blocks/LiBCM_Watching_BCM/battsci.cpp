//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//BATTSCI Serial Functions

#include "libcm.h"

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

void BATTSCI_enable() { digitalWrite(PIN_BATTSCI_REn,LOW); }

void BATTSCI_disable() { digitalWrite(PIN_BATTSCI_REn,HIGH); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_readByte() { return Serial2.read(); }

uint8_t BATTSCI_peekByte() { return Serial2.peek(); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_bytesAvailableToRead() { return Serial2.available(); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//This is a non-blocking function
void BATTSCI_processLatestFrame(void)
{
  if( BATTSCI_bytesAvailableToRead() > BATTSCI_BYTES_IN_FRAME )  //Verify a full frame (0xAA + 0x87) exists in the buffer
  {
    uint8_t resyncAttempt = 0; //prevents endless loop by bailing after N tries
  
    while(BATTSCI_peekByte() != 0x87)
    {
      //throw away data until the next frame starts (0x87)
      if(resyncAttempt == 0) { Serial.print("\nSync BATTSCI:"); } 
      else                   { Serial.print(' '); }

      Serial.print(String( BATTSCI_readByte() ));

      resyncAttempt++;
      if( resyncAttempt > BATTSCI_BYTES_IN_FRAME )
      {
        Serial.print("\n0x87 packet not found\n");
        return; //prevent hanging in while loop if BATTSCI signal is corrupt (i.e. 0xE6 never occurs)
      }
    }

    //At this point the next byte is 0x87 (unless there's some other frame type we haven't seen yet)
    if(BATTSCI_peekByte() == 0x87) //start of frame
    {
      Serial.print("\nBATT,");
    }
    else //didn't receive an 0x87 frame
    {
      Serial.print("\nUnknown Frame Type,");
    }

    //print frame (which should be 0x87 & 0xAA bytes, unless there's some unknown new frame)
    for(uint8_t ii = 0; ii < BATTSCI_BYTES_IN_FRAME; ii++)
    {
      Serial.print(String( BATTSCI_readByte() ));
      Serial.print(',');
    }
  }
}