//reads the OEM MOTFSA/B signals sent by the MCM to the ECM

#define PIN_MOTFSA_CLK 12
#define PIN_MOTFSB_DAT 13

//works properly
bool didClockToggleHigh(void)
{
  static bool clockEdgeNow = LOW;
  static bool clockEdgePrevious = LOW;

  clockEdgeNow = digitalRead(PIN_MOTFSA_CLK);

  bool clockWentHigh = false;
  if((clockEdgeNow == HIGH) && (clockEdgePrevious == LOW)) { clockWentHigh = true; }
  
  clockEdgePrevious = clockEdgeNow;
  
  return clockWentHigh;
}

void setup()
{
  Serial.begin(115200);
  Serial.print('\n');
}

void loop()
{
  static uint16_t dataShiftRegister = 0;
  static uint8_t charactersPrinted = 0;
  
  if(didClockToggleHigh() == true)
  {
    //MOTSFA serial:
    //  S: start bit is high
    //  F: stop  bit is low
    //  D: QTY8 data bits
    //Example: S DDDDDDDD F
    //       0b1 10011110 0
     
    dataShiftRegister <<= 1; //LSB 'empty' (and zero) //we'll store MOTFSB data in LSB
    if(digitalRead(PIN_MOTFSB_DAT) == HIGH) { dataShiftRegister |= 0x001; } //store MOTFSB data in LSB

    bool checkForStartBit =   dataShiftRegister & 0b0000001000000000;
    bool checkForStopBit  = ~(dataShiftRegister & 0b0000000000000001);
      
    if( checkForStartBit && checkForStopBit)
    {
      //complete byte received //SDDDDDDDDF //see above definition
      dataShiftRegister >>= 1; //0SDDDDDDDD //lower byte contains data
      uint8_t dataReceived = (uint8_t)(dataShiftRegister & 0xFF); //DDDDDDDD

      static uint8_t previousDataReceived = 0;

      if((dataReceived != 0xFF) && (previousDataReceived == 0xFF))
      {
        Serial.print('\n');
      }

      previousDataReceived = dataReceived;
      
      Serial.print(String(dataReceived,HEX));
      Serial.print(' ');

      dataShiftRegister = 0; //clear register for next byte

//      if(charactersPrinted++ >= 23)
//      { 
//        Serial.print('\n'); 
//        charactersPrinted = 0;
//      } 
    }   
  }
}
