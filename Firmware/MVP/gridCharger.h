//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef gridCharger_h
    #define gridCharger_h

    #define PLUGGED_IN true
    #define UNPLUGGED  false

    #define VCELL_HYSTERESIS 200 // '200' = 20 mV //prevents rapid grid charger on/off toggling when first cell is full

    void gridCharger_handler(void);

    #define CELLSTATE_OVERDISCHARGED 4
    #define CELLSTATE_OVERCHARGED    3
    #define CELLSTATE_SOMECELLSFULL  2
    #define CELLSTATE_ZEROCELLSFULL  1
    #define CELLSTATE_UNINITIALIZED  0


#endif