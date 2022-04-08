//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//stores various system millisecond timers

#include "libcm.h"

////////////////////////////////////////////////////////////////////////////////////

bool time_toUpdate_keyOffValues(void)
{
  uint32_t keyOffUpdatePeriod_ms = 0;
  static uint32_t timestamp_lastUpdate_ms = 0;
  bool isItTimeToUpdate = false;

  //determine period between LTC cell voltage reads (saves power)
  if( (cellBalance_wereCellsBalanced() == false) || //cells were balanced last time they were measured
      (gpio_isGridChargerChargingNow() == true )  ) //grid charger is actively charging
  { 
    keyOffUpdatePeriod_ms = 1000; //if over 1800 ms, LTC ICs will turn off (bad)
  } 
  else { keyOffUpdatePeriod_ms = 60000; } //JTS2doLater: Change to 10 minutes
  
  //Has enough time passed yet?
  if( (millis() - timestamp_lastUpdate_ms) > keyOffUpdatePeriod_ms )
  { 
    isItTimeToUpdate = true;
    timestamp_lastUpdate_ms = millis();
  }

  return isItTimeToUpdate;
}

////////////////////////////////////////////////////////////////////////////////////

bool time_hasKeyBeenOffLongEnough(void)
{
  bool keyOffForLongEnough = false;

  if( (millis() - key_latestTurnOffTime_ms_get() ) > (KEYOFF_TURNOFF_LIBCM_DELAY_MINUTES * 60000) )
  {
    keyOffForLongEnough = true;
  }

  return keyOffForLongEnough;
}

////////////////////////////////////////////////////////////////////////////////////

void time_waitForLoopPeriod(void)
{
    static uint32_t timestamp_previousLoopStart_ms = millis();
    uint16_t numLoopsWaited = 0;

    LED(4,HIGH); //LED4 brightness proportional to how much CPU time is left
    while( (millis() - timestamp_previousLoopStart_ms ) < LOOP_RATE_MILLISECONDS ) { numLoopsWaited++; } //wait here to start next loop
    LED(4,LOW);
    
    if( (key_getSampledState() == KEYON) && (numLoopsWaited == 0) ) { EEPROM_hasLibcmFailedTiming_set(EEPROM_LIBCM_LOOPRATE_EXCEEDED);}

    timestamp_previousLoopStart_ms = millis(); //placed at end to prevent delay at keyON event
    //JTS2doLater: Determine millis() behavior after overflow (50 days)
}