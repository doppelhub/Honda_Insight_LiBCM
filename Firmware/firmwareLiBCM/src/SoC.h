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

    void SoC_begin(void); //call once battery type is known (i.e. after eeprom_applyBootCriticalConfigOverrides())

    void SoC_handler(void);

    #define VCELL_CRITICALLY_OVERCHARGED   43000  //'43000' = 4.3 V
    #define VCELL_CRITICALLY_DISCHARGED    28000  //'28000' = 2.8 V

    #define CELL_VREST_100_PERCENT_SoC 42000 //same for both battery types

    //these are battery-type dependent (see eepromAccess.h for BATTERY_TYPE_VALUE_5AhG3/_47Ah) -- for maximum life,
    //resting cell voltage should remain between the 010% and 085% SoC values returned by these functions
    uint16_t SoC_cellVrest085PercentSoC_get(void);
    uint16_t SoC_cellVrest010PercentSoC_get(void);
    uint16_t SoC_stackFullCapacity_mAh_get(void); //nominal pack size (0:100% SoC)

#endif
