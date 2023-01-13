//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//cpu_map.h - CPU and pin mapping configuration file
//central pin mapping selection file for different processor types

#ifndef cpu_map_h
#define cpu_map_h

  #define CPU_MAP_MEGA2560

  #ifdef CPU_MAP_MEGA2560
  
      #define PIN_BATTCURRENT A0
      #define PIN_USER_SW A1
      #define PIN_VPIN_IN A2
      #define PIN_TEMP_YEL A3
      #define PIN_TEMP_GRN A4
      #define PIN_TEMP_WHT A5
      #define PIN_TEMP_BLU A6
      #define PIN_FANOEM_LOW A7
      #define PIN_FANOEM_HI A8
      #define PIN_TEMP_BAY1 A9
      #define PIN_TEMP_BAY2 A10
      #define PIN_TEMP_BAY3 A11
      #define PIN_LED1 A12
      #define PIN_LED2 A13
      #define PIN_GPIO1 A14
      #define PIN_GPIO2 A15

      #define PIN_METSCI_DE 2
      #define PIN_METSCI_REn 3
      #define PIN_TURNOFFLiBCM 4
      #define PIN_VPIN_OUT_PWM 5
      #define PIN_SENSOR_EN 6
      #define PIN_MCME_PWM 7
      #define PIN_GRID_PWM 8
      #define PIN_GRID_SENSE 9
      #define PIN_GRID_EN 10
      #define PIN_FAN_PWM 11
      #define PIN_HMI_EN 12
      #define PIN_IGNITION_SENSE 13
      #define PIN_HW_VER1 38
      #define PIN_HW_VER0 39
      #define PIN_BATTSCI_REn 40
      #define PIN_BATTSCI_DE 41
      #define PIN_COVER_SWITCH 42
      #define PIN_GPIO0 43
      #define PIN_GPIO3 44
      #define PIN_BUZZER_PWM 45
      #define PIN_LED3 46
      #define PIN_SPI_EXT_CS 47
      #define PIN_LED4 48
      #define PIN_TEMP_EN 49

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

  #endif
      
    #define HW_REVB_PROTOTYPES  

    #ifdef HW_REVB_PROTOTYPES
      //RevB hardware had different pinout.
      //RevC+ have identical pinouts.

      //redefine RevB hardware pinout
      #undef  PIN_USER_SW
      #define PIN_USER_SW      49 //RevC+:  A1
      
      #undef  PIN_VPIN_IN
      #define PIN_VPIN_IN      A7 //RevC+:  A2
      
      #undef  PIN_FANOEM_LOW
      #define PIN_FANOEM_LOW   A1 //RevC+:  A7
      
      #undef  PIN_FANOEM_HI
      #define PIN_FANOEM_HI    A2 //RevC+:  A8
      
      #undef  PIN_TEMP_BAY1 //REVB didn't have battery temp sensors
      #define PIN_TEMP_BAY1 36 //This pin isn't connected to anything
      
      #undef  PIN_TEMP_BAY2 //REVB didn't have battery temp sensors
      #define PIN_TEMP_BAY1 36 //This pin isn't connected to anything
      
      #undef  PIN_TEMP_BAY3 //REVB didn't have battery temp sensors
      #define PIN_TEMP_BAY1 36 //This pin isn't connected to anything

      #undef  PIN_GPIO1
      #define PIN_GPIO1        48 //RevC+: A14
      
      #undef  PIN_GPIO2 //REVB didn't have PIN_GPIO2
      #define PIN_GPIO2 37 //This pin isn't connected to anything

      #undef  PIN_TURNOFFLiBCM
      #define PIN_TURNOFFLiBCM A8 //RevC+:  4

      #undef  PIN_VPIN_OUT_PWM
      #define PIN_VPIN_OUT_PWM  4 //RevC+:  5

      #undef  PIN_SENSOR_EN
      #define PIN_SENSOR_EN    12 //RevC+:  6

      #undef  PIN_HMI_EN
      #define PIN_HMI_EN       A9 //RevC+: 12

      #undef  PIN_BATTSCI_REn
      #define PIN_BATTSCI_REn A11 //RevC+: 40

      #undef  PIN_BATTSCI_DE
      #define PIN_BATTSCI_DE  A10 //RevC+: 41

      #undef  PIN_LED3
      #define PIN_LED3        A14 //RevC+: 46

      #undef  PIN_LED4
      #define PIN_LED4        A15 //RevC+: 48

      #undef  PIN_SPI_EXT_CS
      #define PIN_SPI_EXT_CS    5 //RevC+: 47

      #undef  PIN_TEMP_EN
      #define PIN_TEMP_EN       6 //RevC+: 49  
  
   #endif

#endif