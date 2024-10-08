#ifndef time_h
    #define time_h

    void time_handler(void);

    bool time_isItTimeToPerformKeyOffTasks(void);

    bool time_hasKeyBeenOffLongEnough_toTurnOffLiBCM(void);

    void time_waitForLoopPeriod(void);

    uint8_t time_getLoopCount_8b(void); //frequent tasks can schedule using this instead of 32b millis()

    void time_stopwatch(bool timerAction);

    uint16_t time_hertz_to_milliseconds(uint8_t hertz);

    void    time_loopPeriod_ms_set(uint8_t period_ms);
    uint8_t time_loopPeriod_ms_get(void);

    void     time_latestKeyOn_ms_set(uint32_t keyOnTime);
    uint32_t time_latestKeyOn_ms_get(void);

    void     time_latestKeyOff_ms_set(uint32_t keyOffTime);
    uint32_t time_latestKeyOff_ms_get(void);

    uint32_t time_sinceLatestKeyOn_ms(void);
    uint16_t time_sinceLatestKeyOn_seconds(void);

    void          time_latestUserInputUSB_set(void);
    uint32_t time_sinceLatestUserInputUSB_get_ms(void);

    void          time_latestGridChargerUnplug_set(void);
    uint32_t time_sinceLatestGridChargerUnplug_get_ms(void);

    void time_addSleepPeriodToMillis(void);
    void time_setAbsoluteMillis(uint32_t newMillis);

    bool time_didLiBCM_justBoot(void);

    #define START_TIMER true
    #define STOP_TIMER false
    #define MILLIS_MAXIMUM_VALUE 0xFFFFFFFF //2^32-1

    #define KEY_OFF_UPDATE_PERIOD_ONE_SECOND_ms  ( 1 *  1000)
    #define KEY_OFF_UPDATE_PERIOD_TEN_MINUTES_ms (10 * 60000)
    //JTS2doLater: When the key is on - but the IMA switch is off (or fuse blown) - reduce LTC6804 update period to minimize LTC6804 power consumption
    //Tests to determine whether the IMA switch is on or off:
        //1) compare the VPIN input signal (HVDC voltage inside the PDU) with the pack voltage (as determined by the LTC6804 ICs).
        //1) After keyOn, if the delta varies wildly over a given time period, LiBCM can conclude that the IMA switch is off.
        //2) LiBCM can reset a timer each time the battery current sensor magnitude exceeds 0 amps (which can only happen if the IMA switch is on).
        //2) If the timer overflows (e.g. after an hour with 0 amps through the pack), LiBCM can likely conclude that the IMA switch is off.

    #define MILLISECONDS_PER_HOUR 3600000

    #define TIME_DEFAULT_LOOP_PERIOD_ms 10

#endif
