//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//stores various system millisecond timers

#include "libcm.h"

uint8_t loopPeriod_ms = 10; //JTS2doLater: see how long period can be, then make this a constant

////////////////////////////////////////////////////////////////////////////////////

void time_loopPeriod_ms_set(uint8_t period_ms) { loopPeriod_ms = period_ms; }
uint8_t time_loopPeriod_ms_get(void) { return loopPeriod_ms; }

////////////////////////////////////////////////////////////////////////////////////

bool time_toUpdate_keyOffValues(void)
{
  uint32_t keyOffUpdatePeriod_ms = 0;
  static uint32_t timestamp_lastUpdate_ms = 0;
  bool isItTimeToUpdate = false;

  //determine period between LTC cell voltage reads (saves power)
  if( ((cellBalance_areCellsBalanced() == false) && (SoC_getBatteryStateNow_percent() > CELL_BALANCE_MIN_SoC)) ||
      ((cellBalance_areCellsBalanced() == false) && (gpio_isGridChargerPluggedInNow() == YES)                ) ||
      (gpio_isGridChargerChargingNow() == YES                                                                )  )
  { 
    keyOffUpdatePeriod_ms = KEY_OFF_UPDATE_PERIOD_ONE_SECOND_ms; //if over 1800 ms, LTC ICs will turn off (bad)
  } 
  else { keyOffUpdatePeriod_ms = KEY_OFF_UPDATE_PERIOD_TEN_MINUTES_ms; }
  
  //Has enough time passed yet?
  if( (millis() - timestamp_lastUpdate_ms) > keyOffUpdatePeriod_ms )
  { 
    isItTimeToUpdate = true;
    timestamp_lastUpdate_ms = millis();
  }

  return isItTimeToUpdate;
}

////////////////////////////////////////////////////////////////////////////////////

bool time_hasKeyBeenOffLongEnough_toTurnOffLiBCM(void)
{
  bool keyOffForLongEnough = false;

  if( (uint32_t)(millis() - key_latestTurnOffTime_ms_get() ) > (KEYOFF_DELAY_LIBCM_TURNOFF_MINUTES * 60000) )
  {
    keyOffForLongEnough = true;
  }

  return keyOffForLongEnough;
}

////////////////////////////////////////////////////////////////////////////////////

void time_waitForLoopPeriod(void)
{
    static uint32_t timestamp_previousLoopStart_ms = millis();
    bool timingMet = false;

    LED(4,HIGH); //LED4 brightness proportional to how much CPU time is left
    while( (millis() - timestamp_previousLoopStart_ms) < time_loopPeriod_ms_get() ) { timingMet = true; } //wait here to start next loop
    LED(4,LOW);
    
    if( (key_getSampledState() == KEYSTATE_ON) && (timingMet == false) )
    {
      Serial.print('*');
      EEPROM_hasLibcmFailedTiming_set(EEPROM_LIBCM_LOOPPERIOD_EXCEEDED);
    }

    timestamp_previousLoopStart_ms = millis(); //placed at end to prevent delay at keyON event
}

////////////////////////////////////////////////////////////////////////////////////

//calculate delta between start and stop time
//store start time: START_TIMER
//calculate delta:  STOP_TIMER
void time_stopwatch(bool timerAction)
{
  static uint32_t startTime = 0;

  if(timerAction == START_TIMER) { startTime = millis(); }
  else
  {
      uint32_t stopTime = millis();
      Serial.print(F("\nDelta: "));
      Serial.print(stopTime - startTime);
      Serial.print(F(" ms\n"));
  }
}

////////////////////////////////////////////////////////////////////////////////////

//only works from 1 to 255 Hz
uint16_t time_hertz_to_milliseconds(uint8_t hertz)
{
  uint16_t milliseconds = 0;

  if( (hertz != 0) ) { milliseconds = 1000 / hertz; } //division

  return milliseconds;
}
