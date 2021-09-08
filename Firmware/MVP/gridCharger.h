//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef gridCharger_h
    #define gridCharger_h

    #define PLUGGED_IN true
    #define UNPLUGGED  false

    #define ONE_MILLIVOLT_COUNTS 10

    #define MIN_DISCHARGE_VOLTAGE_COUNTS 36000

    #define VCELL_HYSTERESIS 1000 // '1000' = 100 mV prevents rapid grid charger on/off toggling when first cell is full

    void gridCharger_handler(void);

#endif