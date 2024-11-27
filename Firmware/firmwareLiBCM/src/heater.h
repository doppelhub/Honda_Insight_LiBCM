//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef heater_h
    #define heater_h

    #define HEATER_STATE_CHANGE_HYSTERESIS_ms (2 * 1000) //period required before heater can turn on or off

    #define FORCE_HEATER_OFF_ABOVE_TEMP_C 30 //prevent code changes from overheating pack

    #define HEATER_NOT_CONNECTED             0
    #define HEATER_CONNECTED_DIRECT_TO_LIBCM 1 //without daughterboard, heater is attached to GPIO3
    #define HEATER_CONNECTED_DAUGHTERBOARD   2 //with    daughterboard, heater is attached to GPIO1 

    void heater_begin(void);

    uint8_t heater_isConnected(void);
    
    bool heater_isPackTooHot(void);

    void heater_handler(void);

#endif
