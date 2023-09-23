//JTS2doLater: eeprom.c isn't wrapped into MVP yet 

#include <EEPROM.h>
#include "libcm.h"

//atmega2560 has 4kB EEPROM
//eeprom  read halts CPU for QTY4 cycles
//eeprom write halts CPU for QTY2 cycles and takes ~3.3 ms to complete

//JTS2doLater: does Arduino implement 2560 brownout detector?

//store the date and time customer compiled the source code in program memory
const uint8_t COMPILE_DATE_PROGRAM[BYTES_IN_DATE] = __DATE__; //Format: 'Mmm DD YYYY' //Ex: 'Jan 23 2022' //'Mar  5 2022'
const uint8_t COMPILE_TIME_PROGRAM[BYTES_IN_TIME] = __TIME__; //Format: 'HH:MM:SS'    //Ex: '16:30:31'

const uint16_t EEPROM_LAST_USABLE_ADDRESS         = 0xF9F; //atmega2560 has 4kB EEPROM

//EEPROM address map:
const uint16_t EEPROM_ADDRESS_COMPILE_DATE        = 0x000; //EEPROM range is 0x000:0x00B (12B)
const uint16_t EEPROM_ADDRESS_HOURS_SINCE_UPDATE  = 0x00C; //EEPROM range is 0x00C:0x00D ( 2B)
const uint16_t EEPROM_ADDRESS_FIRMWARE_STATUS     = 0x00E; //EEPROM range is 0x00E:0x00E ( 1B)
const uint16_t EEPROM_ADDRESS_BATTSCI_REGEN       = 0x00F; //EEPROM range is 0x00F:0x00F ( 1B)
const uint16_t EEPROM_ADDRESS_BATTSCI_ASSIST      = 0x010; //EEPROM range is 0x010:0x010 ( 1B)
const uint16_t EEPROM_ADDRESS_KEYON_DELAY         = 0x011; //EEPROM range is 0x011:0x011 ( 1B)
const uint16_t EEPROM_ADDRESS_LOOPPERIOD_MET      = 0x012; //EEPROM range is 0x012:0x012 ( 1B)
const uint16_t EEPROM_ADDRESS_COMPILE_TIME        = 0x013; //EEPROM range is 0x013:0x01B ( 9B)
//this EEPROM space still available
const uint16_t EEPROM_ADDRESS_BATT_HISTORY = EEPROM_LAST_USABLE_ADDRESS - NUM_BYTES_BATTERY_HISTORY; //stored last
const uint16_t EEPROM_ADDRESS_BATT_HISTORY_UNINITIALIZED = EEPROM_ADDRESS_BATT_HISTORY - 1; //0xFF if updating from old version

//compile date & time stored in EEPROM the last time the firmware was updated
uint8_t compileDateEEPROM[BYTES_IN_DATE] = {}; //JTS2doLater: Move these into single function (to save RAM)
uint8_t compileTimeEEPROM[BYTES_IN_TIME] = {};

////////////////////////////////////////////////////////////////////////////////////

uint16_t readFromEEPROM_uint16(uint16_t startAddress)
{
  uint16_t valueFromEEPROM_uint16 = 0;
  valueFromEEPROM_uint16 =  ( EEPROM.read(startAddress    ) << 8 ); //retrieve upper byte
  valueFromEEPROM_uint16 += ( EEPROM.read(startAddress + 1)      ); //retrieve lower byte
}

////////////////////////////////////////////////////////////////////////////////////

void writeToEEPROM_uint16(uint16_t startAddress, uint16_t value)
{
  EEPROM.update( startAddress    , highByte(value) ); //write lower byte
  Serial.print(F("\nwriting uint16 hiByte value(DEC):"));
  Serial.print(highByte(value),DEC);
  Serial.print(F(" at address(HEX):"));
  Serial.print(startAddress,HEX);
  Serial.print(F(", Reads back as(DEC):"));
  Serial.print(EEPROM.read(startAddress));

  EEPROM.update( startAddress + 1,  lowByte(value) ); //write upper byte
  Serial.print(F("\nwriting uint16 loByte value(DEC):"));
  Serial.print(lowByte(value),DEC);
  Serial.print(F(" at address(HEX):"));
  Serial.print(startAddress,HEX);
  Serial.print(F(", Reads back as(DEC):"));
  Serial.print(EEPROM.read(startAddress+1));

  Serial.print(F("\nuint16 'value' written is(DEC):"));
  Serial.print(value);
  Serial.print(F(", reads back as(DEC):"));
  Serial.print(readFromEEPROM_uint16(startAddress));
}

////////////////////////////////////////////////////////////////////////////////////

//copy compile date and time into RAM (stored in array compileDateEEPROM[]) //Example: "Jan 23 2022"
void compileTimestamp_loadFromEEPROM(void)
{
  for(int ii = 0; ii < BYTES_IN_DATE; ii++) { compileDateEEPROM[ii] = EEPROM.read(ii + EEPROM_ADDRESS_COMPILE_DATE); }  
  for(int ii = 0; ii < BYTES_IN_TIME; ii++) { compileTimeEEPROM[ii] = EEPROM.read(ii + EEPROM_ADDRESS_COMPILE_TIME); }  
}

////////////////////////////////////////////////////////////////////////////////////

//store compile date and time into EEPROM (i.e. after the firmware is updated)
//Limit calls to this function (EEPROM has limited write lifetime)
void compileTimestamp_writeToEEPROM(void)
{
  for(int ii = 0; ii < BYTES_IN_DATE; ii++) { EEPROM.update( (ii + EEPROM_ADDRESS_COMPILE_DATE), COMPILE_DATE_PROGRAM[ii] ); }
  for(int ii = 0; ii < BYTES_IN_TIME; ii++) { EEPROM.update( (ii + EEPROM_ADDRESS_COMPILE_TIME), COMPILE_TIME_PROGRAM[ii] ); }
}

////////////////////////////////////////////////////////////////////////////////////

//compare compile date & time to last values stored in EEPROM
//if timestamps are different, then firmware was just updated
bool wasFirmwareJustUpdated(void)
{
  compileTimestamp_loadFromEEPROM(); //result stored in 'compileDateEEPROM[]' & 'compileTimeTTPROM[]'

  bool areDatesIdentical = true;

  for(uint8_t ii = 0; ii < BYTES_IN_DATE; ii++)
  {
    if(compileDateEEPROM[ii] != COMPILE_DATE_PROGRAM[ii]) { areDatesIdentical = false; }
  }

  for(uint8_t ii = 0; ii < BYTES_IN_TIME; ii++)
  {
    if(compileTimeEEPROM[ii] != COMPILE_TIME_PROGRAM[ii]) { areDatesIdentical = false; }
  }
  
  if(areDatesIdentical == true) { return false; } //firmware NOT updated
  else                          { return  true; } //firmware was updated
}

////////////////////////////////////////////////////////////////////////////////////

uint16_t eeprom_uptimeStoredInEEPROM_hours_get(void)
{
  return readFromEEPROM_uint16(EEPROM_ADDRESS_HOURS_SINCE_UPDATE);
}

////////////////////////////////////////////////////////////////////////////////////

//Limit calls to this function (EEPROM has limited write lifetime)
void uptimeStoredInEEPROM_hours_set(uint16_t hourCount)
{
	writeToEEPROM_uint16(EEPROM_ADDRESS_HOURS_SINCE_UPDATE, hourCount);
}

////////////////////////////////////////////////////////////////////////////////////

//Takes 4 clock cycles.  EEPROM read limit: infinite
uint8_t eeprom_expirationStatus_get(void)
{ 
  //structured this way to prevent EEPROM read/write failures from disabling LiBCM
  if( (EEPROM.read(EEPROM_ADDRESS_FIRMWARE_STATUS)) == FIRMWARE_EXPIRED ) { return FIRMWARE_EXPIRED;   }
  else                                                                    { return FIRMWARE_UNEXPIRED; }          
}

////////////////////////////////////////////////////////////////////////////////////

void EEPROM_expirationStatus_set(uint8_t newFirmwareStatus) { EEPROM.update(EEPROM_ADDRESS_FIRMWARE_STATUS, newFirmwareStatus); }

////////////////////////////////////////////////////////////////////////////////////

//JTS2doNow: rewrite this function so it can store more than 2^32-1 millis()
//add value stored in EEPROM (from last keyOFF event) to previous keyOFF time
uint16_t EEPROM_calculateTotalHoursSinceLastFirmwareUpdate(void)
{
  uint32_t timeSincePreviousKeyOff_ms = millis() - key_latestTurnOffTime_ms_get();
  uint16_t timeSincePreviousKeyOff_hours = (uint16_t)(timeSincePreviousKeyOff_ms / MILLISECONDS_PER_HOUR);
  
  uint16_t totalHours = eeprom_uptimeStoredInEEPROM_hours_get() + timeSincePreviousKeyOff_hours;
  if(totalHours > REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS) { totalHours = REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS; } //coerce

  return totalHours;
}

////////////////////////////////////////////////////////////////////////////////////

void eeprom_checkForExpiredFirmware(void)
{ 
	if(wasFirmwareJustUpdated() == true)
  {

  	//user just updated the firmware, so...
    uptimeStoredInEEPROM_hours_set(0); //reset hour counter to zero
    compileTimestamp_writeToEEPROM(); //store new compile date in EEPROM (so we can compare again on future keyOFF events)
    EEPROM_expirationStatus_set(FIRMWARE_UNEXPIRED);

    Serial.print(F("\nFirmwareUpdated"));
    gpio_playSound_firmwareUpdated();
  }
  else //user didn't update the firmware
  { 
    uint16_t newUptime_hours = EEPROM_calculateTotalHoursSinceLastFirmwareUpdate();
    Serial.print(F("\nnewUpdate_hours:"));
    Serial.print(newUptime_hours,DEC);
    uptimeStoredInEEPROM_hours_set(newUptime_hours); //store new total uptime in EEPROM

    Serial.print(F("\nTotal hours since firmware last uploaded: "));
    if(newUptime_hours == REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS) //newUptime_hours is bounded to REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS
    {
      Serial.print(F("EXPIRED\nOpen Beta ALERT: Firmware update required (linsight.org/downloads)\nLiBCM disabled until firmware is updated"));
      EEPROM_expirationStatus_set(FIRMWARE_EXPIRED);
    }  
    else //firmware not expired
    {
      Serial.print(newUptime_hours);
      Serial.print(F(" ("));
      Serial.print(REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS);
      Serial.print(F( " Hours MAX)" ));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////

uint8_t eeprom_hasLibcmDisabledAssist_get(void) { return EEPROM.read(EEPROM_ADDRESS_BATTSCI_ASSIST); }
void    eeprom_hasLibcmDisabledAssist_set(uint8_t wasAssistLimited) { EEPROM.update(EEPROM_ADDRESS_BATTSCI_ASSIST, wasAssistLimited); }

uint8_t eeprom_hasLibcmDisabledRegen_get(void) { return EEPROM.read(EEPROM_ADDRESS_BATTSCI_REGEN); }
void    eeprom_hasLibcmDisabledRegen_set(uint8_t wasRegenLimited) { EEPROM.update(EEPROM_ADDRESS_BATTSCI_REGEN, wasRegenLimited); }

////////////////////////////////////////////////////////////////////////////////////

uint8_t eeprom_delayKeyON_ms_get(void) { return EEPROM.read(EEPROM_ADDRESS_KEYON_DELAY); }
void    eeprom_delayKeyON_ms_set(uint8_t delay_ms) { EEPROM.update(EEPROM_ADDRESS_KEYON_DELAY, delay_ms); }

////////////////////////////////////////////////////////////////////////////////////

uint8_t eeprom_hasLibcmFailedTiming_get(void) { return EEPROM.read(EEPROM_ADDRESS_LOOPPERIOD_MET); }
void    eeprom_hasLibcmFailedTiming_set(uint8_t timing) { EEPROM.update(EEPROM_ADDRESS_LOOPPERIOD_MET, timing); }

////////////////////////////////////////////////////////////////////////////////////

void eeprom_verifyDataValid(void)
{
  //verify all runtime-configurable data stored in EEPROM is valid.  If not, load default value(s)
  if( !( (eeprom_hasLibcmDisabledRegen_get() == EEPROM_LICBM_DISABLED_REGEN) || (eeprom_hasLibcmDisabledRegen_get() == EEPROM_REGEN_NEVER_LIMITED) ) )
  {
    //invalid data in EEPROM
    Serial.print(F("\nRestoring EEPROM value: EEPROM_LIBCM_DISABLED_REGEN"));
    eeprom_hasLibcmDisabledRegen_set(EEPROM_REGEN_NEVER_LIMITED);
  }

  if( !( (eeprom_hasLibcmDisabledAssist_get() == EEPROM_LICBM_DISABLED_ASSIST) || (eeprom_hasLibcmDisabledAssist_get() == EEPROM_ASSIST_NEVER_LIMITED) ) )
  {
    //invalid data in EEPROM
    Serial.print(F("\nRestoring EEPROM value: EEPROM_LIBCM_DISABLED_ASSIST"));
    eeprom_hasLibcmDisabledAssist_set(EEPROM_ASSIST_NEVER_LIMITED);
  }

  if(eeprom_delayKeyON_ms_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE)
  {
    Serial.print(F("\nRestoring EEPROM value: EEPROM_ADDRESS_KEYON_DELAY"));
    eeprom_delayKeyON_ms_set(0);
  }

  if( !( (eeprom_hasLibcmFailedTiming_get() == EEPROM_LIBCM_LOOPPERIOD_MET) || (eeprom_hasLibcmFailedTiming_get() == EEPROM_LIBCM_LOOPPERIOD_EXCEEDED) ) )
  {
    //invalid data in EEPROM
    Serial.print(F("\nRestoring EEPROM value: EEPROM_LIBCM_LOOPPERIOD_MET"));
    eeprom_hasLibcmDisabledRegen_set(EEPROM_LIBCM_LOOPPERIOD_MET);
  }
}

////////////////////////////////////////////////////////////////////////////////////

void eeprom_resetDebugValues(void)
{
  eeprom_hasLibcmDisabledRegen_set(EEPROM_REGEN_NEVER_LIMITED);
  eeprom_hasLibcmDisabledAssist_set(EEPROM_ASSIST_NEVER_LIMITED);
  eeprom_hasLibcmFailedTiming_set(EEPROM_LIBCM_LOOPPERIOD_MET);
}

////////////////////////////////////////////////////////////////////////////////////

uint16_t convertTemperatureAndSoC_toAddress(uint8_t indexTemperature, uint8_t indexSoC)
{
  uint16_t address = (indexTemperature * TOTAL_TEMP_BINS + indexSoC) * NUM_BYTES_PER_BIN + EEPROM_ADDRESS_BATT_HISTORY;

  return address;
}

////////////////////////////////////////////////////////////////////////////////////

uint16_t eeprom_batteryHistory_getValue(uint8_t indexTemperature, uint8_t indexSoC)
{
  uint16_t address = convertTemperatureAndSoC_toAddress(indexTemperature, indexSoC);

  return readFromEEPROM_uint16(address);
}

////////////////////////////////////////////////////////////////////////////////////

void eeprom_batteryHistory_incrementValue(uint8_t indexTemperature, uint8_t indexSoC)
{
  uint16_t address = convertTemperatureAndSoC_toAddress(indexTemperature, indexSoC);
  Serial.print(F("\nEEPROM addres 0x"));
  Serial.print(address,HEX);

  uint16_t existingValue = readFromEEPROM_uint16(address);
  Serial.print(F("\nExistingValue:"));
  Serial.print(existingValue,HEX);

  if(existingValue != 65535) { writeToEEPROM_uint16(address, existingValue + 1); }
}

////////////////////////////////////////////////////////////////////////////////////

void eeprom_batteryHistory_reset(void)
{
  uint16_t minAddress = EEPROM_ADDRESS_BATT_HISTORY;
  uint16_t maxAddress = EEPROM_ADDRESS_BATT_HISTORY + NUM_BYTES_BATTERY_HISTORY;
  
  Serial.print(F("\nInitializing battery history"));

  for(uint16_t address=minAddress; address<=maxAddress; address++)
  {
    EEPROM.update(address, EEPROM_ADDRESS_FORMATTED_VALUE);
  }

}

////////////////////////////////////////////////////////////////////////////////////

void eeprom_begin(void)
{
  eeprom_verifyDataValid();

  if(EEPROM.read(EEPROM_ADDRESS_BATT_HISTORY_UNINITIALIZED) == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE)
  {
    eeprom_batteryHistory_reset();

    EEPROM.update(EEPROM_ADDRESS_BATT_HISTORY_UNINITIALIZED, EEPROM_ADDRESS_FORMATTED_VALUE);
  }
}

