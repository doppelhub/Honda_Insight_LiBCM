//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
#include "libcm.h"

//JTS2doLater: Feature: If SoC greater than 70% when grid charger first plugged in, then charge to 85% SoC.

uint32_t lastPlugin_ms = 0;
uint32_t latestChargerDisable_ms = 0;
uint32_t minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_NONE_ms;
//////////////////////////////////////////////////////////////////////////////////

uint8_t isChargingAllowed(void)
{
    uint8_t helper = YES__CHARGING_ALLOWED;

    uint8_t buzzerToneRequest = BUZZER_OFF;

    //cell voltage checks
    if     (LTC68042result_hiCellVoltage_get()    > CELL_VREST_85_PERCENT_SoC                ) { helper = NO__ATLEASTONECELL_TOO_HIGH; buzzerToneRequest = BUZZER_HIGH;                                                        }
    else if(LTC68042result_loCellVoltage_get()    < CELL_VMIN_GRIDCHARGER                    ) { helper = NO__ATLEASTONECELL_TOO_LOW;  buzzerToneRequest = BUZZER_HIGH;                                                        }
    else if(LTC68042result_hiCellVoltage_get()    > CELL_VMAX_GRIDCHARGER                    ) { helper = NO__ATLEASTONECELL_FULL;     buzzerToneRequest = (buzzer_getTone_now() ? BUZZER_OFF : BUZZER_LOW);                   }

    //thermal checks
    else if(temperature_gridCharger_getLatest()   > DISABLE_GRIDCHARGING_ABOVE_CHARGER_TEMP_C) { helper = NO__CHARGER_IS_HOT;                                                                                                  }
    else if(temperature_gridCharger_getLatest()  == TEMPERATURE_SENSOR_FAULT_LO              ) { helper = NO__TEMP_UNPLUGGED_GRID;                                                                                             }
    else if(temperature_battery_getLatest()       < DISABLE_GRIDCHARGING_BELOW_BATTERY_TEMP_C) { helper = NO__BATTERY_IS_COLD;                                                                                                 }
    else if(temperature_battery_getLatest()       > DISABLE_GRIDCHARGING_ABOVE_BATTERY_TEMP_C) { helper = NO__BATTERY_IS_HOT;                                                                                                  }
    else if(temperature_intake_getLatest()        > DISABLE_GRIDCHARGING_ABOVE_INTAKE_TEMP_C ) { helper = NO__AIRINTAKE_IS_HOT;                                                                                                }
    else if(temperature_intake_getLatest()       == TEMPERATURE_SENSOR_FAULT_LO              ) { helper = NO__TEMP_UNPLUGGED_INTAKE;                                                                                           }
    else if(temperature_exhaust_getLatest()       > DISABLE_GRIDCHARGING_ABOVE_EXHAUST_TEMP_C) { helper = NO__TEMP_EXHAUST_IS_HOT;                                                                                             }

    //time checks
    else if( (millis()                          ) < DISABLE_GRIDCHARGING_LIBCM_BOOT_DELAY_ms ) { helper = NO__LIBCMJUSTBOOTED;                                                                                                 }
    else if( (millis() - lastPlugin_ms          ) < DISABLE_GRIDCHARGING_PLUGIN_DELAY_ms     ) { helper = NO__JUSTPLUGGEDIN;                                                                                                   }
    else if( (millis() - latestChargerDisable_ms) < minimumOffTime_beforeTurningOn_ms        ) { helper = NO__RECENTLY_TURNED_OFF;                                                                                             }

    //other checks
    else if(key_getSampledState() == KEYSTATE_ON                                             ) { helper = NO__KEY_IS_ON;               buzzerToneRequest = ((buzzer_getTone_now() != BUZZER_HIGH) ? BUZZER_HIGH : BUZZER_LOW); }

    buzzer_requestTone(BUZZER_REQUESTOR_GRIDCHARGER, buzzerToneRequest);

    return helper;
}

//////////////////////////////////////////////////////////////////////////////////

void reportChargerState(uint8_t canWeCharge)
{
    Serial.print(F("\nGrid charger disabled: "));
    uint32_t minimumOffTime_ms = GRID_MIN_TIME_OFF_LONG_ms; //most common value; will be overwritten as needed

    switch (canWeCharge)
    {
        //voltage issue
        case NO__ATLEASTONECELL_TOO_HIGH: { Serial.print(F("Overcharged")       );                                                        break; } //JTS2doLater: display Warning on LCD
        case NO__ATLEASTONECELL_TOO_LOW:  { Serial.print(F("Overdischarged")    );                                                        break; } //JTS2doLater: display Warning on LCD
        case NO__ATLEASTONECELL_FULL:     { Serial.print(F("Pack Charged")      );                                                        break; }
        //thermal issue
        case NO__CHARGER_IS_HOT:          { Serial.print(F("Charger Hot")       );                                                        break; }
        case NO__TEMP_UNPLUGGED_GRID:     { Serial.print(F("T_grid Unplugged")  );                                                        break; }
        case NO__BATTERY_IS_COLD:         { Serial.print(F("Pack Too Cold")     );                                                        break; }
        case NO__BATTERY_IS_HOT:          { Serial.print(F("Pack Too Hot")      );                                                        break; }
        case NO__AIRINTAKE_IS_HOT:        { Serial.print(F("Cabin Too Hot")     );                                                        break; }
        case NO__TEMP_UNPLUGGED_INTAKE:   { Serial.print(F("T_intake Unplugged"));                                                        break; }
        case NO__TEMP_EXHAUST_IS_HOT:     { Serial.print(F("Exhaust Too Hot")   );                                                        break; }
        //time issue
        case NO__JUSTPLUGGEDIN:           { Serial.print(F("Plugin Delay")      ); minimumOffTime_ms = GRID_MIN_TIME_OFF_NONE_ms;         break; }
        case NO__LIBCMJUSTBOOTED:         { Serial.print(F("LiBCM Powerup")     ); minimumOffTime_ms = GRID_MIN_TIME_OFF_NONE_ms;         break; }
        case NO__RECENTLY_TURNED_OFF:     { Serial.print(F("Turnoff Delay")     ); minimumOffTime_ms = minimumOffTime_beforeTurningOn_ms; break; }
        //other checks
        case NO__KEY_IS_ON:               { Serial.print(F("Key is ON")         ); minimumOffTime_ms = GRID_MIN_TIME_OFF_NONE_ms;         break; }
        //unknown reason
        default:                          { Serial.print(F("Unknown Reason")    );                                                        break; }
    }

    minimumOffTime_beforeTurningOn_ms = minimumOffTime_ms;
}

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

void powered_handler(void)
{
    static uint8_t chargerState_previous = NO__UNINITIALIZED;

    uint8_t areAllSystemsGo = isChargingAllowed();

    if(areAllSystemsGo == YES__CHARGING_ALLOWED)
    {
        if( LTC68042result_hiCellVoltage_get() <= (CELL_VMAX_GRIDCHARGER - VCELL_HYSTERESIS) ) //hysteresis to prevent rapid grid charger cycling
        {
            runFansIfNeeded();
            gpio_turnGridCharger_on();
            gpio_setGridCharger_powerLevel('H'); //JTS2doNow: Reduce duty cycle as temp increases

            if(chargerState_previous != YES__CHARGING_ALLOWED)
            {
                Serial.print(F("\nCharger: ON" ));
                chargerState_previous = YES__CHARGING_ALLOWED;
            }
        }
    }
    else
    {
        //something is unhappy //turn everything off
        fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_OFF);
        gpio_turnGridCharger_off();
        gpio_setGridCharger_powerLevel('0');


        if(areAllSystemsGo != chargerState_previous)
        {
            if(chargerState_previous == YES__CHARGING_ALLOWED)
            {
                Serial.print(F("\nCharger: OFF"));
                latestChargerDisable_ms = millis();
            }

            reportChargerState(areAllSystemsGo);

            chargerState_previous = areAllSystemsGo;
        }

    }


}

//////////////////////////////////////////////////////////////////////////////////

void unpowered_handler(void)
{
    gpio_turnGridCharger_off(); //safety: continuously turn grid charger off in case a bit flip occurs in RAM (that would otherwise turn it on)
}

//////////////////////////////////////////////////////////////////////////////////

void handleEvent_plugin(void)
{
    Serial.print(F("Plugged In"));
    gpio_setGridCharger_powerLevel('0');
    lastPlugin_ms = millis();
	LiDisplay_gridChargerPluggedIn(); //JTS2doNow: Move inside LiDisplay.c... LiDisplay handler should check key state
}

//////////////////////////////////////////////////////////////////////////////////

void handleEvent_unplug(void)
{
    Serial.print(F("Unplugged"));
    gpio_turnGridCharger_off();
    buzzer_requestTone(BUZZER_REQUESTOR_GRIDCHARGER, BUZZER_OFF);
    gpio_setGridCharger_powerLevel('Z'); //reduces power consumption
    fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_OFF);
    LiDisplay_gridChargerUnplugged();
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handler(void)
{
    static bool isGridChargerPluggedIn_previous = NO;
           bool isGridChargerPluggedIn          = gpio_isGridChargerPluggedInNow();

    if(isGridChargerPluggedIn_previous != isGridChargerPluggedIn)
    {
        Serial.print(F("\nGrid: "));

        if(isGridChargerPluggedIn == YES) { handleEvent_plugin(); }
        else                              { handleEvent_unplug(); }

        isGridChargerPluggedIn_previous = isGridChargerPluggedIn;
    }

    if(isGridChargerPluggedIn == YES) { powered_handler();   }
    else                              { unpowered_handler(); }
}

//////////////////////////////////////////////////////////////////////////////////
