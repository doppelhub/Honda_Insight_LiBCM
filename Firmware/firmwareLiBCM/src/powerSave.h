//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef powerSave_h
    #define powerSave_h
    
    #define PERIOD_TO_DISABLE_SLEEP_AFTER_USB_DATA_RECEIVED_ms ((uint32_t)60000)

    #define PERIOD_TO_DISABLE_TURNOFF_AFTER_CHARGER_UNPLUGGED_hours 4 //prevents turnoff during brief AC power outage
    #define PERIOD_TO_DISABLE_TURNOFF_AFTER_CHARGER_UNPLUGGED_ms (((uint32_t)1000 * 60 * 60) * PERIOD_TO_DISABLE_TURNOFF_AFTER_CHARGER_UNPLUGGED_hours)

    //     UNKNOWN_INTERRUPT 0 //specifically not defined //see explanation in 'ISR(PCINT1_vect)'
    #define    USB_INTERRUPT 1
    #define  KEYON_INTERRUPT 2
    #define TIMER2_INTERRUPT 3

    void powerSave_init(void);

    void powerSave_turnOffIfAllowed(void);
    void powerSave_sleepIfAllowed(void);

#endif
