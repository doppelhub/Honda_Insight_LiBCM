//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef gridCharger_h
    #define gridCharger_h

    #define PLUGGED_IN true
    #define UNPLUGGED  false

    #define VCELL_HYSTERESIS 1000 //prevents rapid grid charger on/off toggling when first cell is full

    void gridCharger_handler(void);

    bool gridCharger_getSampledState(void);

    void gridCharger_balanceCells(void);

#endif