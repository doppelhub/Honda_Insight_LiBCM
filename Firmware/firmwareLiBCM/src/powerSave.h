//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef powerSave_h
    #define powerSave_h
    
    #define PERIOD_TO_DISABLE_POWERSAVE_AFTER_USB_DATA_RECEIVED_ms 60000

    //     UNKNOWN_INTERRUPT 0 //specifically not defined //see explanation in 'ISR(PCINT1_vect)'
    #define    USB_INTERRUPT 1
    #define  KEYON_INTERRUPT 2
    #define TIMER2_INTERRUPT 3

    void powerSave_handler(void);

    void powerSave_init(void);

    void powerSave_sleepIfAllowed(void);

#endif
