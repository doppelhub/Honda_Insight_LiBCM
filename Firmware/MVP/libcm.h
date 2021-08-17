//main LiBCM include file

#ifndef libcm_h
  #define libcm_h

  #define FW_VERSION "0.0.14"
  #define BUILD_DATE "2021AUG13"
  #define HW_REVB

  //define standard libraries used by LiBCM
  #include <Arduino.h>
  //#include <stdint.h>
  //#include <SPI.h>
  //#include <Wire.h>

  //Define LiBCM system include files.  Note: Do not alter order.
  #include "config.h"
  #include "cpu_map.h"
  #include "LT_SPI.h"
  #include "LTC68042.h"
  #include "battsci.h"
  #include "metsci.h"

  #ifdef I2C_LIQUID_CRYSTAL
    #include "LiquidCrystal_I2C.h"
  #endif
  #ifdef I2C_TWI
    #include "TwiLiquidCrystal.h"
  #endif

#endif