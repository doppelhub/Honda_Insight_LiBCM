//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef soc_h
    #define soc_h

    void SoC_integrateCharge_adcCounts(int16_t adcCounts);

    uint16_t SoC_getBatteryStateNow_mAh(void);
    void     SoC_setBatteryStateNow_mAh(uint16_t newPackCharge_mAh);

    void     SoC_setBatteryStateNow_percent(uint8_t newSoC);
    uint8_t  SoC_getBatteryStateNow_percent(void);
    uint16_t SoC_getBatteryStateNow_deciPercent(void); //JTS2doLater: Use this in Battsci, etc.

    uint8_t SoC_estimateFromRestingCellVoltage_percent(void);

    void SoC_updateUsingLatestOpenCircuitVoltage(void);

    void SoC_handler(void);

    #define VCELL_CRITICALLY_OVERCHARGED   43000  //'43000' = 4.3 V
    #define VCELL_CRITICALLY_DISCHARGED    28000  //'28000' = 2.8 V

    #ifdef BATTERY_TYPE_5AhG3
        #define CELL_VREST_100_PERCENT_SoC 42000
        #define CELL_VREST_085_PERCENT_SoC 40000 //for maximum life, resting cell voltage should remain below 85% SoC
        #define CELL_VREST_010_PERCENT_SoC 34200 //for maximum life, resting cell voltage should remain above 10% SoC
        #define STACK_mAh_NOM 5000 //5 Ah nominal //nominal pack size (0:100% SoC)
    #elif defined BATTERY_TYPE_47Ah
        #define CELL_VREST_100_PERCENT_SoC 42000
        #define CELL_VREST_085_PERCENT_SoC 39700
        #define CELL_VREST_010_PERCENT_SoC 34000
        #define STACK_mAh_NOM 47000
    #else
        #error (Battery type not specified in config.h)
    #endif

#endif
