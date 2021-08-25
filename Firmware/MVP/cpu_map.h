//cpu_map.h - CPU and pin mapping configuration file
//central pin mapping selection file for different processor types

#ifndef cpu_map_h
#define cpu_map_h

  #ifdef CPU_MAP_MEGA2560
  
    #ifdef HW_REVB
      #define PIN_BATTCURRENT A0
      #define PIN_FANOEM_LOW A1
      #define PIN_FANOEM_HI A2
      #define PIN_TEMP_YEL A3
      #define PIN_TEMP_GRN A4
      #define PIN_TEMP_WHT A5
      #define PIN_TEMP_BLU A6
      #define PIN_VPIN_IN A7
      #define PIN_TURNOFFLiBCM A8
      #define PIN_HMI_EN A9
      #define PIN_BATTSCI_DE A10
      #define PIN_BATTSCI_REn A11
      #define PIN_LED1 A12
      #define PIN_LED2 A13
      #define PIN_LED3 A14
      #define PIN_LED4 A15

      #define PIN_METSCI_DE 2
      #define PIN_METSCI_REn 3
      #define PIN_VPIN_OUT_PWM 4
      #define PIN_SPI_EXT_CS 5
      #define PIN_TEMP_EN 6
      #define PIN_MCME_PWM 7
      #define PIN_GRID_PWM 8
      #define PIN_GRID_SENSE 9
      #define PIN_GRID_EN 10
      #define PIN_FAN_PWM 11
      #define PIN_I_SENSOR_EN 12
      #define PIN_KEY_ON 13
      #define PIN_SPI_CS SS
      #define PIN_GPIO1 48
      #define PIN_USER_SW 49

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

      //JTS2doLater: Replace Arduino I/O functions as shown in example code below
      /*
        #define DIRECTION_DDR     DDRD
        #define DIRECTION_PORT    PORTD
        #define X_DIRECTION_BIT   5  // Uno Digital Pin 5
        #define Y_DIRECTION_BIT   6  // Uno Digital Pin 6
        #define Z_DIRECTION_BIT   7  // Uno Digital Pin 7
        #define DIRECTION_MASK    ((1<<X_DIRECTION_BIT)|(1<<Y_DIRECTION_BIT)|(1<<Z_DIRECTION_BIT)) // All direction bits
      */

    #endif

  #endif

#endif
