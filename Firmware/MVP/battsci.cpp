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
      frameSum_87 += BATTSCI_writeByte( 0x87 );                                           //B0 Never changes
      frameSum_87 += BATTSCI_writeByte( 0x40 );                                           //B1 Never changes
      frameSum_87 += BATTSCI_writeByte( (spoofedVoltageToSend >> 1) );                    //B2 Half Vbatt (e.g. 0x40 = d64 = 128 V)

      //To convert LiBCM_SoC to MCM_SoC
      //LiBCM_SoC_deciPercent = LiBCM_SoC_percent * 10;
      //SoC_Integer = LiBCM_SoC_deciPercent / 128; //Example: 75% SoC = 75*10/128 = 5d
      //SoC_upper_byte = (SoC_Integer | 0x00010000)  //set flag in the upper nibble //we don't know what this flag does... but it's required.
      //SoC_lower_byte = LiBCM_SoC_deciPercent - (SoC_Integer * 128);

      //JTS2doNow: Replace this garbage with current-accumulated SoC
      if(LTC68042result_loCellVoltage_get() <= 30000 )
      { //at least one cell is severely under-charged.  Disable Assist.
        frameSum_87 += BATTSCI_writeByte( 0x11 );                                         //B3 Battery SoC (upper byte)
        frameSum_87 += BATTSCI_writeByte( 0x48 ); //20% SoC                               //B4 Battery SoC (lower byte)
        debugUSB_sendChar('1');
      }
      else
      { //all cells above 3.000 volts

        // Battery is full. Disable Regen.
        if        (vCellWithESR_counts >= 39500) { //39500 = 3.9500 volts
          frameSum_87 += BATTSCI_writeByte( 0x16 );                                       //B3 Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x20 ); //80% SoC                             //B4 Battery SoC (lower byte)
          debugUSB_sendChar('8');
          //JTS2doNow: Change SoC to 81%

        // Regen & Assist, no background charge
        } else if (vCellWithESR_counts >= 37000) { //37000 = 3.7000 volts
          frameSum_87 += BATTSCI_writeByte( 0x15 );                                       //B3 Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x50 ); //72% SoC                             //B4 Battery SoC (lower byte)
          debugUSB_sendChar('7');

        // Regen & Assist, with background charge
        } else if (vCellWithESR_counts >= 36000) { //34500 = 3.4500 volts
          frameSum_87 += BATTSCI_writeByte( 0x14 );                                       //B3 Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x58 ); //60% SoC                             //B4 Battery SoC (lower byte)
          debugUSB_sendChar('6');

        // Regen & Assist, with background charge
        } else if (vCellWithESR_counts >= 35000) { //33000 = 3.3000 volts
          frameSum_87 += BATTSCI_writeByte( 0x13 );                                       //B3 Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x10 ); //40% SoC                             //B4 Battery SoC (lower byte)
          debugUSB_sendChar('4');

        // Battery is empty. Disable Assist.
        } else {
          frameSum_87 += BATTSCI_writeByte( 0x11 );                                       //B3 Battery SoC (upper byte)
          frameSum_87 += BATTSCI_writeByte( 0x48 ); //20% SoC                             //B4 Battery SoC (lower byte)
          debugUSB_sendChar('2');
        }
      }

      frameSum_87 += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F ); //B5 Battery Current (upper byte)
      frameSum_87 += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F ); //B6 Battery Current (lower byte)
      frameSum_87 += BATTSCI_writeByte( 0x32 );                                           //B7 always 0x32, except before 0xAAbyte5 changes from 0x00 to 0x10 (then 0x23)

      int8_t battTempBATTSCI = temperature_battery_getLatest() + 30;                      //T_MCM = T_actual + 30
      frameSum_87 += BATTSCI_writeByte( battTempBATTSCI );                                //B8 max temp //0 degC = 30d = 0x1E
      frameSum_87 += BATTSCI_writeByte( battTempBATTSCI );                                //B9 min temp

      frameSum_87 += BATTSCI_writeByte( METSCI_getPacketB3() );                           //B10 MCM latest B3 data byte
                     BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_87) );         //B11 Send Checksum. sum(byte0:byte11) should equal 0
      frame2send = 0xAA;
    }
    else if( frame2send == 0xAA )
    {
      //Place 0xAA frame into serial send buffer
      uint8_t frameSum_AA = 0; //this will overflow, which is ok for CRC
      frameSum_AA += BATTSCI_writeByte( 0xAA );                                           //B0 Never changes
      frameSum_AA += BATTSCI_writeByte( 0x10 );                                           //B1 Always 0x10, unless METSCI signal not received
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                           //B2 Never changes unless P codes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                           //B3 Never changes unless P codes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                           //B4 Never changes unless P codes
      frameSum_AA += BATTSCI_writeByte( 0x00 );                                           //B5 Never changes unless P codes //search note on '0xAAbyte5'

      //charge request byte //limits assist and regen
      //first verify all cell voltages are in range
      if     (LTC68042result_loCellVoltage_get() < CELL_VMIN_KEYON) { frameSum_AA += BATTSCI_writeByte( 0x32 ); } //B6 battery empty; disable assist
      else if(LTC68042result_hiCellVoltage_get() > CELL_VMAX_KEYON) { frameSum_AA += BATTSCI_writeByte( 0x52 ); } //B6 battery full; disable regen
      else
      {
        //if we get here, all cells are at a 'safe' voltage
        //use current-accumulated SoC to determine whether we should disable assist or regen
        if( (METSCI_getPacketB4() == 24) || (METSCI_getPacketB4() == 0) ) //These METSCI 'B4' data values indicate the key has just turned on
        {
          if(SoC_getBatteryStateNow_percent() > (STACK_SoC_MIN + 1) )   { frameSum_AA += BATTSCI_writeByte( 0x40 ); } //B6 use IMA to start the engine
          else                                                          { frameSum_AA += BATTSCI_writeByte( 0x20 ); } //B6 use backup starter
        }
        else if(SoC_getBatteryStateNow_percent() > (STACK_SoC_MAX - 1)) { frameSum_AA += BATTSCI_writeByte( 0x52 ); } //B6 battery full; disable regen
        else if(SoC_getBatteryStateNow_percent() < (STACK_SoC_MIN + 1)) { frameSum_AA += BATTSCI_writeByte( 0x32 ); } //B6 battery empty; disable assist
        else /* battery neither charged nor empty */                    { frameSum_AA += BATTSCI_writeByte( 0x12 ); } //B6 enable both regen & assist
      }

      frameSum_AA += BATTSCI_writeByte( 0x61 );                                            //B7 BCM hardware/firmware version?
      frameSum_AA += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F );  //B8 Battery Current (upper byte)
      frameSum_AA += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F );  //B9 Battery Current (lower byte)
      frameSum_AA += BATTSCI_writeByte( METSCI_getPacketB4() );                            //B10 MCM latest B4 data byte //
                     BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_AA) );          //B11 Send Checksum. sum(byte0:byte11) should equal 0
      frame2send = 0x87;
    }
  }
}
