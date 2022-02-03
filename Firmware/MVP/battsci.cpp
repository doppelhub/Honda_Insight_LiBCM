//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//BATTSCI Serial Functions

/************************************************************************************************************************
 * The BCM constantly sends two different 12 Byte frames to the MCM.
 *
 * frame syntax is documented in function "BATTSCI_sendFrames"
 ************************************************************************************************************************/

#include "libcm.h"

uint8_t spoofedVoltageToSend_Counts = 0; //formatted as MCM expects to see it (Vpack / 2) //2 volts per count
int16_t spoofedCurrentToSend_Counts = 0; //formatted as MCM expects to see it (2048 - amps * 20) //50 mA per count

//LUT remaps actual lithium battery SoC (unit: percent) to mimic OEM NiMH behavior (unit: deciPercent)
//input: actual lithium SoC (unit: percent integer)
//output: OEM NiMH SoC equivalent (unit: decipercent integer)
//Example: if the actual lithium SoC is 85%, then "remap_actualToSpoofedSoC[85]" will return 799d (79.9%), which is the value to send on BATTSCI
const uint16_t remap_actualToSpoofedSoC[101] = {
	  0, 22, 44, 67, 89,111,133,156,178,190, //LiCBM SoC = 00% to 09%
	200,209,217,225,232,240,248,256,264,272, //LiCBM SoC = 10% to 19%
	279,287,295,303,311,319,326,334,342,350, //LiCBM SoC = 20% to 29% //MCM enables heavy regen below 350 (35.0%)
	355,363,375,387,399,411,423,435,447,459, //LiCBM SoC = 30% to 39% //MCM enables light regen below 700 (70.0%)
	471,483,495,507,519,532,544,556,568,580, //LiCBM SoC = 40% to 49%
	592,604,616,628,640,652,664,676,688,700, //LiCBM SoC = 50% to 59% //MCM disables background regen agove 582 (58.2%)
	701,705,709,713,717,721,725,728,732,736, //LiCBM SoC = 60% to 69% //TODO_NATALYA (not urgent as of 2022JAN21) drive and eval regen behaviour to see if LiBCM 60 to 69 needs to be remapped to a smaller MCM range of 69 to 72, or if this range needs to begin at LiBCM 60 = MCM 68% instead of 70%
	740,744,748,752,756,760,764,768,772,775, //LiCBM SoC = 70% to 79%
	779,783,787,791,795,799,800,814,829,843, //LiCBM SoC = 80% to 89% //MCM disables regen above 800 (80.0%)
	857,871,886,900,914,929,943,957,971,986, //LiCBM SoC = 90% to 99%
	1000,                                    //LiCBM SoC = 100%
};  //Data empirically gathered from OEM NiMH IMA system //see ../Firmware/Prototype Building Blocks/Remap SoC.ods for calculations

uint16_t previousOutputSoC_deciPercent = remap_actualToSpoofedSoC[SoC_getBatteryStateNow_percent()];


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

void BATTSCI_enable(void) {
	digitalWrite(PIN_BATTSCI_DE,HIGH);
	previousOutputSoC_deciPercent = remap_actualToSpoofedSoC[SoC_getBatteryStateNow_percent()]; // If user grid charged over night SoC may have changed a lot.
	
  //JTS: Don't want to overload serial buffer on cold boot (will cause check engine light)
  //Serial.print(F("\nLiBCM SoC: "));
  //Serial.print(String(SoC_getBatteryStateNow_percent()));
	//Serial.print('%');
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_disable(void) { digitalWrite(PIN_BATTSCI_DE,LOW); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_bytesAvailableForWrite(void) { return Serial2.availableForWrite(); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_writeByte(uint8_t data) { Serial2.write(data); return data; }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Convert battery voltage (unit: volts) into BATTSCI format (unit: 2 volts per count)
void BATTSCI_setPackVoltage(uint8_t spoofedVoltage) { spoofedVoltageToSend_Counts = (spoofedVoltage >> 1); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Convert battery current (unit: amps) into BATTSCI format (unit: 50 mA per count)
void BATTSCI_setSpoofedCurrent(int16_t spoofedCurrent) { spoofedCurrentToSend_Counts = (2048 - (spoofedCurrent * 20)); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_calculateChecksum( uint8_t frameSum )
{
  uint8_t twosComplement = (~frameSum) + 1;
  return (twosComplement & 0x7F);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BATTSCI_calculateTemperatureByte(void)
{
  #define BATTSCI_TEMP_OFFSET 30 //MCM subtracts this value from received byte to determine temperature in degrees celcius
  #define BATTSCI_TEMP_21DEGC (21 + BATTSCI_TEMP_OFFSET) //Lowest temp LiBCM will ever send to MCM

  uint8_t tempBATTSCI = temperature_battery_getLatest() + BATTSCI_TEMP_OFFSET;
  if(tempBATTSCI < BATTSCI_TEMP_21DEGC) { tempBATTSCI = BATTSCI_TEMP_21DEGC; } //spoof temps below 21 degC to 21 degC //allows IMA start and max assist

  //JTS2doNow: EHW5 power density drops off below freezing... need to spoof lower temperatures to limit assist at cold temperatures.

  return tempBATTSCI;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//calculate cell voltage measurement offet caused by equivalent series resistance (ESR)
//returns ESR-based voltage offset in counts
//each count is 100 uV (e.g. -01234 counts = -123.4 mV)
//returns positive number during assist //Example @ +140 amps assist: 140 * 20 = +2800 counts = +0.2800 volts
//returns negative number during regen  //Example @ -070 amps  regen: -70 * 20 = -1400 counts = -0.1400 volts
int16_t cellVoltageOffsetDueToESR(void)
{
  #define CELL_ESR_mOHM 2
  #define CELL_ESR_COUNTS (CELL_ESR_mOHM * 10) //ten counts is one mOhm

  //Derivation:
  //  vCellCorrection_ESR = Icell_amps                * ESR
  //  vCellCorrection_ESR = Icell_amps                *  2 mOhm
  //  vCellCorrection_ESR = Icell_amps                * 20 counts
  return (int16_t)(adc_getLatestBatteryCurrent_amps() * CELL_ESR_COUNTS);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool BATTSCI_isPackFull(void)
{
  // uint16_t currentAdjustedCellVoltage_max = CELL_VMAX_REGEN;
  // JTS2doNow: Determine regen cell ESR
  // if(adc_getLatestBatteryCurrent_amps() < 0)
  // {
  //   //regen, need to account for ESR-related cell voltage increase
  //   currentAdjustedCellVoltage_max = CELL_VREST_85_PERCENT_SoC - cellVoltageOffsetDueToESR(); //fcn returns negative number (- - = +)
  // }

  if( (LTC68042result_hiCellVoltage_get() < CELL_VMAX_REGEN               ) && //below hard voltage limit (if SoC estimator is wrong)
      //(LTC68042result_hiCellVoltage_get() < currentAdjustedCellVoltage_max) && //below ESR-adjusted voltage limit (due to IMA current) //JTS2doNow: required?
      (  SoC_getBatteryStateNow_percent() < STACK_SoC_MAX                 ) )  //below SoC limit
       { return false; } //pack is good
  else { return true;  } //pack is overcharged
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool BATTSCI_isPackEmpty(void)
{
  uint16_t currentAdjusted_Vmin = CELL_VMIN_ASSIST;

  //account for additional cell voltage drop during assist
  //if(adc_getLatestBatteryCurrent_amps() > 0) { currentAdjusted_Vmin = CELL_VMIN_ASSIST - cellVoltageOffsetDueToESR(); }

  if( (LTC68042result_loCellVoltage_get() > CELL_VMIN_ASSIST    ) && //above hard voltage limit (if SoC estimator is wrong)
      //(LTC68042result_loCellVoltage_get() > currentAdjusted_Vmin) && //above ESR-adjusted voltage limit (due to IMA current)
      (  SoC_getBatteryStateNow_percent() > STACK_SoC_MIN       ) )  //above SoC limit
       { return false; } //pack is good
  else { return true;  } //pack is undercharged
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//sternly demand no regen and/or assist from MCM
uint8_t BATTSCI_calculateRegenAssistFlags(void)
{
  #define BATTSCI_DISABLE_ASSIST_FLAG 0x10
  #define BATTSCI_DISABLE_REGEN_FLAG  0x20

  uint8_t flags = 0;

  if(BATTSCI_isPackEmpty == true) { flags |= BATTSCI_DISABLE_ASSIST_FLAG; }
  if(BATTSCI_isPackFull  == true) { flags |= BATTSCI_DISABLE_REGEN_FLAG;  }

  return flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//JTS2doNow: Change negative logic to positive logic (less confusing)

// Byte06 ("charge request byte"):
// Initial keyON values (first ~100 BATTSCI frames, ~7 seconds... until METSCI B4 is neither 24 nor 0):
// 0x00 = 00d = 0b0000 0000: pack 'good'
// 0x01 = 01d = 0b0000 0001: ??? See "Electronics/Data from OEM../Day 1/Day1Analysis.ods>Day1-5"
// 0x20 = 32d = 0b0010 0000: pack empty
// 0x21 = 33d = 0b0010 0001: ??? See "Electronics/Data from OEM../Day 1/Day1Analysis.ods>Day1-6"
// 0x40 = 64d = 0b0100 0000: pack full

// Afterwards, this becomes:
// 0x12 = 18d = 0b0001 0010: pack 'good'
// 0x32 = 50d = 0b0011 0010: pack empty
// 0x52 = 82d = 0b0101 0010: pack full (usually... see "Day1-1" for case where pack is empty)

//kindly request regen and/or no regen from MCM
uint8_t BATTSCI_calculateChargeRequestByte(void)
{
  #define BATTSCI_IMA_START_ALLOWED     0x40 //use IMA to start engine
  #define BATTSCI_IMA_START_DISABLED    0x20 //use backup starter
  #define BATTSCI_NO_CHARGE_REQUEST     0x12 //engine started
  #define BATTSCI_REQUEST_REGEN_FLAG    0x20 //request strong background regen
  #define BATTSCI_REQUEST_NO_REGEN_FLAG 0x40 //request no background regen

  //has driver started car yet?
  if( (METSCI_getPacketB4() == 24) || (METSCI_getPacketB4() == 0) ) //if B4 packet is either 0 or 24, then car isn't started
  {
    //car isn't started (key turned to 'ON' position, but not yet to 'START' position)

    //is IMA battery charged enough for IMA start?
    if(BATTSCI_isPackEmpty() == true) { return BATTSCI_IMA_START_DISABLED; } //pack empty
    //JTS2doLater: 0x00 is a third case... probably means "pack neither empty nor full"
    else                              { return BATTSCI_IMA_START_ALLOWED;  } //pack charged enough for IMA start
  }
  else
  {
    //car is started (key previously turned to 'START', presently in 'ON' position)

    uint8_t chargeRequestByte = BATTSCI_NO_CHARGE_REQUEST;

    if(BATTSCI_isPackEmpty() == true) { chargeRequestByte |= BATTSCI_REQUEST_REGEN_FLAG; }
    if(BATTSCI_isPackFull()  == true) { chargeRequestByte |= BATTSCI_REQUEST_NO_REGEN_FLAG ; }

    return chargeRequestByte;
  }

  //code should never get here
  return BATTSCI_NO_CHARGE_REQUEST;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Adjust final SoC value as needed to improve driving characteristics
uint16_t BATTSCI_SoC_Hysteresis(uint16_t SoC_mappedToMCM_deciPercent)
{
  if (SoC_mappedToMCM_deciPercent > previousOutputSoC_deciPercent)
  {
    SoC_mappedToMCM_deciPercent = previousOutputSoC_deciPercent + 1;
  }
  else if (SoC_mappedToMCM_deciPercent < previousOutputSoC_deciPercent)
  {
	  SoC_mappedToMCM_deciPercent = previousOutputSoC_deciPercent - 1;
  }

  previousOutputSoC_deciPercent = SoC_mappedToMCM_deciPercent;

  #ifdef REDUCE_BACKGROUND_REGEN_UNLESS_BRAKING
    if( (SoC_mappedToMCM_deciPercent < 720) && (SoC_mappedToMCM_deciPercent > 250) ) { SoC_mappedToMCM_deciPercent = 720; }
    //JTS2doNow: Need to increase spoofed pack voltage when this mode is activated
    //           Otherwise, MCM ignores LiBCM request when pack is discharged
    //           Need to verify MCM honors BATTSCI "disable assist" flag (so pack isn't over-discharged)

  #endif

  return SoC_mappedToMCM_deciPercent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Convert spoofed SoC value (unit: deciPercent) into BATTSCI data (two bytes)
uint16_t BATTSCI_convertSoC_deciPercent_toBytes(uint16_t SoC_deciPercent) //deciPercent = % * 10 //Example: 72.1% = 721 deciPercent
{
  //Useful BATTSCI SoC values:
  //  80% = 0x16 0x20 -- Regen disabled, Assist allowed       , Background charge disabled
  //  72% = 0x15 0x50 -- Regen allowed , Assist allowed       , Background charge disabled
  //  60% = 0x14 0x58 -- Regen allowed , Assist allowed       , Background charge enabled
  //  40% = 0x13 0x10 -- Regen allowed , Assist allowed       , Background charge more aggressive
  //  25% = 0x11 0x7A -- Regen allowed , Assist barely allowed, Background charge very aggressive
  //  20% = 0x11 0x48 -- Regen allowed , Assist disabled      , Background charge runs even during idle
  //  20% = 0x21 0x48 -- THIS IS ALSO 20% (upper byte's upper nibble contains unknown flags)

  //SoC is sent over BATTSCI as two bytes:
  //each count in the lower byte is 0.1%, with a range from 0 (0x00) to 127 (0x7F) (0.0 to 12.7% SoC).
  //each count in the upper byte's lower nibble represents 12.8% SoC.
  //              the upper byte's upper nibble has several unknown flags, and is either 0b0001 (usually) or 0b0010 (sometimes)

  //Example: convert BATTSCI(two bytes) to SoC(%)
  //  given BATTSCI bytes:     0x15 0x50 = 21d 81d = 0b00010101 0b01010001
  //  mask out upper byte's upper nibble = 05d 80d = 0b00000101 0b01010001
  //  upperByte_SoC is 05d * 12.8% = 64.0%
  //  lowerByte_SoC is 81d *  0.1% =  8.1%
  //  totalSoC = 64.0% + 8.1% = 72.1%

  //Example: convert SoC(%) to BATTSCI(two bytes)
  //  given totalSoC: 72.1% = 721d
  //  upperByteTemp = (721d / 128d) = 05d = 0b00000101 //each count in upper byte is 12.8% (128d)
  //  lowerByte = totalSoC - (upperByteTemp * 128d) = 721d - (05d * 128d) = 721d - 640d = 81d //subtract SoC portion represented by upper byte
  //  upperByte = (upperByteTemp | 0b00010000) = (0b00000101 | 0b00010000) = 0b00010101 = 21d //set upper nibble's unknown flag

  //This function converts SoC(deciPercent) to BATTSCI(two bytes), which is the latter example (above)
  uint8_t spoofedSoC_upperByte = (SoC_deciPercent >> 7);                        //upperByteTemp //divide by 128 (2^7)
  uint8_t spoofedSoC_LowerByte = SoC_deciPercent - (spoofedSoC_upperByte << 7); //lowerByte     //multiply by 128 (2^7)
          spoofedSoC_upperByte |= 0b00010000;                                   //upperByte     //set upper nibble flag (see above)

  return ( (uint16_t)(spoofedSoC_upperByte << 8) | (uint16_t)(spoofedSoC_LowerByte) ); //join both bytes together
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t BATTSCI_calculateSpoofedSoC(void)
{
  uint16_t SoC_toMCM_deciPercent = 0;
  if     (BATTSCI_isPackFull()  == true) { SoC_toMCM_deciPercent = 820; } //disable regen  //JTS2doNow: See if this is actually required (also sent as flag)
  else if(BATTSCI_isPackEmpty() == true) { SoC_toMCM_deciPercent = 200; } //disable assist //TODO_NATALYA (not urgent as of 2022JAN21) watch SoC gauge behaviour at 20% MCM SoC vs 19% MCM SoC to see if we should use 20% or 19%
  else { SoC_toMCM_deciPercent = remap_actualToSpoofedSoC[SoC_getBatteryStateNow_percent()]; } //get MCM-remapped SoC value

  SoC_toMCM_deciPercent = BATTSCI_SoC_Hysteresis(SoC_toMCM_deciPercent);

  return BATTSCI_convertSoC_deciPercent_toBytes(SoC_toMCM_deciPercent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BATTSCI_sendFrames(void)
{
  static uint32_t previousMillis = 0;

  if( ( BATTSCI_bytesAvailableForWrite() > BATTSCI_BYTES_IN_FRAME ) && //Verify serial send ring buffer has room
      ( (millis() - previousMillis) >= 100 )                        )  //Send a frame every 100 ms
  {
    //time to send a BATTSCI frame!
    previousMillis = millis(); //stores the next frame start time

    static uint8_t frame2send = 0x87; //stores the next frame type to send

    if(frame2send == 0x87)
    {
      //Place 0x87 frame into serial send buffer
      uint8_t frameSum_87 = 0; //this will overflow, which is ok for CRC
      frameSum_87 += BATTSCI_writeByte( 0x87 );                                              //B0 Never changes
      frameSum_87 += BATTSCI_writeByte( 0x40 );                                              //B1 Never changes
      frameSum_87 += BATTSCI_writeByte( spoofedVoltageToSend_Counts );                       //B2 Half Vbatt_actual (e.g. 0x40 = d64 = 128 V

      uint16_t spoofedSoC_Bytes = BATTSCI_calculateSpoofedSoC();
      frameSum_87 += BATTSCI_writeByte( highByte(spoofedSoC_Bytes) );                        //B3 SoC (upper byte)
      frameSum_87 += BATTSCI_writeByte(  lowByte(spoofedSoC_Bytes) );                        //B4 SoC (lower byte)

      frameSum_87 += BATTSCI_writeByte( highByte(spoofedCurrentToSend_Counts << 1) & 0x7F ); //B5 Battery Current (upper byte)
      frameSum_87 += BATTSCI_writeByte(  lowByte(spoofedCurrentToSend_Counts     ) & 0x7F ); //B6 Battery Current (lower byte)
      frameSum_87 += BATTSCI_writeByte( 0x32 );                                              //B7 always 0x32, except before 0xAAbyte5 changes from 0x00 to 0x10 (then 0x23)
      frameSum_87 += BATTSCI_writeByte( BATTSCI_calculateTemperatureByte() );                //B8 max battery module temp
      frameSum_87 += BATTSCI_writeByte( BATTSCI_calculateTemperatureByte() );                //B9 min battery module temp
      frameSum_87 += BATTSCI_writeByte( METSCI_getPacketB3() );                              //B10 MCM latest B3 data byte
                     BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_87) );            //B11 Send Checksum. sum(byte0:byte11) should equal 0
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
      frameSum_AA += BATTSCI_writeByte( BATTSCI_calculateRegenAssistFlags()  );           //B5 Disable assist/regen flags
      frameSum_AA += BATTSCI_writeByte( BATTSCI_calculateChargeRequestByte() );           //B6 Request regen/noRegen if battery low/high
      frameSum_AA += BATTSCI_writeByte( 0x61 );                                           //B7 BCM hardware/firmware version?
      frameSum_AA += BATTSCI_writeByte( highByte(spoofedCurrentToSend_Counts << 1) & 0x7F ); //B8 Battery Current (upper byte)
      frameSum_AA += BATTSCI_writeByte(  lowByte(spoofedCurrentToSend_Counts     ) & 0x7F ); //B9 Battery Current (lower byte)
      frameSum_AA += BATTSCI_writeByte( METSCI_getPacketB4() );                           //B10 MCM latest B4 data byte //
                     BATTSCI_writeByte( BATTSCI_calculateChecksum(frameSum_AA) );         //B11 Send Checksum. sum(byte0:byte11) should equal 0
      frame2send = 0x87;
    }
  }
}
