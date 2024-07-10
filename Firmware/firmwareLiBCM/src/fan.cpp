//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles fans
//LiBCM allows QTY4 different subsystems to independently control fan speed.
//If energy is available, the fans will spin at the highest requested speed (OFF/LOW/MED/HIGH)

#include "libcm.h"

uint8_t fanSpeed_now  = FAN_OFF; //actual fan state now (OFF/LOW/MED/HIGH)
uint8_t fanSpeed_goal = FAN_OFF; //desired fan state (highest speed requested by any subsystem)

uint8_t fanSpeed_allRequestors = FAN_FORCE_OFF; //each subsystem's fan speed request is stored in 2 bits (see 'FAN_REQUESTOR' constants)

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t fan_getSpeed_now(void)  { return fanSpeed_now;  }

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t fan_getAllRequestors_mask(void) { return fanSpeed_allRequestors; }

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t numBitshifts_thisRequestor(uint8_t requestor)
{
    uint8_t numShifts = 0;

    switch (requestor)
    {
        case FAN_REQUESTOR_BATTERY:     numShifts = FAN_REQUESTOR_BITSHIFTS_BATTERY;     break;
        case FAN_REQUESTOR_USER:        numShifts = FAN_REQUESTOR_BITSHIFTS_USER;        break;
        case FAN_REQUESTOR_GRIDCHARGER: numShifts = FAN_REQUESTOR_BITSHIFTS_GRIDCHARGER; break;
        case FAN_REQUESTOR_CELLBALANCE: numShifts = FAN_REQUESTOR_BITSHIFTS_CELLBALANCE; break;
    }

    return numShifts;
}

/////////////////////////////////////////////////////////////////////////////////////////

void fan_requestSpeed(uint8_t requestor, char newFanSpeed)
{
    uint8_t fanSpeedOtherSubsystems = fanSpeed_allRequestors & ~(requestor); //mask out this requestor's previous request
    uint8_t fanSpeedThisRequestor = newFanSpeed<<numBitshifts_thisRequestor(requestor); //left shift to requestor's position in memory
    fanSpeed_allRequestors = (fanSpeedThisRequestor | fanSpeedOtherSubsystems); //combine the above
}

/////////////////////////////////////////////////////////////////////////////////////////

//intake temp sensor only accurately measures cabin air temp when fans are on or have only recently turned off
bool isIntakeTempSensorMeasuringCabinTemp(void)
{
    bool isIntakeTempValid = NO;

    static uint32_t lastTimeFanTurned_off_ms = 0;

    //capture timestamp each time fan state changes
    static uint8_t fanSpeed_previous = FAN_OFF;
    if (fanSpeed_now != fanSpeed_previous)
    {
        //fan speed just changed
        if (fanSpeed_now == FAN_OFF){ lastTimeFanTurned_off_ms = millis(); }

        fanSpeed_previous = fanSpeed_now;
    }

    uint32_t timeSinceFansTurnedOff_ms = (millis() - lastTimeFanTurned_off_ms);

    //determine if air plenum contains cabin temp air
    if (fanSpeed_now != FAN_OFF) { isIntakeTempValid = YES; } //fan is on.  Intake temp sensor might not be at cabin temp yet, but probably good enough
    else
    {
        if (timeSinceFansTurnedOff_ms < FAN_TIME_OFF_BEFORE_INTAKE_TEMP_INVALID_ms ) { isIntakeTempValid = YES; } //air inside intake plenum is still at cabin temp
    }

    return isIntakeTempValid;
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t relativePackTemp(void)
{
    #define PACKTEMP_DELTA_UNKNOWN      255
    #define PACKTEMP_HOTTER_THAN_INTAKE   3
    #define PACKTEMP_CLOSE_TO_INTAKE      5
    #define PACKTEMP_COLDER_THAN_INTAKE   9

    int8_t packTemp = temperature_battery_getLatest();
    int8_t intakeTemp = temperature_intake_getLatest();

    uint8_t relativeDelta = PACKTEMP_DELTA_UNKNOWN;

    if      (packTemp == TEMPERATURE_SENSOR_FAULT_HI                      ) { relativeDelta = PACKTEMP_DELTA_UNKNOWN;      }
    else if (packTemp >  (intakeTemp + MIN_CABIN_AIR_DELTA_FOR_FANS_degC) ) { relativeDelta = PACKTEMP_HOTTER_THAN_INTAKE; }
    else if (packTemp >  (intakeTemp - MIN_CABIN_AIR_DELTA_FOR_FANS_degC) ) { relativeDelta = PACKTEMP_CLOSE_TO_INTAKE;    }
    else if (packTemp >  TEMPERATURE_SENSOR_FAULT_LO                      ) { relativeDelta = PACKTEMP_COLDER_THAN_INTAKE; }

    return relativeDelta;
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t packTempState(void)
{
    #define PACKTEMP_UNKNOWN 255
    #define PACKTEMP_RUNAWAY   1
    #define PACKTEMP_HOT       2
    #define PACKTEMP_WARM      4
    #define PACKTEMP_OK        8
    #define PACKTEMP_COOL     16
    #define PACKTEMP_COLD     32

    uint8_t status = PACKTEMP_UNKNOWN;
    int8_t packTemp = temperature_battery_getLatest();

    if      (packTemp > TEMPERATURE_PACK_IN_THERMAL_RUNAWAY                            ) { status = PACKTEMP_RUNAWAY; }
    else if (packTemp > temperature_coolBatteryAbove_C() +  FAN_DELTA_T_HIGH_SPEED_degC) { status = PACKTEMP_HOT;  }
    else if (packTemp > temperature_coolBatteryAbove_C()                               ) { status = PACKTEMP_WARM; }
    else if (packTemp > temperature_heatBatteryBelow_C()                               ) { status = PACKTEMP_OK;   }
    else if (packTemp > temperature_heatBatteryBelow_C() -  FAN_DELTA_T_HIGH_SPEED_degC) { (heater_isConnected() == HEATER_NOT_CONNECTED)?(status=PACKTEMP_COOL):(status = PACKTEMP_OK); }
    else if (packTemp > TEMPERATURE_SENSOR_FAULT_LO                                    ) { (heater_isConnected() == HEATER_NOT_CONNECTED)?(status=PACKTEMP_COLD):(status = PACKTEMP_OK); }

    return status;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool doesPackWantFans(void)
{
    uint8_t request = packTempState();
    if ( (request == PACKTEMP_HOT)  ||
         (request == PACKTEMP_WARM) ||
         (request == PACKTEMP_COOL) ||
         (request == PACKTEMP_COLD)  ) { return YES; }
    else                               { return  NO; }
}

/////////////////////////////////////////////////////////////////////////////////////////

uint32_t howOftenToSampleCabinAir(void)
{
    uint32_t sampleCabinTempAtLeastEvery_ms = 0;

    if      (key_getSampledState() == KEYSTATE_ON   ) { sampleCabinTempAtLeastEvery_ms = SAMPLE_CABIN_AIR_INTERVAL_KEY_ON_ms;    }
    else if (gpio_isGridChargerPluggedInNow() == YES) { sampleCabinTempAtLeastEvery_ms = SAMPLE_CABIN_AIR_INTERVAL_PLUGGEDIN_ms; }
    else                                              { sampleCabinTempAtLeastEvery_ms = SAMPLE_CABIN_AIR_INTERVAL_KEY_OFF_ms;   }

    return sampleCabinTempAtLeastEvery_ms;
}

/////////////////////////////////////////////////////////////////////////////////////////

//if the battery is too hot or cold, periodically run fans if they haven't run recently
//required for intake air temp sensor to actually measure cabin air temp
uint8_t periodicallyRunFans(void)
{
    uint8_t request = FAN_OFF;

    static uint32_t lastTimeThisFunctionRequestedFan_ms = MILLIS_MAXIMUM_VALUE - SAMPLE_CABIN_AIR_INTERVAL_KEY_OFF_ms; //initial value causes this to run immediately on powerup

    if ((millis() - lastTimeThisFunctionRequestedFan_ms) < FAN_TIME_ON_TO_SAMPLE_CABIN_AIR_ms) { request = FAN_LOW; }
    if ((millis() - lastTimeThisFunctionRequestedFan_ms) > howOftenToSampleCabinAir()        ) { request = FAN_LOW; lastTimeThisFunctionRequestedFan_ms = millis(); }

    return request;
}

/////////////////////////////////////////////////////////////////////////////////////////

void updateFanRequest_battery(void)
{
    uint8_t request = FAN_OFF;

    if (isIntakeTempSensorMeasuringCabinTemp() == YES)
    {
        if      ( (packTempState() == PACKTEMP_HOT ) && (relativePackTemp() == PACKTEMP_HOTTER_THAN_INTAKE) ) { request = FAN_HIGH; }
        else if ( (packTempState() == PACKTEMP_WARM) && (relativePackTemp() == PACKTEMP_HOTTER_THAN_INTAKE) ) { request = FAN_LOW;  }
        else if ( (packTempState() == PACKTEMP_COOL) && (relativePackTemp() == PACKTEMP_COLDER_THAN_INTAKE) ) { request = FAN_LOW;  }
        else if ( (packTempState() == PACKTEMP_COLD) && (relativePackTemp() == PACKTEMP_COLDER_THAN_INTAKE) ) { request = FAN_HIGH; }
    }
    else
    {
        //periodically move cabin air into intake plenum
        if (doesPackWantFans() == YES) { request = periodicallyRunFans(); }
    }

    fan_requestSpeed(FAN_REQUESTOR_BATTERY, request);
}

/////////////////////////////////////////////////////////////////////////////////////////

void determineFastestFanSpeedRequest(void)
{
    const uint8_t fan_hi_bits = (fanSpeed_allRequestors & FAN_HI_MASK) >> 1; //shift to align with lo_bits
    const uint8_t fan_lo_bits = (fanSpeed_allRequestors & FAN_LO_MASK);

    if      (fan_hi_bits & fan_lo_bits) { fanSpeed_goal = FAN_HIGH; } //at least one subsystem is requesting high speed
    else if (fan_hi_bits)               { fanSpeed_goal = FAN_MED;  } //at least one subsystem is requesting medium speed
    else if (fan_lo_bits)               { fanSpeed_goal = FAN_LOW;  } //at least one subsystem is requesting low speed
    else                                { fanSpeed_goal = FAN_OFF;  } //no subsystem is requesting fan
}

/////////////////////////////////////////////////////////////////////////////////////////

bool hasEnoughTimePassedToChangeFanSpeed(void)
{
    static uint32_t latestFanSpeedChange_ms = 0;

    static uint8_t fanSpeed_goal_previous = FAN_OFF;

    bool hasEnoughTimePassed = YES;

    if (fanSpeed_goal != fanSpeed_goal_previous)
    {
        const uint32_t hysteresisTarget_ms = (fanSpeed_goal > fanSpeed_goal_previous) ?
            FAN_SPEED_INCREASE_HYSTERESIS_ms : FAN_SPEED_DECREASE_HYSTERESIS_ms;

        const uint32_t currentTime_ms = millis();

        if ((currentTime_ms - latestFanSpeedChange_ms) < hysteresisTarget_ms) { hasEnoughTimePassed = NO; }
        else
        {
            // enough time has passed
            latestFanSpeedChange_ms = currentTime_ms;
            fanSpeed_goal_previous = fanSpeed_goal;
        }
    }

    return hasEnoughTimePassed;
}

/////////////////////////////////////////////////////////////////////////////////////////

void fan_handler(void)
{
    updateFanRequest_battery();

    determineFastestFanSpeedRequest(); //result stored in fanSpeed_goal

    if (SoC_isThermalManagementAllowed() == NO) { fanSpeed_now = FAN_OFF; } //not enough energy to run fans
    else
    {
        if ((fanSpeed_now != fanSpeed_goal)                &&
            (hasEnoughTimePassedToChangeFanSpeed() == YES)  ) { fanSpeed_now = fanSpeed_goal; }
    }

    #ifdef BATTERY_TYPE_5AhG3
        gpio_setFanSpeed_OEM(fanSpeed_now);
    #elif defined BATTERY_TYPE_47AhFoMoCo
        //OEM battery fan is removed in FoMoCo systems.  The battery fan circuitry is repurposed to allow LiBCM to control the PDU fan
        //Note that the MCM retains its OEM behavior (i.e. it can still control the PDU fan, too).
        //JTS2doLater: Add new fan handler specifically for OEM fan... the direct gpio functions used below will only work properly if no other subsystem calls them
        //JTS2doLater: Enable  high speed if more than 500 kJ have flowed through the IGBT stage over the previous 60 seconds.
        //JTS2doLater: Disable high speed if less than 250 kJ have flowed through the IGBT stage over the previous 60 seconds.
        if ((key_getSampledState() == KEYSTATE_ON)                         &&
            (time_sinceLatestKeyOn_ms() > FAN_SPEED_INCREASE_HYSTERESIS_ms) )
        {
            gpio_setFanSpeed_OEM(FAN_LOW); //PDU fan defaults to low speed when keyON
        }
        else { gpio_setFanSpeed_OEM(FAN_OFF); }
    #endif

    gpio_setFanSpeed_PCB(fanSpeed_now);
}

/////////////////////////////////////////////////////////////////////////////////////////
