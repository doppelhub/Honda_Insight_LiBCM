//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

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

uint8_t spoofedVoltageToSend = 0;
int16_t spoofedCurrentToSend = 0; //JTS2doLater spoofed pack current can probably be int8_t (+127 A)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_begin(void)
{
  pinMode(PIN_BATTSCI_DE, OUTPUT);
  digitalWrite(PIN_BATTSCI_DE,LOW);

  pinMode(PIN_BATTSCI_REn, OUTPUT);
  digitalWrite(PIN_BATTSCI_REn,HIGH);

  Serial2.begin(9600,SERIAL_8E1);
  Serial.print(F("\nBATTSCI BEGIN"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_enable(void)
{
  digitalWrite(PIN_BATTSCI_DE,HIGH);
  BATTSCI_state = RUNNING;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_disable(void)
{
  digitalWrite(PIN_BATTSCI_DE,LOW);
  BATTSCI_state = STOPPED;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_bytesAvailableForWrite(void)
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

void BATTSCI_setPackVoltage(uint8_t spoofedVoltage)
{
  spoofedVoltageToSend = spoofedVoltage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_setSpoofedCurrent(int16_t spoofedCurrent) //JTS2doLater spoofed pack current can probably be int8_t (+127 A)
{
  spoofedCurrentToSend = spoofedCurrent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_calculateChecksum( uint8_t frameSum )
{
  uint8_t twosComplement = (~frameSum) + 1;
  return (twosComplement & 0x7F);
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
    int16_t batteryCurrent_toBATTSCI = 2048 - spoofedCurrentToSend*20;

    //Add ESR offset to max cell voltage
    //Derivation:
    //       vCellWithESR_counts = Vcell_Now                          + Icell_Now (assist: +, regen: -)      * ESR
    //       vCellWithESR_counts = Vcell_Now                          + Icell_Now                            * 1.6 mOhm
    //       vCellWithESR_counts = Vcell_counts                       + Icell_amps                           * 16 
    uint16_t vCellWithESR_counts = LTC68042result_hiCellVoltage_get() + (adc_getLatestBatteryCurrent_amps() << 4);
    //<<1=0.2mOhm, <<2=0.4mOhm, <<3=0.8mOhm, <<4=1.6mOhm, <<5=3.2mOhm, <<6=6.4mOhm, <<7=12.8mOhm //uint16_t overflows above here 

    if(frame2send == 0x87)
    {
      //Place 0x87 frame into serial send buffer
      uint8_t frameSum_87 = 0; //this will overflow, which is ok for CRC
      frameSum_87 += BATTSCI_writeByte( 0x87 );                                           //Never changes
      frameSum_87 += BATTSCI_writeByte( 0x40 );                                           //Never changes
      frameSum_87 += BATTSCI_writeByte( (spoofedVoltageToSend >> 1) );                    //Half Vbatt (e.g. 0x40 = d64 = 128 V)

      if(LTC68042result_loCellVoltage_get() <= 30000 )
      { //at least one cell is severely under-charged.  Disable Assist.
        frameSum_87 += BATTSCI_writeByte( 0x11 );                                         //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x48 ); //20% SoC                               //Battery SoC (lower byte)
        debugUSB_sendChar('1');
      }
      else
      { //all cells above 3.000 volts

        // Battery is full. Disable Regen.   
        if        (vCellWithESR_counts >= 39500) { //39500 = 3.9500 volts                                                    
          frameSum_87 += BATTSCI_writeByte( 0x16 );                                         //Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x20 ); //80% SoC                               //Battery SoC (lower byte)
          debugUSB_sendChar('8');
          //JTS2doNow: Change SoC to 81%

        // Regen & Assist, no background charge   
        } else if (vCellWithESR_counts >= 37000) { //37000 = 3.7000 volts                                               
          frameSum_87 += BATTSCI_writeByte( 0x15 );                                         //Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x50 ); //72% SoC                               //Battery SoC (lower byte)
          debugUSB_sendChar('7');

        // Regen & Assist, with background charge 
        } else if (vCellWithESR_counts >= 36000) { //34500 = 3.4500 volts                                            
          frameSum_87 += BATTSCI_writeByte( 0x14 );                                         //Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x58 ); //60% SoC                               //Battery SoC (lower byte)
          debugUSB_sendChar('6');

        // Regen & Assist, with background charge   
        } else if (vCellWithESR_counts >= 35000) { //33000 = 3.3000 volts                                              
          frameSum_87 += BATTSCI_writeByte( 0x13 );                                         //Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x10 ); //40% SoC                               //Battery SoC (lower byte)
          debugUSB_sendChar('4');

        // Battery is empty. Disable Assist.  
        } else {
          frameSum_87 += BATTSCI_writeByte( 0x11 );                                         //Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x48 ); //20% SoC                               //Battery SoC (lower byte)
          debugUSB_sendChar('2');
        }
      }

      frameSum_87 += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F ); //Battery Current (upper byte)
      frameSum_87 += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F ); //Battery Current (lower byte)
      frameSum_87 += BATTSCI_writeByte( 0x32 );                                           //Almost always 0x32
      int8_t battTempBATTSCI = temperature_battery_getLatest() * 2;
      frameSum_87 += BATTSCI_writeByte( battTempBATTSCI );                                //max temp: degC*2 (e.g. 0x3A = 58d = 29 degC //JTS2doNow: works with negative degC?
      frameSum_87 += BATTSCI_writeByte( battTempBATTSCI );                                //min temp: degC*2 (e.g. 0x3A = 58d = 29 degC
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

      //JTS2doNow: change to bitwise, with temp variable.
      if( (LTC68042result_hiCellVoltage_get() < 42000 ) && (LTC68042result_loCellVoltage_get() > 29500) )
      { //all cells are within acceptable range
        frameSum_AA += BATTSCI_writeByte( 0x00 );                                         //enable assist & regen
      
      } else if( (LTC68042result_hiCellVoltage_get() > 42000 ) && (LTC68042result_loCellVoltage_get() < 29500))
      { //at least one cell is over-changed AND at least one cell is under-charged
        //JTS2doLater: set P-code... battery is beyond redeemable.
        frameSum_AA += BATTSCI_writeByte( 0x30 );                                         //b00110000 disables regen & assist

      } else if(LTC68042result_hiCellVoltage_get() > 42000) //42000 = 4.2000
      { //at least one cell is overcharged.  disable regen
        frameSum_AA += BATTSCI_writeByte( 0x20 );                                         //b00100000 disables regen  
      
      } else if(LTC68042result_loCellVoltage_get() < 29500) //29500 = 2.9500 volts
      { //at least one cell is undercharged. disable assist
        frameSum_AA += BATTSCI_writeByte( 0x10 );                                         //b00010000 disables assist
      }

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