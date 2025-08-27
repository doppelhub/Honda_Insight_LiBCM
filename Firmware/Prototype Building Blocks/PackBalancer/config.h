//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"  //For Arduino IDE compatibility

	#define FW_VERSION "0.1.1"
    #define BUILD_DATE "2024SEP29"

	#define LCD_4X20_CONNECTED  //Comment to disable all 4x20 LCD commands

	//#define BATTERY_TYPE_5AhG3
	#define BATTERY_TYPE_47AhFoMoCo

	//choose which functions control the LEDs
		#define LED_NORMAL //enable "LED()" functions (see debug.c)
		//#define LED_DEBUG //enable "debugLED()" functions (FYI: blinkLED functions won't work)

	#define DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS 1000 //JTS2doNow: Model after "debugUSB_printLatestData_keyOn"

	#define STACK_SoC_MAX 85 //maximum state of charge before regen  is disabled
	#define STACK_SoC_MIN 10 //minimum state of charge before assist is disabled

	#define CELL_VMAX_REGEN                     42000 //42000 = 4.2000 volts
	#define CELL_VMIN_ASSIST                    31900 //allows for ESR-based voltage drop
	#define CELL_VMAX_GRIDCHARGER               39000 //3.9 volts is 75% SoC //other values: See SoC.cpp //MUST be less than 'CELL_VREST_85_PERCENT_SoC'
	#define CELL_VMIN_GRIDCHARGER               30000 //grid charger will not charge severely empty cells
	#define CELL_VMIN_KEYOFF                    CELL_VREST_10_PERCENT_SoC //when car is off, LiBCM turns off below this voltage  //JTS2doLater: Change to higher SoC
	#define CELL_BALANCE_MIN_SoC                65    //when car is off, cell balancing is disabled below this percentage
	#define CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE 15    //'32' = 3.2 mV //CANNOT exceed 255 counts (25.5 mV)
	#define CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT 9    //'22' = 2.2 mV //LTC6804 total measurement error is 2.2 mV //MUST be less than CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE
	#define CELL_BALANCE_MAX_TEMP_C             40

#endif
