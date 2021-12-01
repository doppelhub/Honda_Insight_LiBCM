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
int16_t spoofedCurrentToSend = 0;     //JTS2doLater spoofed pack current can probably be int8_t (+127 A)

uint8_t LiBCM_SoC = 60;

byte SoC_Bytes[] = {0x15, 0x50};      // SoC Bytes to send to MCM.  Index 0 is the upper byte and index 1 is the lower byte.
byte temperature_Byte = 0x33;         // Temperature Byte to send to MCM.  0x33 is +21 Degrees C.
uint16_t calculatedSoC = 500;
uint16_t oldCalculatedSoC = 500;

bool initializeSoC = true;

uint8_t SoCUpdateDelayFrameIncrement = 6;	// How many frames between SoC updates to MCM?
uint8_t SoCUpdateDelayCounter = 207;

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

void BATTSCI_evaluateSoCBytes(uint16_t evalSoC) {
  //  BCM sends two bytes in a row for SoC values.  First is the upper byte, second is the lower byte.
  //  Upper byte increments every time the lower byte hits 128.  Lower byte valid values therefore are 0x00 --> 0x7F inclusive.

  //  80% = 0x16 0x20 -- No Regen, Assist allowed
  //  72% = 0x15 0x50 -- Regen allowed, Assist allowed
  //  60% = 0x14 0x58 -- Regen allowed, Assist allowed, Background Regen allowed
  //  40% = 0x13 0x10 -- Regen allowed, Assist allowed, Background Regen more aggressive
  //  25% = 0x11 0x7A -- Regen allowed, Assist barely allowed, Background Regen very aggressive
  //  20% = 0x11 0x48 -- Regen allowed, No Assist, Regen running even during idle

  // MCM can read and accept values outside this window, like 19% SoC or 85% SoC, but we don't need to use those.

  /**
    BATTSCI_evaluateSoCBytes evaluates what bytes LiBCM will send to the MCM to set the SoC, depending on the SoC we gave it

    @param      evalSoC     Integer value of SoC in 0.1% increments beginning at 20%. 0 = 20.0%
                            This should be between 0 and 600.
                            Out of bounds values will still work, but may cause unexpected results.
    @return                 This function does not return anything.  It modifies SoC_Bytes[] in place.
  */

  SoC_Bytes[0] = 0x00;
  evalSoC += 0x48;			// MCM 2nd-byte SoC is 0x48 when SoC is 20%.  20% is our reference, so we need to add this first.

  do {
    SoC_Bytes[0] += 0x01;
    evalSoC -= 128;			// MCM 2nd-byte SoC CANNOT exceed 0x7F so we need to subtract 128 from it over and over until 2nd-byte is < 0x80
  }
  while (evalSoC > 128);

  SoC_Bytes[0] += 0x11;		// MCM 1st-byte SoC is 0x11 when SoC is 20% so we need to add 0x11.
  SoC_Bytes[1] = evalSoC;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_evaluateTemperatureByte(uint16_t evalSoC) {
  /**
    BATTSCI_evaluateTemperatureByte determines the IMA battery temperature we are sending to the MCM.

    @param      evalSoC     Integer value of SoC in 0.1% increments beginning at 20%. 0 = 20.0%
    @return                 This function does not return anything.
  */

  // battTempBATTSCI will be displayed if it won't affect MCM behaviour, as determined by SoC
  // 0x26 = 08 Deg C
  // 0x30 = 18 Deg C
  // 0x31 = 19 Deg C
  int8_t battTempBATTSCI = temperature_battery_getLatest() + 30;                      //T_MCM = T_actual + 30

  if (evalSoC >= 200)             // 200 = 40% SoC or higher
  {
	if (battTempBATTSCI > 0x33) {
		temperature_Byte = battTempBATTSCI;
	} else temperature_Byte = 0x33;      // Set temperature to +21 deg C to allow max Assist in 2nd and 3rd
  } else {
	if (battTempBATTSCI < 0x30) {
		temperature_Byte = battTempBATTSCI;
	} else temperature_Byte = 0x30;      // Set temperature to +18 deg C to reduce max Assist in 2nd and 3rd
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_calculateSoC()
{
  /**
    BATTSCI_calculateSoC calculates what SoC we want to send the MCM, in order to govern the MCM's behaviour.
    The MCM changes how the car uses assist and regen depending on the SoC number we send to it.

    @return                 This function does not return anything.
  */
  LiBCM_SoC = SoC_getBatteryStateNow_percent();

  if (LiBCM_SoC >= 95) {
    calculatedSoC = 620;          // No Regen Allowed, set MCM SoC high to enforce a cooldown period (while SoC drops) before regen is allowed again.
    oldCalculatedSoC = 620;       // Make sure SoC doesn't immediately spike back down
  } else if (LiBCM_SoC >= 90) {
    calculatedSoC = map(LiBCM_SoC, 90, 94, 600, 619);   // No Regen Allowed
  } else if (LiBCM_SoC >= 60) {
    calculatedSoC = map(LiBCM_SoC, 60, 89, 501, 599);   // No BG Regen Allowed
  } else if (LiBCM_SoC >= 30) {
    calculatedSoC = map(LiBCM_SoC, 30, 59, 151, 500);   // BG Regen Allowed
  } else if (LiBCM_SoC >= 5) {
    calculatedSoC = map(LiBCM_SoC, 5, 29, 1, 150);      // BG Regen Allowed, SoC very low.
  } else {
    // No Assist Allowed
    calculatedSoC = 0;
    oldCalculatedSoC = 0;         // Make sure SoC doesn't immediately spike back up
    BATTSCI_evaluateSoCBytes(calculatedSoC);
  }

  // On first run we set oldCalculatedSoC and calculatedSoC to the same value.
  if (initializeSoC) {
    oldCalculatedSoC = calculatedSoC;
    initializeSoC = false;
    BATTSCI_evaluateSoCBytes(calculatedSoC);
  }

  static int16_t packMilliAmps = 0;
  packMilliAmps = adc_getLatestBatteryCurrent_amps();

  // Modify calculatedSoC in place to be an increment of oldCalculatedSoC
  // packMilliAmps is being checked so that we only increment SoC if we have current < -0.01 Amps or decremented if we have > +0.01 Amps
  // This should prevent SoC being changed during Auto Stop.
  // To Do: We may be able to remove these checks once we are calculating an LiBCM SoC using coulomb counting.
  if (calculatedSoC > oldCalculatedSoC) {
    if (packMilliAmps <= -10 ) {				// packMilliAmps is - if Amps are going IN and voltage is going UP
      calculatedSoC = (oldCalculatedSoC + 1);
      BATTSCI_evaluateSoCBytes(calculatedSoC);
      oldCalculatedSoC = calculatedSoC;         // Set oldCalculatedSoC to the incremented calculatedSoC
    }
  } else if (calculatedSoC < oldCalculatedSoC) {
    if (packMilliAmps >= 10 ) {					// packMilliAmps is + if Amps are going OUT and voltage is going DOWN
      calculatedSoC = (oldCalculatedSoC - 1);
      BATTSCI_evaluateSoCBytes(calculatedSoC);
      oldCalculatedSoC = calculatedSoC;         // Set oldCalculatedSoC to the incremented calculatedSoC
    }
  }

  BATTSCI_evaluateTemperatureByte(calculatedSoC);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_initializeMCMSoC(void)
{
  // Key On causes a recalculation of SoC to spoof to MCM
	initializeSoC = true;
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

    SoCUpdateDelayCounter += 1;

    if(frame2send == 0x87)
    {
      //Place 0x87 frame into serial send buffer
      uint8_t frameSum_87 = 0; //this will overflow, which is ok for CRC
      frameSum_87 += BATTSCI_writeByte( 0x87 );                                           //Never changes
      frameSum_87 += BATTSCI_writeByte( 0x40 );                                           //Never changes
      frameSum_87 += BATTSCI_writeByte( (spoofedVoltageToSend >> 1) );                    //Half Vbatt (e.g. 0x40 = d64 = 128 V)

      LiBCM_SoC = SoC_getBatteryStateNow_percent();
      if(LiBCM_SoC <= 5 )
      {
        //at least one cell is severely under-charged.  Disable Assist.  0x11 0x48 is 20% SoC.  This if statement functionally performs a negaive recal.
        SoC_Bytes[0] = 0x11;
        SoC_Bytes[1] = 0x48;

        // Make sure SoC doesn't immediately spike back up
        calculatedSoC = 0;
        oldCalculatedSoC = 0;
        temperature_Byte = 0x30;
        SoCUpdateDelayCounter = 0;  // Setting this to 0 keeps SoC at 20% for extra frames so the IMA has extra time to be charged.

        debugUSB_sendChar('1');
      }
      else
      { // all cells above 3.000 volts

        // The loop begins at 200 so if we need to we can force an SoC value to be held longer, such as if we need to do a positive or negative recal.
        if (SoCUpdateDelayCounter >= (SoCUpdateDelayFrameIncrement + 200)) {
          BATTSCI_calculateSoC();
          SoCUpdateDelayCounter = 200;
	    }

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

	  frameSum_87 += BATTSCI_writeByte( SoC_Bytes[0] );                                   //Battery SoC (upper byte)
	  frameSum_87 += BATTSCI_writeByte( SoC_Bytes[1] );                                   //Battery SoC (lower byte)

      frameSum_87 += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F ); //Battery Current (upper byte)
      frameSum_87 += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F ); //Battery Current (lower byte)
      frameSum_87 += BATTSCI_writeByte( 0x32 );                                      //Almost always 0x32 -- 0x00 P1449	0x01 P1449	0x10 P1447	0x11 P1447	0x12 P1447	0X14 P1447	0x15 N	0x16 N	0x17 N	0x20 N	0x30 N	0x31 N	0x33 N	0x7F N
      frameSum_87 += BATTSCI_writeByte( temperature_Byte );                               //max temp: degC*2 (e.g. 0x3A = 58d = 28 degC
      frameSum_87 += BATTSCI_writeByte( temperature_Byte );                               //min temp: degC*2 (e.g. 0x3A = 58d = 28 degC
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
      frameSum_AA += BATTSCI_writeByte( 0x61 );                                           //Never changes	0x00 N	0x01 P1440	0x60 N	0x32 N
      frameSum_AA += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F ); //Battery Current (upper byte)
      frameSum_AA += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F ); //Battery Current (lower byte)
      frameSum_AA += BATTSCI_writeByte( METSCI_getPacketB4() );                           //MCM's sanity check that BCM isn't getting behind
                     BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_AA) );         //Send Checksum. sum(byte0:byte11) should equal 0
      frame2send = 0x87;
    }
  }
}
