//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef temperature_h
    #define temperature_h

    int8_t temperature_battery_getLatest(void);
    int8_t temperature_intake_getLatest(void);
    int8_t temperature_gridCharger_getLatest(void);
  #ifndef BATTERY_TYPE_47AhFoMoCo
    int8_t temperature_exhaust_getLatest(void);
    int8_t temperature_ambient_getLatest(void); //IMA bay temperature
  #endif

    int8_t temperature_measureOneSensor_degC(uint8_t thermistorPin);

    void temperature_measureAndPrintAll(void);
    void temperature_printAll_latest(void);

    int8_t temperature_coolBatteryAbove_C(void);
    int8_t temperature_heatBatteryBelow_C(void);

    void temperature_handler(void);

    #define TEMPERATURE_SENSOR_FAULT_HI         99
    #define TEMPERATURE_SENSOR_FAULT_LO        -99
    #define TEMPERATURE_PACK_IN_THERMAL_RUNAWAY 70

    #define TEMPSENSORSTATE_OFF      1
    #define TEMPSENSORSTATE_TURNON   2
    #define TEMPSENSORSTATE_POWERUP  4
    #define TEMPSENSORSTATE_MEASURE  8
    #define TEMPSENSORSTATE_STAYON  16
    #define TEMPSENSORSTATE_TURNOFF 32

    #define ROOM_TEMP_DEGC     23
    #define TEMP_FREEZING_DEGC  0

  #ifdef BATTERY_TYPE_5AhG3
    #define NUM_BATTERY_TEMP_SENSORS 3
  #elif defined BATTERY_TYPE_47AhFoMoCo
    // what were 2 OEM temp sensors (PIN_TEMP_GRN, PIN_TEMP_YEL) are now on battery modules
    #define NUM_BATTERY_TEMP_SENSORS 5
  #endif

    #define TEMP_POWERUP_DELAY_ms 100

    #define TEMP_UPDATE_PERIOD_KEYON_ms        (1 *  1000) //  1k per second
    #define TEMP_UPDATE_PERIOD_KEYOFF_ms       (1 * 60000) // 60k per minute
    #define TEMP_UPDATE_PERIOD_GRIDCHARGING_ms (2 *  1000) //  1k per second

#endif
