//JTS2doLater: eeprom.h isn't wrapped into MVP yet

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

    bool time_didLiBCM_justBoot(void);

    #define START_TIMER true
    #define STOP_TIMER false
    #define MILLIS_MAXIMUM_VALUE 0xFFFFFFFF //2^32-1

    #define KEY_OFF_UPDATE_PERIOD_ONE_SECOND_ms  ( 1 *  1000)
    #define KEY_OFF_UPDATE_PERIOD_TEN_MINUTES_ms (10 * 60000)

    #define MILLISECONDS_PER_HOUR 3600000

    #define TIME_DEFAULT_LOOP_PERIOD_ms 10

#endif
