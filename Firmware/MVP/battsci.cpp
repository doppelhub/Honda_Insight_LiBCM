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

byte SoC_Bytes[] = {0x11, 0x48};
uint8_t calculatedSoC = 20;

uint8_t tempSoC = 19; // This variable and everything that uses it is only for LCD debug, and can be removed once that's no longer needed.
void    tempSoC_set(uint8_t newSoC) { tempSoC = newSoC; }
uint8_t tempSoC_get(void                 ) { return tempSoC; }

uint8_t SoCHysteresisCounter = 0;

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

void BATTSCI_evaluateSoCBytes() {
  switch(calculatedSoC)
  {
    // Wrapping for 2nd byte seems to happen at 0x00 (000) and 0x7F (127)
    case 80: SoC_Bytes[0] = 0x16; SoC_Bytes[1] = 0x20; break;
    case 79: SoC_Bytes[0] = 0x16; SoC_Bytes[1] = 0x16; break;
    case 78: SoC_Bytes[0] = 0x16; SoC_Bytes[1] = 0x0C; break;
    case 77: SoC_Bytes[0] = 0x16; SoC_Bytes[1] = 0x02; break;
    case 76: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x78; break;
    case 75: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x6E; break;
    case 74: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x64; break;
    case 73: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x5A; break;
    case 72: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x50; break;
    case 71: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x46; break;
    case 70: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x3C; break;

    case 69: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x32; break;
    case 68: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x28; break;
    case 67: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x1E; break;
    case 66: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x14; break;
    case 65: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x0A; break;
    case 64: SoC_Bytes[0] = 0x15; SoC_Bytes[1] = 0x00; break;
    case 63: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x76; break;
    case 62: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x6C; break;
    case 61: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x62; break;
    case 60: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x58; break;

    case 59: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x4E; break;
    case 58: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x44; break;
    case 57: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x3A; break;
    case 56: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x30; break;
    case 55: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x26; break;
    case 54: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x1C; break;
    case 53: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x12; break;
    case 52: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x08; break;
    case 51: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x7E; break;
    case 50: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x74; break;

    case 49: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x6A; break;
    case 48: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x60; break;
    case 47: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x56; break;
    case 46: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x4C; break;
    case 45: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x42; break;
    case 44: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x38; break;
    case 43: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x2E; break;
    case 42: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x24; break;
    case 41: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x1A; break;
    case 40: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x10; break;

    case 39: SoC_Bytes[0] = 0x13; SoC_Bytes[1] = 0x06; break;
    case 38: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x7C; break;
    case 37: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x72; break;
    case 36: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x68; break;
    case 35: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x5E; break;
    case 34: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x54; break;
    case 33: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x4A; break;
    case 32: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x40; break;
    case 31: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x36; break;
    case 30: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x2C; break;

    case 29: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x22; break;
    case 28: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x18; break;
    case 27: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x0E; break;
    case 26: SoC_Bytes[0] = 0x12; SoC_Bytes[1] = 0x04; break;
    case 25: SoC_Bytes[0] = 0x11; SoC_Bytes[1] = 0x7A; break;
    case 24: SoC_Bytes[0] = 0x11; SoC_Bytes[1] = 0x70; break;
    case 23: SoC_Bytes[0] = 0x11; SoC_Bytes[1] = 0x66; break;
    case 22: SoC_Bytes[0] = 0x11; SoC_Bytes[1] = 0x5C; break;
    case 21: SoC_Bytes[0] = 0x11; SoC_Bytes[1] = 0x52; break;
    case 20: SoC_Bytes[0] = 0x11; SoC_Bytes[1] = 0x48; break;

    default: SoC_Bytes[0] = 0x14; SoC_Bytes[1] = 0x58; break; // Default to 60 if there's an issue for some reason
  }
}

void BATTSCI_calculateSoC(uint16_t voltage)
{
  // NM:  The only Magic Number we're using here is 72% SoC.
  // 72% achieves the following behaviour in both 2000-2004 and 2005-2006 model Insights:
  // Assist Enabled, Regen Enabled, Background Regen Disabled
  // Later we can change the profile to not be based around 72, but that would require a new ECM_YEAR variable in config.h

  if (voltage >= 39000) {  // No Regen Allowed
    uint8_t tempSoCPercent = map(voltage, 39000, 42000, 77, 80);
    calculatedSoC = tempSoCPercent;
  } else if (voltage >= 36000) {  // No BG Regen Allowed
    uint8_t tempSoCPercent = map(voltage, 36000, 38999, 72, 76);
    calculatedSoC = tempSoCPercent;
  } else if ((voltage < 36000) && (voltage > 33500)) {  // BG Regen Allowed
    uint8_t tempSoCPercent = map(voltage, 33501, 35999, 31, 71);
    calculatedSoC = tempSoCPercent;
  } else if ((voltage <= 33500) && (voltage > 32500)) {  // BG Regen Allowed, SoC very low.
    uint8_t tempSoCPercent = map(voltage, 32501, 33500, 21, 30);
    calculatedSoC = tempSoCPercent;
  } else if (voltage <= 32500) {  // No Assist Allowed
    calculatedSoC = 20;
  }
  tempSoC_set(calculatedSoC);
  BATTSCI_evaluateSoCBytes();
}

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

    SoCHysteresisCounter += 1;

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

        // NM:  Updating SoC only every so often.  The OEM SoC gauge doesn't seem to function when the SoC fluctuates too quickly.
        // A different hysteresis methodology could be employed; this is just a beta implementation.
        if (SoCHysteresisCounter >= 50) {
          BATTSCI_calculateSoC(vCellWithESR_counts);
          SoCHysteresisCounter = 0;
        }

        frameSum_87 += BATTSCI_writeByte( SoC_Bytes[0] );                                 //Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( SoC_Bytes[1] );                                 //Battery SoC (lower byte)

        if        (vCellWithESR_counts >= 39500) { //39500 = 3.9500 volts
          debugUSB_sendChar('8');
        } else if (vCellWithESR_counts >= 36000) { //36000 = 3.6000 volts
          debugUSB_sendChar('7');
        } else if (vCellWithESR_counts >= 34500) { //34500 = 3.4500 volts
          debugUSB_sendChar('6');
        } else if (vCellWithESR_counts >= 33500) { //33500 = 3.3500 volts
          debugUSB_sendChar('4');
        } else {
          debugUSB_sendChar('2');
        }
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
