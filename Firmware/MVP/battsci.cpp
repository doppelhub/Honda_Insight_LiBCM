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

uint8_t LiBCM_SoC = SoC_getBatteryStateNow_percent();

byte SoC_Bytes[] = {0x01, 0x71};      // SoC Bytes to send to MCM.  Index 0 is the upper byte and index 1 is the lower byte.
byte temperature_Byte = 0x33;         // Temperature Byte to send to MCM.  0x33 is +21 Degrees C.
uint16_t SoC_sentToMCM = 500;
uint16_t lastSoC_sentToMCM = 500;

bool initializeSpoofedSoC = true;

uint8_t SoC_sentToMCMDelayIncrement = 17;	// How many frames between SoC updates to MCM?  OEM BCM usually does 17 but sometimes does 16
uint8_t SoC_sentToMCMDelayCounter = 218;

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
  initializeSpoofedSoC = true;
  SoC_Bytes[0] = 0x01;
  SoC_Bytes[1] = 0x71;
  SoC_sentToMCMDelayCounter = 218;
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

void BATTSCI_simulateNegativeRecal() {
	// SoC is very low.  Disable Assist.  0x21 0x48 is 20% SoC.
	// Make sure spoofed SoC doesn't immediately spike back up
	SoC_sendToMCM = 190;
	lastSoC_sentToMCM = 190;
	SoC_sentToMCMDelayCounter = 0;  // Setting this to 0 keeps spoofed SoC at 19% for extra frames so the IMA has extra time to be charged.
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
  //  20% = 0x21 0x48 -- THIS IS ALSO 20%

  /**
    BATTSCI_evaluateSoCBytes evaluates what bytes LiBCM will send to the MCM to set the SoC, depending on the SoC we gave it

    @param      evalSoC     Integer value of SoC in 0.1% increments.
    @return                 This function does not return anything.  It modifies SoC_Bytes[] in place.
  */

  SoC_Bytes[0] = 0x20;		// MCM 1st-byte SoC is 0x10 OR 0x20 when SoC is 0% so we need to add 0x20.
  do {
    SoC_Bytes[0] += 0x01;
    evalSoC -= 128;			// MCM 2nd-byte SoC CANNOT exceed 0x7F so we need to subtract 128 from it over and over until 2nd-byte is < 0x80
  }
  while (evalSoC > 128);
  SoC_Bytes[1] = evalSoC;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_evaluateTemperatureByte() {
  /**
    BATTSCI_evaluateTemperatureByte determines the IMA battery temperature we are sending to the MCM.

    @return                 This function does not return anything.
  */
  // battTempBATTSCI will be displayed if it won't affect MCM behaviour
  // 0x33 = 21 Deg C
  int8_t battTempBATTSCI = temperature_battery_getLatest() + 30;	//T_MCM = T_actual + 30
  if (battTempBATTSCI > 0x33) {
    temperature_Byte = battTempBATTSCI;
  } else temperature_Byte = 0x33;									// Set temperature to +21 deg C to allow max Assist in 2nd and 3rd
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_calculateSoC_sentToMCM()
{
	/**
	BATTSCI_calculateSoC_sentToMCM calculates what SoC we want to send the MCM, in order to govern the MCM's behaviour.
	The MCM changes how the car uses assist and regen depending on the SoC number we send to it.

	@return                 This function does not return anything.
	*/
	LiBCM_SoC = SoC_getBatteryStateNow_percent();

	if ((LiBCM_SoC > STACK_SoC_MAX) || (LTC68042result_hiCellVoltage_get() > CELL_MAX_ALLOWED_VOLTAGE_REGEN)) {
		SoC_sendToMCM = 820;          // No Regen Allowed, set MCM SoC high to enforce a cooldown period (while SoC drops) before regen is allowed again.
		lastSoC_sentToMCM = 820;       // Make sure SoC doesn't immediately spike back down
	} else if (LiBCM_SoC >= 90) {
		SoC_sendToMCM = map_16bU(LiBCM_SoC, 90, 94, 800, 819);   // No Regen Allowed
	} else if (LiBCM_SoC >= 60) {
		SoC_sendToMCM = map_16bU(LiBCM_SoC, 60, 89, 701, 799);   // No BG Regen Allowed 84 = 582
	} else if (LiBCM_SoC >= 30) {
		SoC_sendToMCM = map_16bU(LiBCM_SoC, 30, 59, 351, 700);   // BG Regen Allowed
	} else if (LiBCM_SoC >= 10) {
		SoC_sendToMCM = map_16bU(LiBCM_SoC, 10, 29, 201, 350);   // BG Regen Allowed, SoC very low.
	} else {
		BATTSCI_simulateNegativeRecal();
		BATTSCI_evaluateSoCBytes(SoC_sendToMCM);
	}

	// On first run we set lastSoC_sentToMCM and SoC_sendToMCM to the same value.
	if (initializeSpoofedSoC) {
		lastSoC_sentToMCM = SoC_sendToMCM;
		initializeSpoofedSoC = false;
	}

	static int16_t packMilliAmps = adc_getLatestBatteryCurrent_amps();

	// Modify SoC_sendToMCM in place to be an increment of lastSoC_sentToMCM
	// packMilliAmps is being checked so that we only increment SoC if we have current < -0.01 Amps or decremented if we have > +0.01 Amps
	// This should prevent SoC being changed during Auto Stop.

	if ((SoC_sendToMCM > lastSoC_sentToMCM) && (packMilliAmps <= -10)) {		// packMilliAmps is - if Amps are going IN and voltage is going UP
		if(SoC_sendToMCM > (lastSoC_sentToMCM + 5)) {
			SoC_sendToMCM = (lastSoC_sentToMCM + 5);
		}
		BATTSCI_evaluateSoCBytes(SoC_sendToMCM);
		lastSoC_sentToMCM = SoC_sendToMCM;
	} else if ((SoC_sendToMCM < lastSoC_sentToMCM) && (packMilliAmps >= 10)) {  // packMilliAmps is + if Amps are going OUT and voltage is going DOWN
		if (SoC_sendToMCM < (lastSoC_sentToMCM - 5)) {
			SoC_sendToMCM = (lastSoC_sentToMCM - 5);
		}
		BATTSCI_evaluateSoCBytes(SoC_sendToMCM);
		lastSoC_sentToMCM = SoC_sendToMCM;
	}

	BATTSCI_evaluateTemperatureByte(SoC_sendToMCM);
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

    SoC_sentToMCMDelayCounter += 1;

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

	  if (((SoC_getBatteryStateNow_percent() < STACK_SoC_MIN) && (SoC_sentToMCMDelayCounter >= 171)) || (LTC68042result_loCellVoltage_get() < CELL_MIN_ALLOWED_VOLTAGE_ASSIST))
	  {
	    BATTSCI_simulateNegativeRecal();
	    BATTSCI_evaluateSoCBytes(SoC_sendToMCM);
        debugUSB_sendChar('1');
      }
      else
      { // all cells above 3.000 volts

        // The loop begins at 200 so if we need to we can force an SoC value to be held longer, such as if we need to do a positive or negative recal.
        if (SoC_sentToMCMDelayCounter >= (SoC_sentToMCMDelayIncrement + 200)) {
          BATTSCI_calculateSoC_sentToMCM();
          SoC_sentToMCMDelayCounter = 200;
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

	  frameSum_87 += BATTSCI_writeByte( SoC_Bytes[0] );
	  frameSum_87 += BATTSCI_writeByte( SoC_Bytes[1] );

      frameSum_87 += BATTSCI_writeByte( highByte(batteryCurrent_toBATTSCI << 1) & 0x7F ); //B5 Battery Current (upper byte)
      frameSum_87 += BATTSCI_writeByte(  lowByte(batteryCurrent_toBATTSCI     ) & 0x7F ); //B6 Battery Current (lower byte)
      frameSum_87 += BATTSCI_writeByte( 0x32 );                                           //B7 always 0x32, except before 0xAAbyte5 changes from 0x00 to 0x10 (then 0x23)

	  // (05 DEC 2021) To Do: Revisit this after more thorough analysis of assist and regen flags.
      frameSum_87 += BATTSCI_writeByte( temperature_Byte );                               //B8 max temp //0 degC = 30d = 0x1E
      frameSum_87 += BATTSCI_writeByte( temperature_Byte );                               //B9 min temp

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
