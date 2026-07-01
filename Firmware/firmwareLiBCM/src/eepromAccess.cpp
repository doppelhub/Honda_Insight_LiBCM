//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#include <EEPROM.h>
#include "libcm.h"

//atmega2560 has 4kB EEPROM
//eeprom  read halts CPU for QTY4 cycles
//eeprom write halts CPU for QTY2 cycles and takes ~3.3 ms to complete

//JTS2doLater: Store program checksum in EEPROM; verify equal on keyOff

//store the date and time customer compiled the source code in program memory
const uint8_t COMPILE_DATE_PROGRAM[BYTES_IN_DATE] = __DATE__; //Format: 'Mmm DD YYYY' //Ex: 'Jan 23 2022' //'Mar  5 2022'
const uint8_t COMPILE_TIME_PROGRAM[BYTES_IN_TIME] = __TIME__; //Format: 'HH:MM:SS'    //Ex: '16:30:31'

const uint16_t EEPROM_LAST_USABLE_ADDRESS         = 0xF9F; //atmega2560 has 4kB EEPROM

//EEPROM address map:
const uint16_t EEPROM_ADDRESS_COMPILE_DATE        = 0x000; //EEPROM range is 0x000:0x00B (12B)
const uint16_t EEPROM_ADDRESS_HOURS_SINCE_UPDATE  = 0x00C; //EEPROM range is 0x00C:0x00D ( 2B)
const uint16_t EEPROM_ADDRESS_FIRMWARE_STATUS     = 0x00E; //EEPROM range is 0x00E       ( 1B)
const uint16_t EEPROM_ADDRESS_BATTSCI_REGEN       = 0x00F; //EEPROM range is 0x00F       ( 1B)
const uint16_t EEPROM_ADDRESS_BATTSCI_ASSIST      = 0x010; //EEPROM range is 0x010       ( 1B)
const uint16_t EEPROM_ADDRESS_KEYON_DELAY         = 0x011; //EEPROM range is 0x011       ( 1B)
const uint16_t EEPROM_ADDRESS_NEXT_Wh_RECORD      = 0x012; //EEPROM range is 0x012       ( 1B)
const uint16_t EEPROM_ADDRESS_COMPILE_TIME        = 0x013; //EEPROM range is 0x013:0x01B ( 9B)
const uint16_t EEPROM_ADDRESS_MAX_VCELL_DELTA     = 0x01C; //EEPROM range is 0x01C:0x01D ( 2B)
const uint16_t EEPROM_ADDRESS_BVO_OFFSET          = 0x01E; //EEPROM range is 0x01E       ( 1B)
const uint16_t EEPROM_ADDRESS_MDV_OFFSET          = 0x01F; //EEPROM range is 0x01F       ( 1B)
const uint16_t EEPROM_ADDRESS_SPF_OFFSET          = 0x020; //EEPROM range is 0x020       ( 1B)

//config.h "sticky override" tunables (see config.h for behavior description)
const uint16_t EEPROM_ADDRESS_STACK_SoC_MAX                        = 0x021; //EEPROM range is 0x021       ( 1B)
const uint16_t EEPROM_ADDRESS_STACK_SoC_MIN                        = 0x022; //EEPROM range is 0x022       ( 1B)
const uint16_t EEPROM_ADDRESS_CELL_VMAX_REGEN                      = 0x023; //EEPROM range is 0x023:0x024 ( 2B)
const uint16_t EEPROM_ADDRESS_CELL_VMIN_ASSIST                     = 0x025; //EEPROM range is 0x025:0x026 ( 2B)
const uint16_t EEPROM_ADDRESS_CELL_VMAX_GRIDCHARGER                = 0x027; //EEPROM range is 0x027:0x028 ( 2B)
const uint16_t EEPROM_ADDRESS_CELL_VMIN_GRIDCHARGER                = 0x029; //EEPROM range is 0x029:0x02A ( 2B)
const uint16_t EEPROM_ADDRESS_CELL_VMIN_KEYOFF                     = 0x02B; //EEPROM range is 0x02B:0x02C ( 2B)
const uint16_t EEPROM_ADDRESS_CELL_BALANCE_MIN_SoC                 = 0x02D; //EEPROM range is 0x02D       ( 1B)
const uint16_t EEPROM_ADDRESS_CELL_BALANCE_MAX_TEMP_C              = 0x02E; //EEPROM range is 0x02E       ( 1B)
const uint16_t EEPROM_ADDRESS_COOL_TEMP_C_KEYOFF                   = 0x02F; //EEPROM range is 0x02F       ( 1B)
const uint16_t EEPROM_ADDRESS_COOL_TEMP_C_GRIDCHARGING             = 0x030; //EEPROM range is 0x030       ( 1B)
const uint16_t EEPROM_ADDRESS_COOL_TEMP_C_KEYON                    = 0x031; //EEPROM range is 0x031       ( 1B)
const uint16_t EEPROM_ADDRESS_HEAT_TEMP_C_KEYON                    = 0x032; //EEPROM range is 0x032       ( 1B)
const uint16_t EEPROM_ADDRESS_HEAT_TEMP_C_GRIDCHARGING             = 0x033; //EEPROM range is 0x033       ( 1B)
const uint16_t EEPROM_ADDRESS_HEAT_TEMP_C_KEYOFF                   = 0x034; //EEPROM range is 0x034       ( 1B)
const uint16_t EEPROM_ADDRESS_KEYOFF_DISABLE_THERMAL_MGMT_BELOW_SoC= 0x035; //EEPROM range is 0x035       ( 1B)
const uint16_t EEPROM_ADDRESS_POWEROFF_DELAY_PACK_EMPTY_MINUTES    = 0x036; //EEPROM range is 0x036       ( 1B)
const uint16_t EEPROM_ADDRESS_POWEROFF_DELAY_DAYS                  = 0x037; //EEPROM range is 0x037       ( 1B)
const uint16_t EEPROM_ADDRESS_DISPLAY_POSITIVE_SIGN_DURING_ASSIST  = 0x038; //EEPROM range is 0x038       ( 1B)
const uint16_t EEPROM_ADDRESS_DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE   = 0x039; //EEPROM range is 0x039:0x03A ( 2B)
const uint16_t EEPROM_ADDRESS_DISABLE_ASSIST                       = 0x03B; //EEPROM range is 0x03B       ( 1B)
const uint16_t EEPROM_ADDRESS_DISABLE_REGEN                        = 0x03C; //EEPROM range is 0x03C       ( 1B)
const uint16_t EEPROM_ADDRESS_IGNORE_CELL_VOLTAGE_MISMATCH         = 0x03D; //EEPROM range is 0x03D       ( 1B)
const uint16_t EEPROM_ADDRESS_MIN_SPOOFED_VOLTAGE_60S              = 0x03E; //EEPROM range is 0x03E       ( 1B)

//hardware-identity fields (see config.h "Hardware Specific Configuration" section) -- most have no safe default;
//EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b (0xFF) means "unconfigured", checked by eeprom_validateHardwareConfig()
const uint16_t EEPROM_ADDRESS_CURRENT_HACK                         = 0x03F; //EEPROM range is 0x03F       ( 1B)
const uint16_t EEPROM_ADDRESS_GRIDCHARGER_TYPE                     = 0x040; //EEPROM range is 0x040       ( 1B)
const uint16_t EEPROM_ADDRESS_BATTERY_TYPE                         = 0x041; //EEPROM range is 0x041       ( 1B)
const uint16_t EEPROM_ADDRESS_VOLTAGE_SPOOFING_MODE                = 0x042; //EEPROM range is 0x042       ( 1B)
const uint16_t EEPROM_ADDRESS_STACK_SIZE                           = 0x043; //EEPROM range is 0x043       ( 1B)
//this EEPROM space still available (0x044 onward)
//The following addresses start from end of EEPROM space and work backwards to beginning
const uint16_t EEPROM_ADDRESS_BATT_HISTORY        = EEPROM_LAST_USABLE_ADDRESS - NUM_BYTES_BATTERY_HISTORY;        //0xA57:0xF9F (1536B)
const uint16_t EEPROM_ADDRESS_BATT_HISTORY_UNINIT = EEPROM_ADDRESS_BATT_HISTORY - 1;                               //0xA56       (   1B)
const uint16_t EEPROM_ADDRESS_Wh_RECORDS          = EEPROM_ADDRESS_BATT_HISTORY_UNINIT - 1 - NUM_BYTES_Wh_HISTORY; //0x655:0xA55 (1024B)
const uint16_t EEPROM_ADDRESS_Wh_RECORDS_UNINIT   = EEPROM_ADDRESS_Wh_RECORDS - 1;                                 //0x654       (   1B)

//compile date & time stored in EEPROM the last time the firmware was updated
uint8_t compileDateEEPROM[BYTES_IN_DATE] = {}; //JTS2doLater: Move these into single function (to save RAM)
uint8_t compileTimeEEPROM[BYTES_IN_TIME] = {};

/////////////////////////////////////////////////////////////////////////////////////////

uint16_t readFromEEPROM_uint16(uint16_t startAddress)
{
    uint16_t valueFromEEPROM_uint16 = 0;
    valueFromEEPROM_uint16 =  ( EEPROM.read(startAddress    ) << 8 ); //retrieve upper byte
    valueFromEEPROM_uint16 += ( EEPROM.read(startAddress + 1)      ); //retrieve lower byte

    return valueFromEEPROM_uint16;
}

/////////////////////////////////////////////////////////////////////////////////////////

void writeToEEPROM_uint16(uint16_t startAddress, uint16_t value)
{
    EEPROM.update( startAddress    , highByte(value) );
    EEPROM.update( startAddress + 1,  lowByte(value) );
}

/////////////////////////////////////////////////////////////////////////////////////////

//copy compile date and time into RAM (stored in array compileDateEEPROM[]) //Example: "Jan 23 2022"
void compileTimestamp_loadFromEEPROM(void)
{
    for (int ii = 0; ii < BYTES_IN_DATE; ii++) { compileDateEEPROM[ii] = EEPROM.read(ii + EEPROM_ADDRESS_COMPILE_DATE); }  
    for (int ii = 0; ii < BYTES_IN_TIME; ii++) { compileTimeEEPROM[ii] = EEPROM.read(ii + EEPROM_ADDRESS_COMPILE_TIME); }  
}

/////////////////////////////////////////////////////////////////////////////////////////

//store compile date and time into EEPROM (i.e. after the firmware is updated)
//Limit calls to this function (EEPROM has limited write lifetime)
void compileTimestamp_writeToEEPROM(void)
{
    for (int ii = 0; ii < BYTES_IN_DATE; ii++) { EEPROM.update( (ii + EEPROM_ADDRESS_COMPILE_DATE), COMPILE_DATE_PROGRAM[ii] ); }
    for (int ii = 0; ii < BYTES_IN_TIME; ii++) { EEPROM.update( (ii + EEPROM_ADDRESS_COMPILE_TIME), COMPILE_TIME_PROGRAM[ii] ); }
}

/////////////////////////////////////////////////////////////////////////////////////////

//compare compile date & time to last values stored in EEPROM
//if timestamps are different, then firmware was just updated
bool wasFirmwareJustUpdated(void)
{
    compileTimestamp_loadFromEEPROM(); //result stored in 'compileDateEEPROM[]' & 'compileTimeEEPROM[]'

    bool areDatesIdentical = true;

    for (uint8_t ii = 0; ii < BYTES_IN_DATE; ii++)
    {
        if (compileDateEEPROM[ii] != COMPILE_DATE_PROGRAM[ii]) { areDatesIdentical = false; }
    }

    for (uint8_t ii = 0; ii < BYTES_IN_TIME; ii++)
    {
        if (compileTimeEEPROM[ii] != COMPILE_TIME_PROGRAM[ii]) { areDatesIdentical = false; }
    }
  
    if (areDatesIdentical == true) { return false; } //firmware NOT updated
    else                           { return  true; } //firmware was updated
}

/////////////////////////////////////////////////////////////////////////////////////////

//returns runtime hours since last firmware update
uint16_t eeprom_hoursSinceLastFirmwareUpdate_get(void)
{
    return readFromEEPROM_uint16(EEPROM_ADDRESS_HOURS_SINCE_UPDATE);
}

/////////////////////////////////////////////////////////////////////////////////////////

//Limit calls to this function (EEPROM has limited write lifetime)
void eeprom_hoursSinceLastFirmwareUpdate_set(uint16_t hourCount)
{
    writeToEEPROM_uint16(EEPROM_ADDRESS_HOURS_SINCE_UPDATE, hourCount);
}

/////////////////////////////////////////////////////////////////////////////////////////

//Takes 4 clock cycles.  EEPROM read limit: infinite
uint8_t eeprom_expirationStatus_get(void)
{ 
    //structured this way to prevent EEPROM read/write failures from disabling LiBCM
    if ((EEPROM.read(EEPROM_ADDRESS_FIRMWARE_STATUS)) == FIRMWARE_EXPIRED) { return FIRMWARE_EXPIRED;   }
    else                                                                   { return FIRMWARE_UNEXPIRED; }          
}

/////////////////////////////////////////////////////////////////////////////////////////

void EEPROM_expirationStatus_set(uint8_t newFirmwareStatus) { EEPROM.update(EEPROM_ADDRESS_FIRMWARE_STATUS, newFirmwareStatus); }

/////////////////////////////////////////////////////////////////////////////////////////

//only call this function once during each keyOff event
uint16_t hoursSincePreviousKeyOff(void)
{
    static uint32_t remainder_ms = 0;
    uint32_t delta_ms = millis() - time_latestKeyOff_ms_get() + remainder_ms;
    uint16_t delta_hours = 0;

    while (delta_ms >= MILLISECONDS_PER_HOUR)
    {
        delta_ms -= MILLISECONDS_PER_HOUR;
        delta_hours++;
    }

    remainder_ms = delta_ms;

    return delta_hours;
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_keyOffCheckForExpiredFirmware(void)
{
    if (wasFirmwareJustUpdated() == true)
    {
        //user just updated the firmware, so...
        eeprom_hoursSinceLastFirmwareUpdate_set(0); //reset hour counter to zero
        compileTimestamp_writeToEEPROM(); //store new compile date in EEPROM (so we can compare again on future keyOFF events)
        EEPROM_expirationStatus_set(FIRMWARE_UNEXPIRED);

        eeprom_applyConfigOverrides(); //push any newly-uncommented config.h values into EEPROM (see config.h for behavior)

        Serial.print(F("\nFirmwareUpdated"));
        gpio_playSound_firmwareUpdated();
    }
    else //user didn't update the firmware
    { 
        uint16_t newUptime_hours = hoursSincePreviousKeyOff() + eeprom_hoursSinceLastFirmwareUpdate_get();
        if (newUptime_hours > REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS) { newUptime_hours = REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS; }
        eeprom_hoursSinceLastFirmwareUpdate_set(newUptime_hours);

        Serial.print(F("\nTotal hours since last firmware update: "));
        if (newUptime_hours >= REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS)
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

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t eeprom_hasLibcmDisabledAssist_get(void)              { return EEPROM.read  (EEPROM_ADDRESS_BATTSCI_ASSIST);                   }
void    eeprom_hasLibcmDisabledAssist_set(uint8_t wasAssistLimited) { EEPROM.update(EEPROM_ADDRESS_BATTSCI_ASSIST, wasAssistLimited); }

uint8_t eeprom_hasLibcmDisabledRegen_get(void)               { return EEPROM.read  (EEPROM_ADDRESS_BATTSCI_REGEN);                    }
void    eeprom_hasLibcmDisabledRegen_set(uint8_t wasRegenLimited)   { EEPROM.update(EEPROM_ADDRESS_BATTSCI_REGEN, wasRegenLimited);   }

uint16_t eeprom_maxCellVoltageDelta_get(void)        { return readFromEEPROM_uint16(EEPROM_ADDRESS_MAX_VCELL_DELTA);                  }
void     eeprom_maxCellVoltageDelta_set(uint16_t newMax)     { writeToEEPROM_uint16(EEPROM_ADDRESS_MAX_VCELL_DELTA, newMax);          }

uint8_t eeprom_delayKeyON_ms_get(void)                       { return EEPROM.read  (EEPROM_ADDRESS_KEYON_DELAY);                      }
void    eeprom_delayKeyON_ms_set(uint8_t delay_ms)                  { EEPROM.update(EEPROM_ADDRESS_KEYON_DELAY, delay_ms);            }

/////////////////////////////////////////////////////////////////////////////////////////

int8_t eeprom_getVspoofOffset_BVO(void) { return EEPROM.read  (EEPROM_ADDRESS_BVO_OFFSET); }
int8_t eeprom_getVspoofOffset_MDV(void) { return EEPROM.read  (EEPROM_ADDRESS_MDV_OFFSET); }
int8_t eeprom_getVspoofOffset_SPF(void) { return EEPROM.read  (EEPROM_ADDRESS_SPF_OFFSET); }

void eeprom_setVspoofOffset_BVO(int8_t newOffset_counts) { EEPROM.update(EEPROM_ADDRESS_BVO_OFFSET, newOffset_counts); }
void eeprom_setVspoofOffset_MDV(int8_t newOffset_counts) { EEPROM.update(EEPROM_ADDRESS_MDV_OFFSET, newOffset_counts); }
void eeprom_setVspoofOffset_SPF(int8_t newOffset_counts) { EEPROM.update(EEPROM_ADDRESS_SPF_OFFSET, newOffset_counts); }

/////////////////////////////////////////////////////////////////////////////////////////

//hardcoded fallback values, used only when EEPROM reads the factory-blank sentinel (i.e. this value has never been set)
//these match the values that used to be config.h's compile-time defaults
const uint8_t  DEFAULT_STACK_SoC_MAX                            = 85;
const uint8_t  DEFAULT_STACK_SoC_MIN                            = 10;
const uint16_t DEFAULT_CELL_VMAX_REGEN                          = 43000;
const uint16_t DEFAULT_CELL_VMIN_ASSIST                         = 31900;
const uint16_t DEFAULT_CELL_VMAX_GRIDCHARGER                    = 39600;
const uint16_t DEFAULT_CELL_VMIN_GRIDCHARGER                    = 30000;
//DEFAULT_CELL_VMIN_KEYOFF omitted -- battery-type dependent, computed via SoC_cellVrest010PercentSoC_get() where used (see eeprom_verifyDataValid())
const uint8_t  DEFAULT_CELL_BALANCE_MIN_SoC                     = 65;
const uint8_t  DEFAULT_CELL_BALANCE_MAX_TEMP_C                  = 40;
const uint8_t  DEFAULT_COOL_TEMP_C_KEYOFF                       = 36;
const uint8_t  DEFAULT_COOL_TEMP_C_GRIDCHARGING                 = 30;
const uint8_t  DEFAULT_COOL_TEMP_C_KEYON                        = 30;
const uint8_t  DEFAULT_HEAT_TEMP_C_KEYON                        = 16;
const uint8_t  DEFAULT_HEAT_TEMP_C_GRIDCHARGING                 = 16;
const uint8_t  DEFAULT_HEAT_TEMP_C_KEYOFF                       = 10;
const uint8_t  DEFAULT_KEYOFF_DISABLE_THERMAL_MGMT_BELOW_SoC    = 50;
const uint8_t  DEFAULT_POWEROFF_DELAY_PACK_EMPTY_MINUTES        = 10;
const uint8_t  DEFAULT_POWEROFF_DELAY_DAYS                      = 5; //0 = feature disabled
const uint8_t  DEFAULT_DISPLAY_POSITIVE_SIGN_DURING_ASSIST      = 1; //1 = true
const uint16_t DEFAULT_DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_ms    = 1000;
const uint8_t  DEFAULT_DISABLE_ASSIST                           = 0; //0 = false
const uint8_t  DEFAULT_DISABLE_REGEN                            = 0; //0 = false
const uint8_t  DEFAULT_IGNORE_CELL_VOLTAGE_MISMATCH             = 0; //0 = false
const uint8_t  DEFAULT_MIN_SPOOFED_VOLTAGE_60S                  = 170;
const uint8_t  DEFAULT_VOLTAGE_SPOOFING_MODE                    = VOLTAGE_SPOOFING_MODE_DISABLE; //closest to OEM behavior

uint8_t eeprom_stackSoCMax_get(void)                { return EEPROM.read(EEPROM_ADDRESS_STACK_SoC_MAX);              }
void    eeprom_stackSoCMax_set(uint8_t newValue)    { EEPROM.update(EEPROM_ADDRESS_STACK_SoC_MAX, newValue);         }

uint8_t eeprom_stackSoCMin_get(void)                { return EEPROM.read(EEPROM_ADDRESS_STACK_SoC_MIN);              }
void    eeprom_stackSoCMin_set(uint8_t newValue)    { EEPROM.update(EEPROM_ADDRESS_STACK_SoC_MIN, newValue);         }

uint16_t eeprom_cellVmaxRegen_get(void)             { return readFromEEPROM_uint16(EEPROM_ADDRESS_CELL_VMAX_REGEN);  }
void     eeprom_cellVmaxRegen_set(uint16_t newValue){ writeToEEPROM_uint16(EEPROM_ADDRESS_CELL_VMAX_REGEN, newValue);}

uint16_t eeprom_cellVminAssist_get(void)              { return readFromEEPROM_uint16(EEPROM_ADDRESS_CELL_VMIN_ASSIST);   }
void     eeprom_cellVminAssist_set(uint16_t newValue) { writeToEEPROM_uint16(EEPROM_ADDRESS_CELL_VMIN_ASSIST, newValue); }

uint16_t eeprom_cellVmaxGridcharger_get(void)              { return readFromEEPROM_uint16(EEPROM_ADDRESS_CELL_VMAX_GRIDCHARGER);   }
void     eeprom_cellVmaxGridcharger_set(uint16_t newValue) { writeToEEPROM_uint16(EEPROM_ADDRESS_CELL_VMAX_GRIDCHARGER, newValue); }

uint16_t eeprom_cellVminGridcharger_get(void)              { return readFromEEPROM_uint16(EEPROM_ADDRESS_CELL_VMIN_GRIDCHARGER);   }
void     eeprom_cellVminGridcharger_set(uint16_t newValue) { writeToEEPROM_uint16(EEPROM_ADDRESS_CELL_VMIN_GRIDCHARGER, newValue); }

uint16_t eeprom_cellVminKeyoff_get(void)              { return readFromEEPROM_uint16(EEPROM_ADDRESS_CELL_VMIN_KEYOFF);   }
void     eeprom_cellVminKeyoff_set(uint16_t newValue) { writeToEEPROM_uint16(EEPROM_ADDRESS_CELL_VMIN_KEYOFF, newValue); }

uint8_t eeprom_cellBalanceMinSoC_get(void)             { return EEPROM.read(EEPROM_ADDRESS_CELL_BALANCE_MIN_SoC);      }
void    eeprom_cellBalanceMinSoC_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_CELL_BALANCE_MIN_SoC, newValue); }

uint8_t eeprom_cellBalanceMaxTemp_C_get(void)             { return EEPROM.read(EEPROM_ADDRESS_CELL_BALANCE_MAX_TEMP_C);      }
void    eeprom_cellBalanceMaxTemp_C_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_CELL_BALANCE_MAX_TEMP_C, newValue); }

uint8_t eeprom_coolBatteryAboveTempC_Keyoff_get(void)             { return EEPROM.read(EEPROM_ADDRESS_COOL_TEMP_C_KEYOFF);      }
void    eeprom_coolBatteryAboveTempC_Keyoff_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_COOL_TEMP_C_KEYOFF, newValue); }

uint8_t eeprom_coolBatteryAboveTempC_Gridcharging_get(void)             { return EEPROM.read(EEPROM_ADDRESS_COOL_TEMP_C_GRIDCHARGING);      }
void    eeprom_coolBatteryAboveTempC_Gridcharging_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_COOL_TEMP_C_GRIDCHARGING, newValue); }

uint8_t eeprom_coolBatteryAboveTempC_Keyon_get(void)             { return EEPROM.read(EEPROM_ADDRESS_COOL_TEMP_C_KEYON);      }
void    eeprom_coolBatteryAboveTempC_Keyon_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_COOL_TEMP_C_KEYON, newValue); }

uint8_t eeprom_heatBatteryBelowTempC_Keyon_get(void)             { return EEPROM.read(EEPROM_ADDRESS_HEAT_TEMP_C_KEYON);      }
void    eeprom_heatBatteryBelowTempC_Keyon_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_HEAT_TEMP_C_KEYON, newValue); }

uint8_t eeprom_heatBatteryBelowTempC_Gridcharging_get(void)             { return EEPROM.read(EEPROM_ADDRESS_HEAT_TEMP_C_GRIDCHARGING);      }
void    eeprom_heatBatteryBelowTempC_Gridcharging_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_HEAT_TEMP_C_GRIDCHARGING, newValue); }

uint8_t eeprom_heatBatteryBelowTempC_Keyoff_get(void)             { return EEPROM.read(EEPROM_ADDRESS_HEAT_TEMP_C_KEYOFF);      }
void    eeprom_heatBatteryBelowTempC_Keyoff_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_HEAT_TEMP_C_KEYOFF, newValue); }

uint8_t eeprom_keyoffDisableThermalManagementBelowSoCPercent_get(void)             { return EEPROM.read(EEPROM_ADDRESS_KEYOFF_DISABLE_THERMAL_MGMT_BELOW_SoC);      }
void    eeprom_keyoffDisableThermalManagementBelowSoCPercent_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_KEYOFF_DISABLE_THERMAL_MGMT_BELOW_SoC, newValue); }

uint8_t eeprom_poweroffDelayAfterKeyoffPackEmptyMinutes_get(void)             { return EEPROM.read(EEPROM_ADDRESS_POWEROFF_DELAY_PACK_EMPTY_MINUTES);      }
void    eeprom_poweroffDelayAfterKeyoffPackEmptyMinutes_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_POWEROFF_DELAY_PACK_EMPTY_MINUTES, newValue); }

uint8_t eeprom_poweroffDelayAfterKeyoffDays_get(void)             { return EEPROM.read(EEPROM_ADDRESS_POWEROFF_DELAY_DAYS);      }
void    eeprom_poweroffDelayAfterKeyoffDays_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_POWEROFF_DELAY_DAYS, newValue); }

bool eeprom_isPositiveSignDuringAssist_get(void)          { return EEPROM.read(EEPROM_ADDRESS_DISPLAY_POSITIVE_SIGN_DURING_ASSIST) != 0; }
void eeprom_isPositiveSignDuringAssist_set(bool newValue) { EEPROM.update(EEPROM_ADDRESS_DISPLAY_POSITIVE_SIGN_DURING_ASSIST, newValue ? 1 : 0); }

uint16_t eeprom_debugUsbUpdatePeriodGridcharge_ms_get(void)              { return readFromEEPROM_uint16(EEPROM_ADDRESS_DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE);   }
void     eeprom_debugUsbUpdatePeriodGridcharge_ms_set(uint16_t newValue) { writeToEEPROM_uint16(EEPROM_ADDRESS_DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE, newValue); }

bool eeprom_isAssistDisabled_get(void)          { return EEPROM.read(EEPROM_ADDRESS_DISABLE_ASSIST) != 0; }
void eeprom_isAssistDisabled_set(bool newValue) { EEPROM.update(EEPROM_ADDRESS_DISABLE_ASSIST, newValue ? 1 : 0); }

bool eeprom_isRegenDisabled_get(void)          { return EEPROM.read(EEPROM_ADDRESS_DISABLE_REGEN) != 0; }
void eeprom_isRegenDisabled_set(bool newValue) { EEPROM.update(EEPROM_ADDRESS_DISABLE_REGEN, newValue ? 1 : 0); }

bool eeprom_isCellVoltageMismatchIgnored_get(void)          { return EEPROM.read(EEPROM_ADDRESS_IGNORE_CELL_VOLTAGE_MISMATCH) != 0; }
void eeprom_isCellVoltageMismatchIgnored_set(bool newValue) { EEPROM.update(EEPROM_ADDRESS_IGNORE_CELL_VOLTAGE_MISMATCH, newValue ? 1 : 0); }

uint8_t eeprom_minSpoofedVoltage60S_get(void)             { return EEPROM.read(EEPROM_ADDRESS_MIN_SPOOFED_VOLTAGE_60S);      }
void    eeprom_minSpoofedVoltage60S_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_MIN_SPOOFED_VOLTAGE_60S, newValue); }

//hardware-identity fields -- no eeprom_verifyDataValid() default-restore for these (see eeprom_validateHardwareConfig())
uint8_t eeprom_currentHackMode_get(void)             { return EEPROM.read(EEPROM_ADDRESS_CURRENT_HACK);      }
void    eeprom_currentHackMode_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_CURRENT_HACK, newValue); }

uint8_t eeprom_gridChargerType_get(void)             { return EEPROM.read(EEPROM_ADDRESS_GRIDCHARGER_TYPE);      }
void    eeprom_gridChargerType_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_GRIDCHARGER_TYPE, newValue); }

uint8_t eeprom_batteryType_get(void)             { return EEPROM.read(EEPROM_ADDRESS_BATTERY_TYPE);      }
void    eeprom_batteryType_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_BATTERY_TYPE, newValue); }

//has a safe default (see DEFAULT_VOLTAGE_SPOOFING_MODE) -- restored by eeprom_verifyDataValid() like the other tunables
uint8_t eeprom_voltageSpoofingMode_get(void)             { return EEPROM.read(EEPROM_ADDRESS_VOLTAGE_SPOOFING_MODE);      }
void    eeprom_voltageSpoofingMode_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_VOLTAGE_SPOOFING_MODE, newValue); }

uint8_t eeprom_stackSize_get(void)             { return EEPROM.read(EEPROM_ADDRESS_STACK_SIZE);      }
void    eeprom_stackSize_set(uint8_t newValue) { EEPROM.update(EEPROM_ADDRESS_STACK_SIZE, newValue); }

/////////////////////////////////////////////////////////////////////////////////////////

void printMessage_RestoringEEPROM(void) { Serial.print(F("\nRestoring EEPROM value: ")); }

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_verifyDataValid(void)
{
    //verify all runtime-configurable data stored in EEPROM is valid.  If not, load default value(s)
    if ( !((eeprom_hasLibcmDisabledRegen_get() == EEPROM_LIBCM_DISABLED_REGEN) ||
           (eeprom_hasLibcmDisabledRegen_get() == EEPROM_REGEN_NEVER_LIMITED )  ) )
    {
        printMessage_RestoringEEPROM(); Serial.print(F("LIBCM_DISABLED_REGEN"));
        eeprom_hasLibcmDisabledRegen_set(EEPROM_REGEN_NEVER_LIMITED);
    }

    if ( !((eeprom_hasLibcmDisabledAssist_get() == EEPROM_LIBCM_DISABLED_ASSIST) ||
           (eeprom_hasLibcmDisabledAssist_get() == EEPROM_ASSIST_NEVER_LIMITED )  ) )
    {
        printMessage_RestoringEEPROM(); Serial.print(F("LIBCM_DISABLED_ASSIST"));
        eeprom_hasLibcmDisabledAssist_set(EEPROM_ASSIST_NEVER_LIMITED);
    }

    if (eeprom_delayKeyON_ms_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    {
        printMessage_RestoringEEPROM(); Serial.print(F("KEYON_DELAY"));
        eeprom_delayKeyON_ms_set(0);
    }

    if (eeprom_maxCellVoltageDelta_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_16b)
    {
        printMessage_RestoringEEPROM(); Serial.print(F("IMBALANCE_DELTA"));
        eeprom_maxCellVoltageDelta_set(CELL_MAJOR_IMBALANCE_DELTA);
    }
    
    if (EEPROM.read(EEPROM_ADDRESS_BATT_HISTORY_UNINIT) == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    {
        eeprom_batteryHistory_reset();
        EEPROM.update(EEPROM_ADDRESS_BATT_HISTORY_UNINIT, EEPROM_ADDRESS_FORMATTED_VALUE);
    }

    if (EEPROM.read(EEPROM_ADDRESS_Wh_RECORDS_UNINIT)   == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    {
        eeprom_wattHourHistory_reset();
        EEPROM.update(EEPROM_ADDRESS_Wh_RECORDS_UNINIT, EEPROM_ADDRESS_FORMATTED_VALUE);
        EEPROM.update(EEPROM_ADDRESS_NEXT_Wh_RECORD,    EEPROM_ADDRESS_FORMATTED_VALUE);
    }

    if ((EEPROM.read(EEPROM_ADDRESS_BVO_OFFSET) == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b) &&
        (EEPROM.read(EEPROM_ADDRESS_MDV_OFFSET) == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b) &&
        (EEPROM.read(EEPROM_ADDRESS_SPF_OFFSET) == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)  )
    {
        //JTS2doLater: fix int8_t issue where EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b = 0xFF = -1
        printMessage_RestoringEEPROM(); Serial.print(F("VPACKSPOOF_OFFSETS"));
        EEPROM.update(EEPROM_ADDRESS_BVO_OFFSET, EEPROM_ADDRESS_FORMATTED_VALUE);
        EEPROM.update(EEPROM_ADDRESS_MDV_OFFSET, EEPROM_ADDRESS_FORMATTED_VALUE);
        EEPROM.update(EEPROM_ADDRESS_SPF_OFFSET, EEPROM_ADDRESS_FORMATTED_VALUE);
    }

    //config.h "sticky override" tunables //restore hardcoded default the first time each value is ever read (factory-blank EEPROM)
    if (eeprom_stackSoCMax_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("STACK_SoC_MAX"));       eeprom_stackSoCMax_set(DEFAULT_STACK_SoC_MAX); }

    if (eeprom_stackSoCMin_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("STACK_SoC_MIN"));       eeprom_stackSoCMin_set(DEFAULT_STACK_SoC_MIN); }

    if (eeprom_cellVmaxRegen_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_16b)
    { printMessage_RestoringEEPROM(); Serial.print(F("CELL_VMAX_REGEN"));     eeprom_cellVmaxRegen_set(DEFAULT_CELL_VMAX_REGEN); }

    if (eeprom_cellVminAssist_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_16b)
    { printMessage_RestoringEEPROM(); Serial.print(F("CELL_VMIN_ASSIST"));    eeprom_cellVminAssist_set(DEFAULT_CELL_VMIN_ASSIST); }

    if (eeprom_cellVmaxGridcharger_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_16b)
    { printMessage_RestoringEEPROM(); Serial.print(F("CELL_VMAX_GRIDCHARGER")); eeprom_cellVmaxGridcharger_set(DEFAULT_CELL_VMAX_GRIDCHARGER); }

    if (eeprom_cellVminGridcharger_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_16b)
    { printMessage_RestoringEEPROM(); Serial.print(F("CELL_VMIN_GRIDCHARGER")); eeprom_cellVminGridcharger_set(DEFAULT_CELL_VMIN_GRIDCHARGER); }

    if (eeprom_cellVminKeyoff_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_16b)
    { printMessage_RestoringEEPROM(); Serial.print(F("CELL_VMIN_KEYOFF"));    eeprom_cellVminKeyoff_set(SoC_cellVrest010PercentSoC_get()); }

    if (eeprom_cellBalanceMinSoC_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("CELL_BALANCE_MIN_SoC")); eeprom_cellBalanceMinSoC_set(DEFAULT_CELL_BALANCE_MIN_SoC); }

    if (eeprom_cellBalanceMaxTemp_C_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("CELL_BALANCE_MAX_TEMP_C")); eeprom_cellBalanceMaxTemp_C_set(DEFAULT_CELL_BALANCE_MAX_TEMP_C); }

    if (eeprom_coolBatteryAboveTempC_Keyoff_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("COOL_TEMP_C_KEYOFF")); eeprom_coolBatteryAboveTempC_Keyoff_set(DEFAULT_COOL_TEMP_C_KEYOFF); }

    if (eeprom_coolBatteryAboveTempC_Gridcharging_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("COOL_TEMP_C_GRIDCHARGING")); eeprom_coolBatteryAboveTempC_Gridcharging_set(DEFAULT_COOL_TEMP_C_GRIDCHARGING); }

    if (eeprom_coolBatteryAboveTempC_Keyon_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("COOL_TEMP_C_KEYON")); eeprom_coolBatteryAboveTempC_Keyon_set(DEFAULT_COOL_TEMP_C_KEYON); }

    if (eeprom_heatBatteryBelowTempC_Keyon_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("HEAT_TEMP_C_KEYON")); eeprom_heatBatteryBelowTempC_Keyon_set(DEFAULT_HEAT_TEMP_C_KEYON); }

    if (eeprom_heatBatteryBelowTempC_Gridcharging_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("HEAT_TEMP_C_GRIDCHARGING")); eeprom_heatBatteryBelowTempC_Gridcharging_set(DEFAULT_HEAT_TEMP_C_GRIDCHARGING); }

    if (eeprom_heatBatteryBelowTempC_Keyoff_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("HEAT_TEMP_C_KEYOFF")); eeprom_heatBatteryBelowTempC_Keyoff_set(DEFAULT_HEAT_TEMP_C_KEYOFF); }

    if (eeprom_keyoffDisableThermalManagementBelowSoCPercent_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("KEYOFF_DISABLE_THERMAL_MGMT")); eeprom_keyoffDisableThermalManagementBelowSoCPercent_set(DEFAULT_KEYOFF_DISABLE_THERMAL_MGMT_BELOW_SoC); }

    if (eeprom_poweroffDelayAfterKeyoffPackEmptyMinutes_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("POWEROFF_DELAY_PACK_EMPTY_MINUTES")); eeprom_poweroffDelayAfterKeyoffPackEmptyMinutes_set(DEFAULT_POWEROFF_DELAY_PACK_EMPTY_MINUTES); }

    if (eeprom_poweroffDelayAfterKeyoffDays_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("POWEROFF_DELAY_DAYS")); eeprom_poweroffDelayAfterKeyoffDays_set(DEFAULT_POWEROFF_DELAY_DAYS); }

    if (EEPROM.read(EEPROM_ADDRESS_DISPLAY_POSITIVE_SIGN_DURING_ASSIST) == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("DISPLAY_POSITIVE_SIGN_DURING_ASSIST")); eeprom_isPositiveSignDuringAssist_set(DEFAULT_DISPLAY_POSITIVE_SIGN_DURING_ASSIST); }

    if (eeprom_debugUsbUpdatePeriodGridcharge_ms_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_16b)
    { printMessage_RestoringEEPROM(); Serial.print(F("DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE")); eeprom_debugUsbUpdatePeriodGridcharge_ms_set(DEFAULT_DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_ms); }

    if (EEPROM.read(EEPROM_ADDRESS_DISABLE_ASSIST) == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("DISABLE_ASSIST")); eeprom_isAssistDisabled_set(DEFAULT_DISABLE_ASSIST); }

    if (EEPROM.read(EEPROM_ADDRESS_DISABLE_REGEN) == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("DISABLE_REGEN")); eeprom_isRegenDisabled_set(DEFAULT_DISABLE_REGEN); }

    if (EEPROM.read(EEPROM_ADDRESS_IGNORE_CELL_VOLTAGE_MISMATCH) == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("IGNORE_CELL_VOLTAGE_MISMATCH")); eeprom_isCellVoltageMismatchIgnored_set(DEFAULT_IGNORE_CELL_VOLTAGE_MISMATCH); }

    if (eeprom_minSpoofedVoltage60S_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("MIN_SPOOFED_VOLTAGE_60S")); eeprom_minSpoofedVoltage60S_set(DEFAULT_MIN_SPOOFED_VOLTAGE_60S); }

    if (eeprom_voltageSpoofingMode_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { printMessage_RestoringEEPROM(); Serial.print(F("VOLTAGE_SPOOFING_MODE")); eeprom_voltageSpoofingMode_set(DEFAULT_VOLTAGE_SPOOFING_MODE); }

    //current-hack, grid charger type, battery type, and stack size have no default -- see eeprom_validateHardwareConfig()
}

/////////////////////////////////////////////////////////////////////////////////////////

//Applies any config.h "CONFIG_*" overrides the user uncommented, writing them into EEPROM once at boot.
//Lines left commented out in config.h leave the existing EEPROM value (set here on a previous boot, or the hardcoded
//default restored by eeprom_verifyDataValid() on a factory-blank chip) completely untouched.
void eeprom_applyConfigOverrides(void)
{
    #ifdef CONFIG_STACK_SoC_MAX
        eeprom_stackSoCMax_set(CONFIG_STACK_SoC_MAX);
    #endif

    #ifdef CONFIG_STACK_SoC_MIN
        eeprom_stackSoCMin_set(CONFIG_STACK_SoC_MIN);
    #endif

    #ifdef CONFIG_CELL_VMAX_REGEN
        eeprom_cellVmaxRegen_set(CONFIG_CELL_VMAX_REGEN);
    #endif

    #ifdef CONFIG_CELL_VMIN_ASSIST
        eeprom_cellVminAssist_set(CONFIG_CELL_VMIN_ASSIST);
    #endif

    #ifdef CONFIG_CELL_VMAX_GRIDCHARGER
        eeprom_cellVmaxGridcharger_set(CONFIG_CELL_VMAX_GRIDCHARGER);
    #endif

    #ifdef CONFIG_CELL_VMIN_GRIDCHARGER
        eeprom_cellVminGridcharger_set(CONFIG_CELL_VMIN_GRIDCHARGER);
    #endif

    #ifdef CONFIG_CELL_VMIN_KEYOFF
        eeprom_cellVminKeyoff_set(CONFIG_CELL_VMIN_KEYOFF);
    #endif

    #ifdef CONFIG_CELL_BALANCE_MIN_SoC
        eeprom_cellBalanceMinSoC_set(CONFIG_CELL_BALANCE_MIN_SoC);
    #endif

    #ifdef CONFIG_CELL_BALANCE_MAX_TEMP_C
        eeprom_cellBalanceMaxTemp_C_set(CONFIG_CELL_BALANCE_MAX_TEMP_C);
    #endif

    #ifdef CONFIG_COOL_BATTERY_ABOVE_TEMP_C_KEYOFF
        eeprom_coolBatteryAboveTempC_Keyoff_set(CONFIG_COOL_BATTERY_ABOVE_TEMP_C_KEYOFF);
    #endif

    #ifdef CONFIG_COOL_BATTERY_ABOVE_TEMP_C_GRIDCHARGING
        eeprom_coolBatteryAboveTempC_Gridcharging_set(CONFIG_COOL_BATTERY_ABOVE_TEMP_C_GRIDCHARGING);
    #endif

    #ifdef CONFIG_COOL_BATTERY_ABOVE_TEMP_C_KEYON
        eeprom_coolBatteryAboveTempC_Keyon_set(CONFIG_COOL_BATTERY_ABOVE_TEMP_C_KEYON);
    #endif

    #ifdef CONFIG_HEAT_BATTERY_BELOW_TEMP_C_KEYON
        eeprom_heatBatteryBelowTempC_Keyon_set(CONFIG_HEAT_BATTERY_BELOW_TEMP_C_KEYON);
    #endif

    #ifdef CONFIG_HEAT_BATTERY_BELOW_TEMP_C_GRIDCHARGING
        eeprom_heatBatteryBelowTempC_Gridcharging_set(CONFIG_HEAT_BATTERY_BELOW_TEMP_C_GRIDCHARGING);
    #endif

    #ifdef CONFIG_HEAT_BATTERY_BELOW_TEMP_C_KEYOFF
        eeprom_heatBatteryBelowTempC_Keyoff_set(CONFIG_HEAT_BATTERY_BELOW_TEMP_C_KEYOFF);
    #endif

    #ifdef CONFIG_KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC_PERCENT
        eeprom_keyoffDisableThermalManagementBelowSoCPercent_set(CONFIG_KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC_PERCENT);
    #endif

    #ifdef CONFIG_POWEROFF_DELAY_AFTER_KEYOFF_PACK_EMPTY_MINUTES
        eeprom_poweroffDelayAfterKeyoffPackEmptyMinutes_set(CONFIG_POWEROFF_DELAY_AFTER_KEYOFF_PACK_EMPTY_MINUTES);
    #endif

    #ifdef CONFIG_POWEROFF_DELAY_AFTER_KEYOFF_DAYS
        eeprom_poweroffDelayAfterKeyoffDays_set(CONFIG_POWEROFF_DELAY_AFTER_KEYOFF_DAYS);
    #endif

    #if defined CONFIG_DISPLAY_POSITIVE_SIGN_DURING_ASSIST
        eeprom_isPositiveSignDuringAssist_set(true);
    #elif defined CONFIG_DISPLAY_NEGATIVE_SIGN_DURING_ASSIST
        eeprom_isPositiveSignDuringAssist_set(false);
    #endif

    #ifdef CONFIG_DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS
        eeprom_debugUsbUpdatePeriodGridcharge_ms_set(CONFIG_DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS);
    #endif

    #ifdef CONFIG_DISABLE_ASSIST
        eeprom_isAssistDisabled_set(true);
    #endif

    #ifdef CONFIG_DISABLE_REGEN
        eeprom_isRegenDisabled_set(true);
    #endif

    #ifdef CONFIG_IGNORE_CELL_VOLTAGE_MISMATCH
        eeprom_isCellVoltageMismatchIgnored_set(true);
    #endif

    #ifdef CONFIG_MIN_SPOOFED_VOLTAGE_60S
        eeprom_minSpoofedVoltage60S_set(CONFIG_MIN_SPOOFED_VOLTAGE_60S);
    #endif

    #if   defined CONFIG_VOLTAGE_SPOOFING_DISABLE
        eeprom_voltageSpoofingMode_set(VOLTAGE_SPOOFING_MODE_DISABLE);
    #elif defined CONFIG_VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE
        eeprom_voltageSpoofingMode_set(VOLTAGE_SPOOFING_MODE_ASSIST_ONLY_VARIABLE);
    #elif defined CONFIG_VOLTAGE_SPOOFING_ASSIST_ONLY_BINARY
        eeprom_voltageSpoofingMode_set(VOLTAGE_SPOOFING_MODE_ASSIST_ONLY_BINARY);
    #elif defined CONFIG_VOLTAGE_SPOOFING_ASSIST_AND_REGEN
        eeprom_voltageSpoofingMode_set(VOLTAGE_SPOOFING_MODE_ASSIST_AND_REGEN);
    #elif defined CONFIG_VOLTAGE_SPOOFING_LINEAR
        eeprom_voltageSpoofingMode_set(VOLTAGE_SPOOFING_MODE_LINEAR);
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

//Hardware-identity fields have no safe default, so (unlike eeprom_applyConfigOverrides()) this runs unconditionally
//at every boot -- not just once per detected firmware update -- so a fresh/factory-reset board can escape the
//"unconfigured" state before eeprom_validateHardwareConfig() would otherwise fatally halt it.
void eeprom_applyBootCriticalConfigOverrides(void)
{
    #if   defined CONFIG_SET_CURRENT_HACK_00
        eeprom_currentHackMode_set(CURRENT_HACK_00);
    #elif defined CONFIG_SET_CURRENT_HACK_20
        eeprom_currentHackMode_set(CURRENT_HACK_20);
    #elif defined CONFIG_SET_CURRENT_HACK_40
        eeprom_currentHackMode_set(CURRENT_HACK_40);
    #elif defined CONFIG_SET_CURRENT_HACK_60
        eeprom_currentHackMode_set(CURRENT_HACK_60);
    #endif

    #if   defined CONFIG_GRIDCHARGER_IS_NOT_1500W
        eeprom_gridChargerType_set(GRIDCHARGER_TYPE_NOT_1500W);
    #elif defined CONFIG_GRIDCHARGER_IS_1500W
        eeprom_gridChargerType_set(GRIDCHARGER_TYPE_1500W);
    #endif

    #if   defined CONFIG_BATTERY_TYPE_5AhG3
        eeprom_batteryType_set(BATTERY_TYPE_VALUE_5AhG3);
    #elif defined CONFIG_BATTERY_TYPE_47Ah
        eeprom_batteryType_set(BATTERY_TYPE_VALUE_47Ah);
    #endif

    #if   defined CONFIG_STACK_IS_48S
        eeprom_stackSize_set(STACK_SIZE_VALUE_48S);
    #elif defined CONFIG_STACK_IS_60S
        eeprom_stackSize_set(STACK_SIZE_VALUE_60S);
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

//mirrors the fatal-error UX in LTC68042configure_doesActualPackSizeMatchUserConfig()
void eeprom_hardwareConfig_fatalError(const __FlashStringHelper* reason)
{
    Serial.print(F("\nError: hardware configuration is unconfigured or invalid. Debug: "));
    Serial.print(reason);
    Serial.print(F("\nLiBCM is disabled until config.h is corrected."));

    lcdTransmit_begin();
    delay(50); //delay doesn't matter because this is a fatal error
    lcdTransmit_displayOn();
    delay(50); //delay doesn't matter because this is a fatal error
    lcdTransmit_Warning(LCD_WARN_HW_CONFIG);

    gpio_turnBuzzer_on_highFreq(); //call GPIO directly

    wdt_disable(); //turn off watchdog to prevent reset
    wdt_enable(WDTO_8S);

    delay(7000); //give the user enough time to read error message

    gpio_turnLiBCM_off(); //game over... thanks for playing
}

void eeprom_validateHardwareConfig(void)
{
    if (eeprom_currentHackMode_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { eeprom_hardwareConfig_fatalError(F("current-hack not selected")); }

    if (eeprom_currentHackMode_get() > CURRENT_HACK_60)
    { eeprom_hardwareConfig_fatalError(F("current-hack value is invalid")); }

    if (eeprom_gridChargerType_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { eeprom_hardwareConfig_fatalError(F("grid charger type not selected")); }

    if (eeprom_batteryType_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { eeprom_hardwareConfig_fatalError(F("battery type not selected")); }

    if (eeprom_stackSize_get() == EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b)
    { eeprom_hardwareConfig_fatalError(F("pack size (48S/60S) not selected")); }

    if ((eeprom_stackSize_get() == STACK_SIZE_VALUE_60S) && (eeprom_batteryType_get() == BATTERY_TYPE_VALUE_5AhG3))
    { eeprom_hardwareConfig_fatalError(F("60S not supported with 5AhG3 cells")); }

    if ((eeprom_batteryType_get() == BATTERY_TYPE_VALUE_5AhG3) && (eeprom_gridChargerType_get() == GRIDCHARGER_TYPE_1500W))
    { eeprom_hardwareConfig_fatalError(F("5AhG3 kits dont support 1500 watt charging")); }

    if ((eeprom_stackSize_get() == STACK_SIZE_VALUE_60S) && (eeprom_voltageSpoofingMode_get() != VOLTAGE_SPOOFING_MODE_DISABLE))
    { eeprom_hardwareConfig_fatalError(F("60S only works with voltage-spoofing mode DISABLE")); }

    if (eeprom_voltageSpoofingMode_get() > VOLTAGE_SPOOFING_MODE_LINEAR)
    { eeprom_hardwareConfig_fatalError(F("voltage-spoofing mode value is invalid")); }
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_resetDebugValues(void)
{
    eeprom_hasLibcmDisabledRegen_set(EEPROM_REGEN_NEVER_LIMITED);
    eeprom_hasLibcmDisabledAssist_set(EEPROM_ASSIST_NEVER_LIMITED);
    eeprom_maxCellVoltageDelta_set(CELL_MAJOR_IMBALANCE_DELTA);
}

/////////////////////////////////////////////////////////////////////////////////////////

uint16_t convert_temperatureAndSoC_arrayIndexToEepromAddress(uint8_t indexTemperature, uint8_t indexSoC)
{
    uint16_t address = ((indexTemperature * TOTAL_TEMP_BINS + indexSoC) * NUM_BYTES_PER_BIN) + EEPROM_ADDRESS_BATT_HISTORY;

    return address;
}

/////////////////////////////////////////////////////////////////////////////////////////

uint16_t eeprom_batteryHistory_getValue(uint8_t indexTemperature, uint8_t indexSoC)
{
    uint16_t address = convert_temperatureAndSoC_arrayIndexToEepromAddress(indexTemperature, indexSoC);

    return readFromEEPROM_uint16(address);
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_batteryHistory_incrementValue(uint8_t indexTemperature, uint8_t indexSoC)
{
    uint16_t address = convert_temperatureAndSoC_arrayIndexToEepromAddress(indexTemperature, indexSoC);

    uint16_t existingValue = readFromEEPROM_uint16(address);

    if (existingValue != 0xFFFF) { writeToEEPROM_uint16(address, existingValue + 1); }
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_eraseRange(uint16_t minAddress, uint16_t maxAddress, uint8_t valueToWrite)
{    
    for (uint16_t address = minAddress; address <= maxAddress; address++)
    {
        if ((address & 0b01111111) == 0) //if divisible by 128
        {
            wdt_reset();
            Serial.print('\n');
        }

        EEPROM.update(address, valueToWrite);
    
        Serial.print('.');
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_batteryHistory_reset(void)
{
    uint16_t minAddress = EEPROM_ADDRESS_BATT_HISTORY;
    uint16_t maxAddress = EEPROM_ADDRESS_BATT_HISTORY + NUM_BYTES_BATTERY_HISTORY;
  
    Serial.print(F("\nInitializing battery history"));
    eeprom_eraseRange(minAddress, maxAddress, EEPROM_ADDRESS_FORMATTED_VALUE);
    Serial.print(F("\nDone"));
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_wattHourHistory_storeSession( uint16_t time_s,
                                          uint16_t distance_TBD,
                                          uint16_t assist_Wh,
                                          uint16_t regen_Wh    )
{
    uint8_t recordNumber = EEPROM.read(EEPROM_ADDRESS_NEXT_Wh_RECORD);

    if (recordNumber >= NUM_Wh_RECORDS) { recordNumber = 0; }

    uint16_t baseAddress = EEPROM_ADDRESS_Wh_RECORDS + recordNumber * NUM_BYTES_PER_Wh_RECORD;

    writeToEEPROM_uint16(baseAddress + EEPROM_OFFSET_TIME, time_s      );
    writeToEEPROM_uint16(baseAddress + EEPROM_OFFSET_DIST, distance_TBD); //JTS2doLater: implement, requires LiControl+SPI
    writeToEEPROM_uint16(baseAddress + EEPROM_OFFSET_ASST, assist_Wh   );
    writeToEEPROM_uint16(baseAddress + EEPROM_OFFSET_RGEN, regen_Wh    );

    EEPROM.update(EEPROM_ADDRESS_NEXT_Wh_RECORD, ++recordNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_wattHourHistory_printTripHistory(void)
{
    uint8_t oldestRecordAddress = EEPROM.read(EEPROM_ADDRESS_NEXT_Wh_RECORD);

    Serial.print(F("\n\nTrip History (oldest trip first):"
                   "\ntime(s), distance(TBD), assist(Wh), regen(Wh)\n"));
    
    uint32_t totalAssist_Wh = 0;
    uint32_t totalregen_Wh  = 0;
    uint32_t totalSeconds   = 0;
    uint16_t totalDistance  = 0;

    for (uint8_t ii = 0; ii < NUM_Wh_RECORDS; ii++)
    {
        uint16_t recordToPrint = oldestRecordAddress + ii;
        if (recordToPrint >= NUM_Wh_RECORDS) { recordToPrint -= NUM_Wh_RECORDS; } //print ring buffer starting from oldest

        uint16_t baseAddress = EEPROM_ADDRESS_Wh_RECORDS + recordToPrint * NUM_BYTES_PER_Wh_RECORD;

        Serial.print('\n');
        
        //print time
        uint16_t time_s = readFromEEPROM_uint16(baseAddress + EEPROM_OFFSET_TIME);
        Serial.print(time_s);
        Serial.print(',');
        totalSeconds += time_s;

        //print distance
        uint16_t distance_TBD = readFromEEPROM_uint16(baseAddress + EEPROM_OFFSET_DIST);
        Serial.print(distance_TBD);
        Serial.print(',');
        totalDistance += distance_TBD;

        //print assist
        uint16_t assist_Wh = readFromEEPROM_uint16(baseAddress + EEPROM_OFFSET_ASST);
        Serial.print(assist_Wh);
        Serial.print(',');
        totalAssist_Wh += assist_Wh;

        //print regen
        uint16_t regen_Wh = readFromEEPROM_uint16(baseAddress + EEPROM_OFFSET_RGEN);
        Serial.print(regen_Wh);
        totalregen_Wh += regen_Wh;
    }

    Serial.print(F("\n\nTotals:\ntime (s): "));  Serial.print(totalSeconds,   DEC);
    Serial.print(F(           "\ndistance: "));  Serial.print(totalDistance,  DEC);
    Serial.print(F(           "\nassist Wh: ")); Serial.print(totalAssist_Wh, DEC);
    Serial.print(F(           "\nregen  Wh: ")); Serial.print(totalregen_Wh,  DEC);

    Serial.print('\n');
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_wattHourHistory_reset(void)
{
    uint16_t minAddress = EEPROM_ADDRESS_Wh_RECORDS;
    uint16_t maxAddress = EEPROM_ADDRESS_Wh_RECORDS + NUM_BYTES_Wh_HISTORY;
  
    Serial.print(F("\nInitializing energy history"));
    
    eeprom_eraseRange(minAddress, maxAddress, EEPROM_ADDRESS_FORMATTED_VALUE);
    EEPROM.update(EEPROM_ADDRESS_NEXT_Wh_RECORD, 0);
    
    Serial.print(F("\nDone"));
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_resetAll(void)
{
    uint16_t minAddress = 0;
    uint16_t maxAddress = EEPROM_LAST_USABLE_ADDRESS;
  
    Serial.print(F("\nEEPROM factory reset"));

    eeprom_eraseRange(minAddress, maxAddress, EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b);

    Serial.print(F("\nDone. Rebooting."));

    while (1) { ; } //wait for watchdog reboot
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_resetAll_userConfirm(void)
{
    static uint32_t lastTimeThisFunctionCalled = 0;

    if      (key_getSampledState() == KEYSTATE_ON) { Serial.print(F("\nKey must be off")); }
    else if (millis() - lastTimeThisFunctionCalled > 10000)
    {
        Serial.print(F("\nRepeat command to erase all EEPROM data"));
        lastTimeThisFunctionCalled = millis();
    }
    else { eeprom_resetAll(); }
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_printAll(void)
{
    Serial.print(F("\n\nDumping EEPROM"));

    for(uint16_t address = 0; address <= EEPROM_LAST_USABLE_ADDRESS; address++)
    {
        if ((address & 0b00001111) == 0) //if divisible by 16
        {
            wdt_reset();
            Serial.print('\n');
            Serial.print(F("0x"));
            if (address <= 0xF ) { Serial.print('0'); }
            if (address <= 0xFF) { Serial.print('0'); }
            Serial.print(address, HEX);
            Serial.print(F(": "));
        }

        uint8_t data = EEPROM.read(address);

        if (data <= 0xF) { Serial.print('0'); }
        Serial.print(data, HEX);
        Serial.print(',');
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void eeprom_begin(void)
{
    eeprom_verifyDataValid();
}

/////////////////////////////////////////////////////////////////////////////////////////
