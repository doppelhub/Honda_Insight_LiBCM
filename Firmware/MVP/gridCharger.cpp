//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
#include "libcm.h"

//JTS2doLater: Feature: If SoC greater than 70% when grid charger first plugged in, then charge to 85% SoC.

uint32_t latestPlugin_ms = 0;
uint32_t latestChargerDisable_ms = 0;
uint32_t minGridOffPeriod_ms = GRID_MIN_OFF_PERIOD__NONE_ms;

//////////////////////////////////////////////////////////////////////////////////

//cool onboard TRIAC and blocking diode while charging
//required even if cabin air unfavorably heats or cools pack
//there isn't a temperature sensor near the TRIAC or diode, so empirical data is used to determine when to use fans
//  -At 23 degC, charging with 0.45 A grid charger leaves 85 degC headroom on TRIAC, and similar results on diode
//  -At 23 degC, charging with 2.10 A grid charger leaves 25 degC headroom on TRIAC, and 50 degC on diode
void runFansIfNeeded(void)
{
    uint8_t hysteresis_C = 0;

    if     (fan_getSpeed_now() == FAN_HIGH) { hysteresis_C = FAN_SPEED_HYSTERESIS_HIGH_degC; }
    else if(fan_getSpeed_now() == FAN_LOW ) { hysteresis_C = FAN_SPEED_HYSTERESIS_LOW_degC;  }
    else if(fan_getSpeed_now() == FAN_OFF ) { hysteresis_C = FAN_SPEED_HYSTERESIS_OFF_degC;  }

    if     ( (temperature_intake_getLatest()  < (GRID_CHARGING_FANS_OFF_BELOW_TEMP_C - hysteresis_C)) &&
             (temperature_ambient_getLatest() < (GRID_CHARGING_FANS_OFF_BELOW_TEMP_C - hysteresis_C)) &&
             (temperature_battery_getLatest() < (GRID_CHARGING_FANS_OFF_BELOW_TEMP_C - hysteresis_C))  ) { fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_OFF);  }

    else if( (temperature_intake_getLatest()  < (GRID_CHARGING_FANS_LOW_BELOW_TEMP_C - hysteresis_C)) &&
             (temperature_ambient_getLatest() < (GRID_CHARGING_FANS_LOW_BELOW_TEMP_C - hysteresis_C)) &&
             (temperature_battery_getLatest() < (GRID_CHARGING_FANS_LOW_BELOW_TEMP_C - hysteresis_C))  ) { fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_LOW);  }

    else                                                                                                 { fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_HIGH); }
}

//////////////////////////////////////////////////////////////////////////////////

uint16_t determineMaxAllowedCellVoltage(void)
{
    //prevents rapid grid charger enable/disable when cells full
    if(gpio_isGridChargerChargingNow() == YES) { return CELL_VMAX_GRIDCHARGER;                    }
    else                                       { return CELL_VMAX_GRIDCHARGER - VCELL_HYSTERESIS; }
}

//////////////////////////////////////////////////////////////////////////////////

uint8_t isChargingAllowed(void)
{
    //order is important
    //external checks
    if(gpio_isGridChargerPluggedInNow()    == NO                                       ) { return NO__CHARGER_UNPLUGGED;       }
    if(key_getSampledState()               == KEYSTATE_ON                              ) { return NO__KEY_IS_ON;               }
    //cell voltage checks
    if(LTC68042result_hiCellVoltage_get()   > CELL_VREST_85_PERCENT_SoC                ) { return NO__ATLEASTONECELL_TOO_HIGH; }
    if(LTC68042result_loCellVoltage_get()   < CELL_VMIN_GRIDCHARGER                    ) { return NO__ATLEASTONECELL_TOO_LOW;  }
    if(LTC68042result_hiCellVoltage_get()   > CELL_VMAX_GRIDCHARGER                    ) { return NO__ATLEASTONECELL_FULL;     }
    if(LTC68042result_hiCellVoltage_get()   > determineMaxAllowedCellVoltage()         ) { return NO__CELL_VOLTAGE_HYSTERESIS; }
    //thermal checks
    if(temperature_gridCharger_getLatest()  > DISABLE_GRIDCHARGING_ABOVE_CHARGER_TEMP_C) { return NO__CHARGER_IS_HOT;          }
    if(temperature_gridCharger_getLatest() == TEMPERATURE_SENSOR_FAULT_LO              ) { return NO__TEMP_UNPLUGGED_GRID;     }
    if(temperature_battery_getLatest()      < DISABLE_GRIDCHARGING_BELOW_BATTERY_TEMP_C) { return NO__BATTERY_IS_COLD;         }
    if(temperature_battery_getLatest()      > DISABLE_GRIDCHARGING_ABOVE_BATTERY_TEMP_C) { return NO__BATTERY_IS_HOT;          }
    if(temperature_intake_getLatest()       > DISABLE_GRIDCHARGING_ABOVE_INTAKE_TEMP_C ) { return NO__AIRINTAKE_IS_HOT;        }
    if(temperature_intake_getLatest()      == TEMPERATURE_SENSOR_FAULT_LO              ) { return NO__TEMP_UNPLUGGED_INTAKE;   }
    if(temperature_exhaust_getLatest()      > DISABLE_GRIDCHARGING_ABOVE_EXHAUST_TEMP_C) { return NO__TEMP_EXHAUST_IS_HOT;     }
    //time checks
    if((millis()                          ) < DISABLE_GRIDCHARGING_LIBCM_BOOT_DELAY_ms ) { return NO__LIBCMJUSTBOOTED;         }
    if((millis() - latestPlugin_ms        ) < DISABLE_GRIDCHARGING_PLUGIN_DELAY_ms     ) { return NO__JUSTPLUGGEDIN;           }
    if((millis() - latestChargerDisable_ms) < minGridOffPeriod_ms                      ) { return NO__RECENTLY_TURNED_OFF;     }

    return YES__CHARGING_ALLOWED; //nothing else returned, so we can charge
}

//////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: display status on LCD
void processChargerDisableReason(uint8_t canWeCharge)
{
    Serial.print(F("\nCharger disabled: "));
    uint32_t offPeriod_helper = GRID_MIN_OFF_PERIOD__LONG_ms; //most common value //overwritten as needed
    uint8_t buzzer_helper = BUZZER_OFF; //most common value //overwritten as needed

    switch (canWeCharge)
    {
        //external checks
        case NO__CHARGER_UNPLUGGED:       { Serial.print(F("Unplugged")         ); offPeriod_helper = GRID_MIN_OFF_PERIOD__NONE_ms; break; }
        case NO__KEY_IS_ON:               { Serial.print(F("Key is ON")         ); buzzer_helper = BUZZER_LOW;                      break; }
        //voltage issue
        case NO__ATLEASTONECELL_TOO_HIGH: { Serial.print(F("Overcharged")       ); buzzer_helper = BUZZER_HIGH;                     break; }
        case NO__ATLEASTONECELL_TOO_LOW:  { Serial.print(F("Overdischarged")    ); buzzer_helper = BUZZER_HIGH;                     break; }
        case NO__ATLEASTONECELL_FULL:     { Serial.print(F("Pack Charged")      );                                                  break; }
        case NO__CELL_VOLTAGE_HYSTERESIS: { Serial.print(F("Vcell Hysteresis")  );                                                  break; }
        //thermal issue
        case NO__CHARGER_IS_HOT:          { Serial.print(F("Charger Hot")       );                                                  break; }
        case NO__TEMP_UNPLUGGED_GRID:     { Serial.print(F("T_grid Unplugged")  );                                                  break; }
        case NO__BATTERY_IS_COLD:         { Serial.print(F("Pack Too Cold")     );                                                  break; }
        case NO__BATTERY_IS_HOT:          { Serial.print(F("Pack Too Hot")      );                                                  break; }
        case NO__AIRINTAKE_IS_HOT:        { Serial.print(F("Cabin Too Hot")     );                                                  break; }
        case NO__TEMP_UNPLUGGED_INTAKE:   { Serial.print(F("T_intake Unplugged"));                                                  break; }
        case NO__TEMP_EXHAUST_IS_HOT:     { Serial.print(F("Exhaust Too Hot")   );                                                  break; }
        //time issue
        case NO__JUSTPLUGGEDIN:           { Serial.print(F("Plugin Delay")      ); offPeriod_helper = GRID_MIN_OFF_PERIOD__NONE_ms; break; }
        case NO__LIBCMJUSTBOOTED:         { Serial.print(F("LiBCM Powerup")     ); offPeriod_helper = GRID_MIN_OFF_PERIOD__NONE_ms; break; }
        case NO__RECENTLY_TURNED_OFF:     { Serial.print(F("Turnoff Delay")     ); offPeriod_helper = minGridOffPeriod_ms;          break; }
        //unknown reason
        default:                          { Serial.print(F("Unknown Reason")    );                                                  break; }
    }

    minGridOffPeriod_ms = offPeriod_helper;
    buzzer_requestTone(BUZZER_REQUESTOR_GRIDCHARGER, buzzer_helper);
}

//////////////////////////////////////////////////////////////////////////////////

void chargerControlSignals_handler(void)
{
    static uint8_t isChargingAllowed_previous = NO__UNINITIALIZED;
           uint8_t isChargingAllowed_now      = isChargingAllowed();

    if(isChargingAllowed_now == YES__CHARGING_ALLOWED)
    {        
        if(isChargingAllowed_previous != YES__CHARGING_ALLOWED)
        {
            Serial.print(F("\nCharging"));
            adc_calibrateBatteryCurrentSensorOffset();
        }

        runFansIfNeeded(); //JTS2doNow: run fans as needed even when charging not allowed (e.g. to cool a hot pack)
        gpio_turnGridCharger_on();
        gpio_setGridCharger_powerLevel('H'); //JTS2doNow: Reduce power as temp increases
        buzzer_requestTone(BUZZER_REQUESTOR_GRIDCHARGER, BUZZER_OFF);
    }
    else
    {
        gpio_turnGridCharger_off();

        if(isChargingAllowed_now == NO__CHARGER_UNPLUGGED) { gpio_setGridCharger_powerLevel('Z'); } //saves power
        else                                               { gpio_setGridCharger_powerLevel('0'); } //redundant safety when charger plugged in but disabled

        if(isChargingAllowed_previous == YES__CHARGING_ALLOWED)
        {
            latestChargerDisable_ms = millis();
            //gpio_turnPowerSensors_off();
        }
        
        if(isChargingAllowed_previous != isChargingAllowed_now) { processChargerDisableReason(isChargingAllowed_now); }

        fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_OFF); //JTS2doNow: see note ("cool a hot pack")

        //JTS2doNow: Since the charger should be off now, sound an alarm if battery current isn't ~0 amps.
    }
    adc_updateBatteryCurrent();

    isChargingAllowed_previous = isChargingAllowed_now;
}

//////////////////////////////////////////////////////////////////////////////////

void handleEvent_plugin(void)
{
    Serial.print(F("Plugged In"));
    gpio_setGridCharger_powerLevel('0');
    gpio_turnPowerSensors_on(); //so we can measure current //to save power, it would be nice to move this into YES__CHARGING_ALLOWED (solve powerup hysteresis)
    latestPlugin_ms = millis();
	LiDisplay_gridChargerPluggedIn(); //JTS2doNow: Move inside LiDisplay.c... LiDisplay handler should check key state
}

//////////////////////////////////////////////////////////////////////////////////

void handleEvent_unplug(void)
{
    Serial.print(F("Unplugged"));
    gpio_turnGridCharger_off();
    gpio_setGridCharger_powerLevel('Z'); //reduces power consumption
    gpio_turnPowerSensors_off();
    fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_OFF);
    buzzer_requestTone(BUZZER_REQUESTOR_GRIDCHARGER, BUZZER_OFF);
    LiDisplay_gridChargerUnplugged();
}

//////////////////////////////////////////////////////////////////////////////////

void chargerPowerInput_handler(void)
{
    static bool isGridChargerPluggedIn_previous = NO;
           bool isGridChargerPluggedIn_now      = gpio_isGridChargerPluggedInNow();

    if(isGridChargerPluggedIn_previous != isGridChargerPluggedIn_now)
    {
        Serial.print(F("\nGrid: "));
        if(isGridChargerPluggedIn_now == YES) { handleEvent_plugin(); }
        else                                  { handleEvent_unplug(); }

        isGridChargerPluggedIn_previous = isGridChargerPluggedIn_now;
    }
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handler(void)
{
    chargerPowerInput_handler();
    chargerControlSignals_handler();
}
