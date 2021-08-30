//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//main LiBCM include file

#ifndef libcm_h
  #define libcm_h
  
  #define KEYON  1
  #define KEYOFF 0

  #define NO  0
  #define YES 1

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
  #include "battsci.h"
  #include "metsci.h"
  #include "adc.h"
  #include "vPackSpoof.h"
  #include "lcd.h"
  #include "LTC68042configure.h"
  #include "LTC68042cell.h"
  #include "LTC68042gpio.h"
  #include "LTC68042result.h"

#endif