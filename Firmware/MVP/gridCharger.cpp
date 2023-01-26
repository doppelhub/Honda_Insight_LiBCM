//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
#include "libcm.h"

//JTS2doLater: Feature: If SoC greater than 70% when grid charger first plugged in, then charge to 85% SoC.

uint32_t lastPlugin_ms = 0;
uint32_t latestChargerDisable_ms = 0;
uint32_t minimumOffTime_beforeTurningOn_ms = 0;
//////////////////////////////////////////////////////////////////////////////////

uint8_t isChargingAllowed(void)
{
    uint8_t helper = YES__CHARGING_ALLOWED;

    //cell voltage checks
    if     (LTC68042result_hiCellVoltage_get()    > CELL_VREST_85_PERCENT_SoC                ) { helper = NO__ATLEASTONECELL_TOO_HIGH; }
    else if(LTC68042result_loCellVoltage_get()    < CELL_VMIN_GRIDCHARGER                    ) { helper = NO__ATLEASTONECELL_TOO_LOW;  }
    else if(LTC68042result_hiCellVoltage_get()    > CELL_VMAX_GRIDCHARGER                    ) { helper = NO__ATLEASTONECELL_FULL;     }

    //thermal checks
    else if(temperature_gridCharger_getLatest()   > DISABLE_GRIDCHARGING_ABOVE_CHARGER_TEMP_C) { helper = NO__CHARGER_IS_HOT;          }
    else if(temperature_gridCharger_getLatest()  == TEMPERATURE_SENSOR_FAULT_LO              ) { helper = NO__TEMP_UNPLUGGED_GRID;     }
    else if(temperature_battery_getLatest()       < DISABLE_GRIDCHARGING_BELOW_BATTERY_TEMP_C) { helper = NO__BATTERY_IS_COLD;         }
    else if(temperature_battery_getLatest()       > DISABLE_GRIDCHARGING_ABOVE_BATTERY_TEMP_C) { helper = NO__BATTERY_IS_HOT;          }
    else if(temperature_intake_getLatest()        > DISABLE_GRIDCHARGING_ABOVE_INTAKE_TEMP_C ) { helper = NO__AIRINTAKE_IS_HOT;        }
    else if(temperature_intake_getLatest()       == TEMPERATURE_SENSOR_FAULT_LO              ) { helper = NO__TEMP_UNPLUGGED_INTAKE;   }
    else if(temperature_exhaust_getLatest()       > DISABLE_GRIDCHARGING_ABOVE_EXHAUST_TEMP_C) { helper = NO__TEMP_EXHAUST_IS_HOT;     }

    //time checks
    else if( (millis()                          ) < DISABLE_GRIDCHARGING_LIBCM_BOOT_DELAY_ms ) { helper = NO__LIBCMJUSTBOOTED;         }
    else if( (millis() - lastPlugin_ms          ) < DISABLE_GRIDCHARGING_PLUGIN_DELAY_ms     ) { helper = NO__JUSTPLUGGEDIN;           }
    else if( (millis() - latestChargerDisable_ms) < minimumOffTime_beforeTurningOn_ms        ) { helper = NO__RECENTLY_TURNED_OFF;     }

    return helper;
}

//////////////////////////////////////////////////////////////////////////////////

void reportChargerState(uint8_t canWeCharge)
{
    static uint8_t canWeCharge_previous = NO__UNINITIALIZED;

    //things to do no matter how long a single condition exists
    //JTS2doLater: The actions in this switch can be placed in the one below if we add a buzzer request handler (identical to fan handler)
    switch(canWeCharge)
    {
        //voltage issue
        case NO__ATLEASTONECELL_TOO_HIGH: { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_on_highFreq(); break; }
        case NO__ATLEASTONECELL_TOO_LOW:  { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_on_highFreq(); break; }
        case NO__ATLEASTONECELL_FULL:     { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_off();         break; }
        //thermal issue
        case NO__CHARGER_IS_HOT:          { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_off();         break; }
        case NO__TEMP_UNPLUGGED_GRID:     { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_off();         break; }
        case NO__BATTERY_IS_COLD:         { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_off();         break; }
        case NO__BATTERY_IS_HOT:          { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_off();         break; }
        case NO__AIRINTAKE_IS_HOT:        { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_off();         break; }
        case NO__TEMP_UNPLUGGED_INTAKE:   { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_off();         break; }
        case NO__TEMP_EXHAUST_IS_HOT:     { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_off();         break; }
        //time issue
        case NO__JUSTPLUGGEDIN:           { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_NONE_ms; gpio_turnBuzzer_off();         break; }
        case NO__LIBCMJUSTBOOTED:         { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_NONE_ms; gpio_turnBuzzer_off();         break; }
        case NO__RECENTLY_TURNED_OFF:     { /*    keep existing value      */                              gpio_turnBuzzer_off();         break; }
        //all systems go
        case YES__CHARGING_ALLOWED:       { /*  no need to update value    */                              gpio_turnBuzzer_off();         break; }
        //unknown reason
        default:                          { minimumOffTime_beforeTurningOn_ms = GRID_MIN_TIME_OFF_LONG_ms; gpio_turnBuzzer_on_highFreq(); break; }
    }

    //things to do one time when disable condition first occurs
    if( (canWeCharge != canWeCharge_previous ) &&
        (canWeCharge != YES__CHARGING_ALLOWED)  )
    {
        Serial.print(F("\nGrid charger disabled: "));
        switch (canWeCharge)
        {
            //voltage issue
            case NO__ATLEASTONECELL_TOO_HIGH: { Serial.print(F("Overcharged")   ); break; } //JTS2doLater: display Warning on LCD
            case NO__ATLEASTONECELL_TOO_LOW:  { Serial.print(F("Overdischarged")); break; } //JTS2doLater: display Warning on LCD
            case NO__ATLEASTONECELL_FULL:     { Serial.print(F("Pack Charged")  ); break; }
            //thermal issue
            case NO__CHARGER_IS_HOT:          { Serial.print(F("Charger Hot")       ); break; }
            case NO__TEMP_UNPLUGGED_GRID:     { Serial.print(F("T_grid Unplugged")  ); break; }
            case NO__BATTERY_IS_COLD:         { Serial.print(F("Pack Too Cold")     ); break; }
            case NO__BATTERY_IS_HOT:          { Serial.print(F("Pack Too Hot")      ); break; }
            case NO__AIRINTAKE_IS_HOT:        { Serial.print(F("Cabin Too Hot")     ); break; }
            case NO__TEMP_UNPLUGGED_INTAKE:   { Serial.print(F("T_intake Unplugged")); break; }
            case NO__TEMP_EXHAUST_IS_HOT:     { Serial.print(F("Exhaust Too Hot")   ); break; }
            //time issue
            case NO__JUSTPLUGGEDIN:           { Serial.print(F("Plugin Delay")  ); break; }
            case NO__LIBCMJUSTBOOTED:         { Serial.print(F("LiBCM Powerup") ); break; }
            case NO__RECENTLY_TURNED_OFF:     { Serial.print(F("Turnoff Delay") ); break; }
            //unknown reason
            default:                          { Serial.print(F("Unknown Reason")); break; }
        }

        canWeCharge_previous = canWeCharge;
    }
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
    static bool gridChargerEnabled_previous = NO;

    uint8_t areAllSystemsGo = isChargingAllowed();

    if(areAllSystemsGo == YES__CHARGING_ALLOWED)
    {
        if( LTC68042result_hiCellVoltage_get() <= (CELL_VMAX_GRIDCHARGER - VCELL_HYSTERESIS) ) //hysteresis to prevent rapid grid charger cycling
        {
            runFansIfNeeded();
            gpio_turnGridCharger_on();
            gpio_setGridCharger_powerLevel('H');

            if(gridChargerEnabled_previous == NO)
            {   
                Serial.print(F("\nCharger: ON" ));
                gridChargerEnabled_previous = YES;
            }
        }
    }
    else
    {
        //something is unhappy //turn everything off
        fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_OFF);
        gpio_turnGridCharger_off();
        gpio_setGridCharger_powerLevel('0');
        
        if(gridChargerEnabled_previous == YES)
        {
            Serial.print(F("\nCharger: OFF"));
            latestChargerDisable_ms = millis();
            gridChargerEnabled_previous = NO;
        }
    }

    reportChargerState(areAllSystemsGo); //non-critical tasks go here

    lcd_refresh();
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
    lcd_displayOn();
    lastPlugin_ms = millis();
}

//////////////////////////////////////////////////////////////////////////////////

void handleEvent_unplug(void)
{
    Serial.print(F("Unplugged"));
    gpio_turnGridCharger_off();
    lcd_displayOFF();
    gpio_setGridCharger_powerLevel('H'); //reduces power consumption
    gpio_turnBuzzer_off(); //if issues persist, something else will turn buzzer back on
    fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_OFF);
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handler(void)
{
    static uint8_t isGridChargerPluggedIn_previous = NO;
           uint8_t isGridChargerPluggedIn          = gpio_isGridChargerPluggedInNow();

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