//JTS2doLater: eeprom.c isn't wrapped into MVP yet 

#include <EEPROM.h>
#include "libcm.h"

#define BYTES_IN_DATE 12

//store the day customer compiled the source code in program memory
const uint8_t COMPILE_DATE_PROGRAM[BYTES_IN_DATE]= __DATE__; //Format: Mmm DD YYYY //Ex: Jan 23 2022 //Mar  5 2022

//EEPROM address map:
const uint16_t EEPROM_ADDRESS_COMPILE_DATE       = 0x000; //EEPROM range is 0x000:0x00B (12B)
const uint16_t EEPROM_ADDRESS_HOURS_SINCE_UPDATE = 0x00C; //EEPROM range is 0x00C:0x00D ( 2B)
const uint16_t EEPROM_ADDRESS_FIRMWARE_STATUS    = 0x00E; //EEPROM range is 0x00E:0x00E ( 1B)
const uint16_t EEPROM_ADDRESS_BATTSCI_REGEN      = 0x00F; //EEPROM range is 0x00F:0x00F ( 1B)
const uint16_t EEPROM_ADDRESS_BATTSCI_ASSIST     = 0x010; //EEPROM range is 0x010:0x010 ( 1B)
const uint16_t EEPROM_ADDRESS_KEYON_DELAY        = 0x011; //EEPROM range is 0x011:0x011 ( 1B)
const uint16_t EEPROM_ADDRESS_LOOPRATE_TIMING    = 0x012; //EEPROM range is 0x012:0x012 ( 1B)

//compile date previously stored in EEPROM (the last time the firmware was updated)
uint8_t compileDateEEPROM[BYTES_IN_DATE] = {};

////////////////////////////////////////////////////////////////////////////////////

//copy compile date into RAM (stored in array compileDateEEPROM[]) //Example: "Jan 23 2022"
//t =
void compileDateStoredInEEPROM_get(void)
{
  for(int ii = 0; ii < BYTES_IN_DATE; ii++) { compileDateEEPROM[ii] = EEPROM.read(ii + EEPROM_ADDRESS_COMPILE_DATE); }  
}

////////////////////////////////////////////////////////////////////////////////////

//store compile date into EEPROM (i.e. after the firmware is updated)
//Limit calls to this function (EEPROM has limited write lifetime)
void compileDateStoredInEEPROM_set(void)
{
  for(int ii = 0; ii < BYTES_IN_DATE; ii++) { EEPROM.update( (ii + EEPROM_ADDRESS_COMPILE_DATE), COMPILE_DATE_PROGRAM[ii] ); }
}

////////////////////////////////////////////////////////////////////////////////////

//compare compile date to last date stored in EEPROM
//if dates are different, then firmware was updated since the last keyOFF event
bool wasFirmwareJustUpdated(void)
{
  compileDateStoredInEEPROM_get(); //result stored in array 'compileDateEEPROM[]' (in RAM)

  bool areDatesIdentical = true;

  for(uint8_t ii = 0; ii < BYTES_IN_DATE; ii++)
  {
    if(compileDateEEPROM[ii] != COMPILE_DATE_PROGRAM[ii]) { areDatesIdentical = false; }
  }
  if(areDatesIdentical == true) { return false; } //firmware NOT updated
  else                          { return  true; } //firmware was updated
}

////////////////////////////////////////////////////////////////////////////////////

uint16_t EEPROM_uptimeStoredInEEPROM_hours_get(void)
{
	uint16_t hoursSinceLastFirmwareUpdate = 0;

	//read uint16_t over two calls
  hoursSinceLastFirmwareUpdate  = ( EEPROM.read(EEPROM_ADDRESS_HOURS_SINCE_UPDATE    ) << 8 ); //retrieve upper byte
  hoursSinceLastFirmwareUpdate += ( EEPROM.read(EEPROM_ADDRESS_HOURS_SINCE_UPDATE + 1)      ); //retrieve lower byte

  return hoursSinceLastFirmwareUpdate;
}

////////////////////////////////////////////////////////////////////////////////////

//Limit calls to this function (EEPROM has limited write lifetime)
void uptimeStoredInEEPROM_hours_set(uint16_t hourCount)
{
	EEPROM.update( EEPROM_ADDRESS_HOURS_SINCE_UPDATE    , highByte(hourCount) ); //write lower byte
	EEPROM.update( EEPROM_ADDRESS_HOURS_SINCE_UPDATE + 1,  lowByte(hourCount) ); //write upper byte
}

////////////////////////////////////////////////////////////////////////////////////

//Takes 4 clock cycles.  EEPROM read limit: infinite
uint8_t EEPROM_firmwareStatus_get(void)
{ 
  //structured this way to prevent EEPROM read/write failures from disabling LiBCM
  if( (EEPROM.read(EEPROM_ADDRESS_FIRMWARE_STATUS)) == FIRMWARE_STATUS_EXPIRED ) { return FIRMWARE_STATUS_EXPIRED; }
  else                                                                           { return FIRMWARE_STATUS_VALID  ; }          
}

////////////////////////////////////////////////////////////////////////////////////

void EEPROM_firmwareStatus_set(uint8_t newFirmwareStatus) { EEPROM.update(EEPROM_ADDRESS_FIRMWARE_STATUS, newFirmwareStatus); }

////////////////////////////////////////////////////////////////////////////////////

//add value stored in EEPROM (from last keyOFF event) to previous keyOFF time
uint16_t EEPROM_calculateTotalHoursSinceLastFirmwareUpdate(void)
{
  #define MILLISECONDS_PER_HOUR 3600000

  uint32_t timeSincePreviousKeyOff_ms = millis() - key_latestTurnOffTime_ms_get();
  uint16_t timeSincePreviousKeyOff_hours = (uint16_t)(timeSincePreviousKeyOff_ms / MILLISECONDS_PER_HOUR);
  
  uint16_t totalHours = EEPROM_uptimeStoredInEEPROM_hours_get() + timeSincePreviousKeyOff_hours;
  if(totalHours > REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS) { totalHours = REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS; } //coerce

  return totalHours;
}

////////////////////////////////////////////////////////////////////////////////////

//this function does not return if firmware is too old (e.g. after 40 days)
void EEPROM_checkForExpiredFirmware(void)
{ 
	if(wasFirmwareJustUpdated() == true)
  {
  	//user recently updated the firmware, so...
    uptimeStoredInEEPROM_hours_set(0); //reset hour counter to zero
    compileDateStoredInEEPROM_set(); //store new compile date in EEPROM (so we can compare again on future keyOFF events)
    EEPROM_firmwareStatus_set(FIRMWARE_STATUS_VALID); //prevent P1648 at keyON

    Serial.print(F("\nFirmwareUpdated"));
    gpio_playSound_firmwareUpdated();
  }
  else //user didn't update the firmware
  { 
    uint16_t newUptime_hours = EEPROM_calculateTotalHoursSinceLastFirmwareUpdate(); 
    uptimeStoredInEEPROM_hours_set(newUptime_hours); //store new total uptime in EEPROM

    //Displays total hours on this FW version and maximum hours allowed per version
    Serial.print(F("\nTotal hours since firmware last uploaded: "));
    if(newUptime_hours == REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS) { Serial.print(F("EXPIRED"));    }
    else                                                         { Serial.print(newUptime_hours); }
    Serial.print(F(" (")); Serial.print(REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS); Serial.print(F( " Hours MAX)" )); 

    if(newUptime_hours == REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS) //newUptime_hours is bounded to REQUIRED_FW_UPDATE_PERIOD_HOURS 
    {
      Serial.print(F("\nOpen Beta ALERT: Firmware update required (linsight.org/downloads)\nLiBCM disabled until firmware is updated"));
      lcd_Warning_firmwareUpdate(); 
      EEPROM_firmwareStatus_set(FIRMWARE_STATUS_EXPIRED);
      delay(5000); //give user time to read display
    } 
  }
}

////////////////////////////////////////////////////////////////////////////////////

uint8_t EEPROM_hasLibcmDisabledAssist_get(void) { return EEPROM.read(EEPROM_ADDRESS_BATTSCI_ASSIST); }
void    EEPROM_hasLibcmDisabledAssist_set(uint8_t wasAssistLimited) { EEPROM.update(EEPROM_ADDRESS_BATTSCI_ASSIST, wasAssistLimited); }

uint8_t EEPROM_hasLibcmDisabledRegen_get(void) { return EEPROM.read(EEPROM_ADDRESS_BATTSCI_REGEN); }
void    EEPROM_hasLibcmDisabledRegen_set(uint8_t wasRegenLimited) { EEPROM.update(EEPROM_ADDRESS_BATTSCI_REGEN, wasRegenLimited); }

////////////////////////////////////////////////////////////////////////////////////

uint8_t EEPROM_delayKeyON_ms_get(void) { return EEPROM.read(EEPROM_ADDRESS_KEYON_DELAY); }
void    EEPROM_delayKeyON_ms_set(uint8_t delay_ms) { EEPROM.update(EEPROM_ADDRESS_KEYON_DELAY, delay_ms); }

////////////////////////////////////////////////////////////////////////////////////

uint8_t EEPROM_hasLibcmFailedTiming_get(void) { return EEPROM.read(EEPROM_ADDRESS_LOOPRATE_TIMING); }
void    EEPROM_hasLibcmFailedTiming_set(uint8_t timing) { EEPROM.update(EEPROM_ADDRESS_LOOPRATE_TIMING, timing); Serial.print('*'); }

////////////////////////////////////////////////////////////////////////////////////

void EEPROM_verifyDataValid(void)
{
  //verify all runtime-configurable data stored in EEPROM is valid.  If not, load default value(s)
  if( !( (EEPROM_hasLibcmDisabledRegen_get() == EEPROM_LICBM_DISABLED_REGEN) || (EEPROM_hasLibcmDisabledRegen_get() == EEPROM_REGEN_NEVER_LIMITED) ) )
  {
    //invalid data in EEPROM
    Serial.print(F("\nRestoring EEPROM value: EEPROM_LIBCM_DISABLED_REGEN"));
    EEPROM_hasLibcmDisabledRegen_set(EEPROM_REGEN_NEVER_LIMITED);
  }

  if( !( (EEPROM_hasLibcmDisabledAssist_get() == EEPROM_LICBM_DISABLED_ASSIST) || (EEPROM_hasLibcmDisabledAssist_get() == EEPROM_ASSIST_NEVER_LIMITED) ) )
  {
    //invalid data in EEPROM
    Serial.print(F("\nRestoring EEPROM value: EEPROM_LIBCM_DISABLED_ASSIST"));
    EEPROM_hasLibcmDisabledAssist_set(EEPROM_ASSIST_NEVER_LIMITED);
  }

  if(EEPROM_delayKeyON_ms_get() == EEPROM_ADDRESS_FACTORY_VALUE)
  {
    Serial.print(F("\nRestoring EEPROM value: EEPROM_ADDRESS_KEYON_DELAY"));
    EEPROM_delayKeyON_ms_set(0);
  }

  if( !( (EEPROM_hasLibcmFailedTiming_get() == EEPROM_LIBCM_LOOPRATE_MET) || (EEPROM_hasLibcmFailedTiming_get() == EEPROM_LIBCM_LOOPRATE_EXCEEDED) ) )
  {
    //invalid data in EEPROM
    Serial.print(F("\nRestoring EEPROM value: EEPROM_LIBCM_LOOPRATE_MET"));
    EEPROM_hasLibcmDisabledRegen_set(EEPROM_LIBCM_LOOPRATE_MET);
  }
}

////////////////////////////////////////////////////////////////////////////////////

void EEPROM_resetDebugValues(void)
{
  EEPROM_hasLibcmDisabledRegen_set(EEPROM_REGEN_NEVER_LIMITED);
  EEPROM_hasLibcmDisabledAssist_set(EEPROM_ASSIST_NEVER_LIMITED);
  EEPROM_hasLibcmFailedTiming_set(EEPROM_LIBCM_LOOPRATE_MET);
}