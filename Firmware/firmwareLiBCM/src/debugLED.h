//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef debugLED_h
    #define debugLED_h

    void debugLED(uint8_t LED_number, bool illuminated);

    void LED(uint8_t LED_number, bool illuminated);

    void LED_heartbeat(void);

    void LED_turnAllOff(void);
#endif
