//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//METSCI Serial Functions

#include "libcm.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void METSCI_begin()
{
  pinMode(PIN_METSCI_DE, OUTPUT);
  digitalWrite(PIN_METSCI_DE,LOW);
  
  pinMode(PIN_METSCI_REn, OUTPUT);
  digitalWrite(PIN_METSCI_REn,HIGH);
  
  Serial3.begin(9600,SERIAL_8E1);
  Serial.print(F("\nMETSCI BEGIN"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void METSCI_enable()
{  
  digitalWrite(PIN_METSCI_REn,LOW);
}

void METSCI_disable()
{
  digitalWrite(PIN_METSCI_REn,HIGH);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t METSCI_readByte()
{
  return Serial3.read();
}

uint8_t METSCI_peekByte()
{
  return Serial3.peek();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t METSCI_bytesAvailableToRead()
{
  return Serial3.available();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//This is a non-blocking function
void METSCI_processLatestFrame(void)
{
  if( METSCI_bytesAvailableToRead() > METSCI_BYTES_IN_FRAME )  //Verify a full frame exists in the buffer
  {
    uint8_t resyncAttempt = 0; //prevents endless loop by bailing after N tries
  
    while(METSCI_peekByte() != 0xE6)
    {
      //throw away data until the next frame starts (0xE6)
      if(resyncAttempt == 0) { Serial.print("\nSync METSCI:"); } 
      else                   { Serial.print(' '); }

      Serial.print(String( METSCI_readByte() ));

      resyncAttempt++;
      if( resyncAttempt > METSCI_BYTES_IN_FRAME )
      {
        Serial.print("\n0xE6 packet not found\n");
        return; //prevent hanging in while loop
      }
    }

    //At this point the next byte is 0xE6 (unless there's some other frame type we haven't seen yet)
    if(METSCI_peekByte() == 0xE6) //start of frame
    {
      Serial.print(" MET,");
    }
    else //didn't receive an 0x87 frame
    {
      Serial.print("\nUnknown Frame Type,");
    }

    //print frame (which should be 0xE6 & 0xB4 bytes, unless there's some unknown new frame)
    for(uint8_t ii = 0; ii < METSCI_BYTES_IN_FRAME; ii++)
    {
      Serial.print(String( METSCI_readByte() ));
      Serial.print(',');
    }
  }
}