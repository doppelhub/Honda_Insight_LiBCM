//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//cpu_map.h - CPU and pin mapping configuration file
//central pin mapping selection file for different processor types

#ifndef cpu_map_h
    #define cpu_map_h

    #define CPU_MAP_MEGA2560

    #ifdef CPU_MAP_MEGA2560
    
        #define PIN_BATTCURRENT A0
        #define PIN_USER_SW     A1
        #define PIN_VPIN_IN     A2
        #define PIN_TEMP_YEL    A3
        #define PIN_TEMP_GRN    A4
        #define PIN_TEMP_WHT    A5
        #define PIN_TEMP_BLU    A6
        #define PIN_FANOEM_LOW  A7
        #define PIN_FANOEM_HI   A8
        #define PIN_TEMP_BAY1   A9
        #define PIN_TEMP_BAY2  A10
        #define PIN_TEMP_BAY3  A11
        #define PIN_LED1       A12
        #define PIN_LED2       A13
        #define PIN_GPIO1      A14 //with daughterboard: heater (if installed) //without daughterboard: not used
        #define PIN_GPIO2      A15

        #define PIN_METSCI_DE       2
        #define PIN_METSCI_REn      3
        #define PIN_TURNOFFLiBCM    4
        #define PIN_VPIN_OUT_PWM    5
        #define PIN_SENSOR_EN       6
        #define PIN_MCME_PWM        7
        #define PIN_GRID_PWM        8
        #define PIN_GRID_SENSE      9
        #define PIN_GRID_EN        10
        #define PIN_FAN_PWM        11
        #define PIN_HMI_EN         12
        #define PIN_IGNITION_SENSE 13
        #define PIN_HW_VER1        38
        #define PIN_HW_VER0        39
        #define PIN_BATTSCI_REn    40
        #define PIN_BATTSCI_DE     41
        #define PIN_COVER_SWITCH   42
        #define PIN_GPIO0_CS_MIMA  43
        #define PIN_GPIO3          44 //with daughterboard: 1500W charger current (if installed) //without daughterboard: heater (if installed) 
        #define PIN_BUZZER_PWM     45
        #define PIN_LED3           46
        #define PIN_SPI_EXT_CS     47
        #define PIN_LED4           48
        #define PIN_TEMP_EN        49

        #define PIN_SPI_CS SS

        //Serial3
        #define METSCI_TX 14
        #define METSCI_RX 15

        //Serial2
        #define BATTSCI_TX 16
        #define BATTSCI_RX 17

        //Serial1
        #define HMI_TX 18
        #define HMI_RX 19

        #define DEBUG_SDA 20
        #define DEBUG_CLK 21

        //1500 watt charger controlled by daughterboard, which uses different pinout
        #ifdef GRIDCHARGER_IS_1500W
            #ifdef BATTERY_TYPE_5AhG3
                #error (invalid grid charger selection in config.h: 5AhG3 LiBCM kits dont support 1500 watt charging)
            #endif
            #define PIN_ABSTRACTED_GRID_CURRENT PIN_GPIO3
            #define PIN_ABSTRACTED_GRID_EN      PIN_GRID_PWM
            #define PIN_ABSTRACTED_GRID_VOLTAGE PIN_GPIO2
        #elif defined GRIDCHARGER_IS_NOT_1500W
            #define PIN_ABSTRACTED_GRID_CURRENT PIN_GRID_PWM
            #define PIN_ABSTRACTED_GRID_EN      PIN_GRID_EN
            //these chargers don't support voltage control
        #else
            #error (Grid charger type not specified in config.h)
        #endif

    #endif

#endif
