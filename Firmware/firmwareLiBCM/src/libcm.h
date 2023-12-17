//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//main LiBCM include file

#ifndef libcm_h
    #define libcm_h

    #define NO  false
    #define YES true

    #define OFF false
    #define ON  true

    //define standard libraries used by LiBCM
    #include <Arduino.h>
    #include <avr/wdt.h>

    //Define LiBCM system include files.  Note: Do not alter order.
    #include "../config.h"
    #include "cpu_map.h"
    #include "debugLED.h"
    #include "debugUSB.h"
    #include "gpio.h"
    #include "key.h"
    #include "LT_SPI.h"
    #include "battsci.h"
    #include "metsci.h"
    #include "LiDisplay.h"
    #include "adc.h"
    #include "vPackSpoof.h"
    #include "lcdState.h"
    #include "lcdTransmit.h"
    #include "gridCharger.h"
    #include "LTC68042configure.h"
    #include "LTC68042cell.h"
    #include "LTC68042gpio.h"
    #include "LTC68042result.h"
    #include "BringupTester.h"
    #include "SoC.h"
    #include "temperature.h"
    #include "eepromAccess.h"
    #include "cellBalance.h"
    #include "time.h"
    #include "USB_userInterface.h"
    #include "fan.h"
    #include "buzzer.h"
    #include "status.h"
    #include "heater.h"
    #include "LiControl.h"
    #include "batteryHistory.h"

#endif
