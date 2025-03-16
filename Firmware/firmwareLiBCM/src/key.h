//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef key_h
    #define key_h

    #define KEYSTATE_OFF              0
    #define KEYSTATE_ON               1
    #define KEYSTATE_UNINITIALIZED    2
    #define KEYSTATE_OFF_JUSTOCCURRED 3

    void key_stateChangeHandler(void);

    uint8_t key_getSampledState(void);

    void keyOn_coldBootTasks(void);

#endif
