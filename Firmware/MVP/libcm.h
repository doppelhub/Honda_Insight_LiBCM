//main LiBCM include file

#ifndef libcm_h
  #define libcm_h



  //define standard libraries used by LiBCM
  #include <Arduino.h>
  //#include <stdint.h>
  //#include <SPI.h>
  //#include <Wire.h>

  //Define LiBCM system include files.  Note: Do not alter order.
  #include "config.h"
  #include "cpu_map.h"
  #include "debugLED.h"
  #include "debugUSB.h"
  #include "LT_SPI.h"
  #include "LTC68042.h"
  #include "battsci.h"
  #include "metsci.h"
  #include "adc.h"
  #include "vPackSpoof.h"
  #include "lcd.h"

#endif