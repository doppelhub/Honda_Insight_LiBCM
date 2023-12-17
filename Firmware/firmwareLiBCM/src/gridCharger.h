//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef gridCharger_h
    #define gridCharger_h

    #define VCELL_HYSTERESIS 150 // '150' = 15 mV //prevents rapid grid charger on/off toggling when first cell is full

    #define YES__CHARGING_ALLOWED       0b01111111 //if charging allowed,  first three bits are '011'
    #define NO__UNINITIALIZED           0b10000000 //if charging disabled, first three bits are '100' //safety feature in case a single bit flips 
    #define NO__CHARGER_IS_HOT          0b10000001
    #define NO__BATTERY_IS_COLD         0b10000010
    #define NO__BATTERY_IS_HOT          0b10000011
    #define NO__AIRINTAKE_IS_HOT        0b10000100
    #define NO__TEMP_UNPLUGGED_GRID     0b10000101
    #define NO__TEMP_UNPLUGGED_INTAKE   0b10000110
    #define NO__TEMP_EXHAUST_IS_HOT     0b10000111
    #define NO__ATLEASTONECELL_TOO_HIGH 0b10001000
    #define NO__ATLEASTONECELL_FULL     0b10001001
    #define NO__ATLEASTONECELL_TOO_LOW  0b10001010
    #define NO__JUSTPLUGGEDIN           0b10001011
    #define NO__LIBCMJUSTBOOTED         0b10001100
    #define NO__RECENTLY_TURNED_OFF     0b10001101
    #define NO__KEY_IS_ON               0b10001110
    #define NO__CELL_VOLTAGE_HYSTERESIS 0b10001111
    #define NO__CHARGER_UNPLUGGED       0b10010000

    #define DISABLE_GRIDCHARGING_ABOVE_CHARGER_TEMP_C 60
    #define DISABLE_GRIDCHARGING_BELOW_BATTERY_TEMP_C (TEMP_FREEZING_DEGC + 2)
    #define DISABLE_GRIDCHARGING_ABOVE_BATTERY_TEMP_C 40
    #define DISABLE_GRIDCHARGING_ABOVE_INTAKE_TEMP_C  45
    #define DISABLE_GRIDCHARGING_ABOVE_EXHAUST_TEMP_C 50

    #define GRID_CHARGING_FANS_OFF_BELOW_TEMP_C 20
    #define GRID_CHARGING_FANS_LOW_BELOW_TEMP_C 30 //fans are high above this temp

    #define DISABLE_GRIDCHARGING_LIBCM_BOOT_DELAY_ms  2500 //prevents grid charging at poweron, so that the grid charger will never turn on if the watchdog continuously resets
    #define DISABLE_GRIDCHARGING_PLUGIN_DELAY_ms 1000 //prevent current inrush while plugging in grid charger

    #define GRID_MIN_OFF_PERIOD__NONE_ms      0
    #define GRID_MIN_OFF_PERIOD__SHORT_ms  2000 //prevents rapid grid charger cycling
    #define GRID_MIN_OFF_PERIOD__LONG_ms  30000

    void gridCharger_handler(void);

#endif
