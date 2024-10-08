//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//stores various system millisecond timers

#include "libcm.h"

uint8_t loopPeriod_ms = TIME_DEFAULT_LOOP_PERIOD_ms; //JTS2doLater: see how long period can be, then change to constant

uint32_t timestamp_latestKeyOn_ms             = 0;
uint32_t timestamp_latestKeyOff_ms            = 0;
uint32_t timestamp_latestUserInputUSB_ms      = 0;
uint32_t timestamp_latestGridChargerUnplug_ms = 0;

bool LiBCM_justBooted = YES;
bool isItTimeToPerformKeyOffTasks = NO;

uint8_t loopCounter_8b = 0;

/////////////////////////////////////////////////////////////////////////////////////////

bool time_didLiBCM_justBoot(void) { return LiBCM_justBooted; }

/////////////////////////////////////////////////////////////////////////////////////////

void     time_latestKeyOn_ms_set(uint32_t keyOnTime) { timestamp_latestKeyOn_ms = keyOnTime; }
uint32_t time_latestKeyOn_ms_get(void)        { return timestamp_latestKeyOn_ms;             }

uint32_t time_sinceLatestKeyOn_ms(void)      { return millis() - timestamp_latestKeyOn_ms; }
uint16_t time_sinceLatestKeyOn_seconds(void) { return time_sinceLatestKeyOn_ms() * 0.001;  }

/////////////////////////////////////////////////////////////////////////////////////////

void     time_latestKeyOff_ms_set(uint32_t keyOffTime) { timestamp_latestKeyOff_ms = keyOffTime; }
uint32_t time_latestKeyOff_ms_get(void)         { return timestamp_latestKeyOff_ms;              }

/////////////////////////////////////////////////////////////////////////////////////////

void    time_loopPeriod_ms_set(uint8_t period_ms) { loopPeriod_ms = period_ms; }
uint8_t time_loopPeriod_ms_get(void)       { return loopPeriod_ms; }

/////////////////////////////////////////////////////////////////////////////////////////

void          time_latestUserInputUSB_set(void) { timestamp_latestUserInputUSB_ms = millis(); }
uint32_t time_sinceLatestUserInputUSB_get_ms(void) { return millis() - timestamp_latestUserInputUSB_ms; }

/////////////////////////////////////////////////////////////////////////////////////////

void          time_latestGridChargerUnplug_set(void) { timestamp_latestGridChargerUnplug_ms = millis(); }
uint32_t time_sinceLatestGridChargerUnplug_get_ms(void) { return millis() - timestamp_latestGridChargerUnplug_ms; }

/////////////////////////////////////////////////////////////////////////////////////////

bool time_isItTimeToPerformKeyOffTasks(void) { return isItTimeToPerformKeyOffTasks; }

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t time_getLoopCount_8b(void) { return loopCounter_8b; }

/////////////////////////////////////////////////////////////////////////////////////////

//make up lost time on millis counter, due to slower clock while asleep
void time_addSleepPeriodToMillis(void)
{
    const uint16_t TIMER2_8b_OVERFLOW_COUNTS = 256;
    const uint16_t MILLISECONDS_PER_SECOND = 1000;
    const uint8_t SYSTEM_CLOCK_PRESCALER = 64;
    const uint16_t TIMER2_CLOCK_PRESCALER = 1024;
    const uint16_t TIMER2_INCREMENT_FREQUENCY_Hz = (F_CPU / SYSTEM_CLOCK_PRESCALER) / TIMER2_CLOCK_PRESCALER;
    const uint8_t FUDGE_FACTOR_ms = 50; //based on observed measurements

    const uint16_t SLEEP_PERIOD_ms = FUDGE_FACTOR_ms + ((uint32_t)TIMER2_8b_OVERFLOW_COUNTS * MILLISECONDS_PER_SECOND) / TIMER2_INCREMENT_FREQUENCY_Hz;

    time_setAbsoluteMillis(millis() + SLEEP_PERIOD_ms);
}

/////////////////////////////////////////////////////////////////////////////////////////

void updateKeyOffTaskFlag(void)
{
    static uint32_t timestamp_lastUpdate_ms = 0;
    uint32_t keyOffUpdatePeriod_ms = KEY_OFF_UPDATE_PERIOD_TEN_MINUTES_ms;

    if ( ((cellBalance_areCellsBalancing() == YES) && (SoC_getBatteryStateNow_percent() > CELL_BALANCE_MIN_SoC)) ||
         ((cellBalance_areCellsBalancing() == YES) && (gpio_isGridChargerPluggedInNow() == YES)                ) ||
         ((gpio_isGridChargerChargingNow() == YES)                                                             )  )
    { 
        keyOffUpdatePeriod_ms = KEY_OFF_UPDATE_PERIOD_ONE_SECOND_ms; //if over 1800 ms, LTC ICs will turn off (bad)
    } 
  
    //Has enough time passed yet?
    if ((millis() - timestamp_lastUpdate_ms) > keyOffUpdatePeriod_ms)
    { 
        isItTimeToPerformKeyOffTasks = YES;
        timestamp_lastUpdate_ms = millis();
    }
    else { isItTimeToPerformKeyOffTasks = NO; }
}

/////////////////////////////////////////////////////////////////////////////////////////

bool time_hasKeyBeenOffLongEnough_toTurnOffLiBCM(void)
{
    bool keyOffForLongEnough = false;

    if ((millis() - time_latestKeyOff_ms_get()) > (POWEROFF_DELAY_AFTER_KEYOFF_PACK_EMPTY_MINUTES * (uint32_t)60000))
    {
        keyOffForLongEnough = true;
    }

    return keyOffForLongEnough;
}

/////////////////////////////////////////////////////////////////////////////////////////

//loop execution time (0.9.0c): 8.0 ms with 4x20 enabled
//loop execution time (0.9.0c): 2.8 ms with no display
void time_waitForLoopPeriod(void)
{
    static uint32_t timestamp_loopStart_previous_ms = 0;

    uint32_t timeNow_ms = millis();

    bool timingMet = false;

    LiBCM_justBooted = NO;
    loopCounter_8b++;

    LED(4,OFF); //LED4 brightness proportional to CPU time
    while ((uint32_t)(timeNow_ms - timestamp_loopStart_previous_ms) < time_loopPeriod_ms_get())
    {
        //wait here to start next loop
        timeNow_ms = millis();
        timingMet = true;
    }
    LED(4,ON);

    timestamp_loopStart_previous_ms = timeNow_ms;
        
    if ((key_getSampledState() == KEYSTATE_ON) && (timingMet == false)) { Serial.print('*'); }
}

/////////////////////////////////////////////////////////////////////////////////////////

//calculate delta between start and stop time
//store start time: START_TIMER
//calculate delta:  STOP_TIMER
void time_stopwatch(bool timerAction)
{
    static uint32_t startTime = 0;

    if (timerAction == START_TIMER) { startTime = millis(); }
    else
    {
        uint32_t stopTime = millis();
        Serial.print(F("\nDelta: "));
        Serial.print(stopTime - startTime);
        Serial.print(F(" ms\n"));
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

//only works from 1 to 255 Hz
uint16_t time_hertz_to_milliseconds(uint8_t hertz)
{
    uint16_t milliseconds = 0;

    if (hertz != 0) { milliseconds = 1000 / hertz; } //division

    return milliseconds;
}

/////////////////////////////////////////////////////////////////////////////////////////


void time_setAbsoluteMillis(uint32_t newMillis)
{
    extern volatile uint32_t timer0_millis;
    
    noInterrupts();
    timer0_millis = newMillis;
    interrupts();
}

/////////////////////////////////////////////////////////////////////////////////////////

void time_handler(void)
{
    if (key_getSampledState() == KEYSTATE_OFF) { updateKeyOffTaskFlag(); }
}

/////////////////////////////////////////////////////////////////////////////////////////
