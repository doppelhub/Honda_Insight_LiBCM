//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"  //For Arduino IDE compatibility

	#define FW_VERSION "0.9.0c"
    #define BUILD_DATE "2023APR21"


	//choose your battery type:
		#define BATTERY_TYPE_5AhG3 //previously (incorrectly) referred to as "EHW5"
		//#define BATTERY_TYPE_47AhFoMoCo

	//choose how many cells in series
		#define STACK_IS_48S
		//#define STACK_IS_60S

	//choose ONE of the following:
		//#define SET_CURRENT_HACK_00 //OEM configuration (no current hack installed inside MCM)
		//#define SET_CURRENT_HACK_20 //+20%
		#define SET_CURRENT_HACK_40 //+40%
		//#define SET_CURRENT_HACK_60 //+60% //Note: LiBCM can only measure between 71 A regen & 147 A assist //higher current values will (safely) rail the ADC

	//choose ONE of the following:
		//#define VOLTAGE_SPOOFING_DISABLE              //spoof maximum possible pack voltage at all times //closest to OEM behavior
		#define VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE   //increase assist power by variably   spoofing pack voltage during assist
		//#define VOLTAGE_SPOOFING_ASSIST_ONLY_BINARY   //increase assist power by statically spoofing pack voltage during heavy assist
		//#define VOLTAGE_SPOOFING_ASSIST_AND_REGEN     //increase assist and regen power by variably spoofing pack voltage //DEPRECATED (regen too strong)

	#define LCD_4X20_CONNECTED  //Comment to disable all 4x20 LCD commands
	//#define LIDISPLAY_CONNECTED  //Comment to disable all LiDisplay commands //JTS2doNow: mudder has not yet tested this code. Use at your own risk.

	#define STACK_SoC_MAX 85 //maximum state of charge before regen  is disabled
	#define STACK_SoC_MIN 10 //minimum state of charge before assist is disabled

	#define CELL_VMAX_REGEN                     42500 //42500 = 4.2500 volts
	#define CELL_VMIN_ASSIST                    31900 //allows for ESR-based voltage drop
	#define CELL_VMAX_GRIDCHARGER               39000 //3.9 volts is 75% SoC //other values: See SoC.cpp //MUST be less than 'CELL_VREST_85_PERCENT_SoC'
	#define CELL_VMIN_GRIDCHARGER               30000 //grid charger will not charge severely empty cells
	#define CELL_VMIN_KEYOFF                    CELL_VREST_10_PERCENT_SoC //when car is off, LiBCM turns off below this voltage  //JTS2doLater: Change to higher SoC
	#define CELL_BALANCE_MIN_SoC                65    //when car is off, cell balancing is disabled below this percentage
	#define CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE 32    //'32' = 3.2 mV //CANNOT exceed 255 counts (25.5 mV)
	#define CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT 22    //'22' = 2.2 mV //LTC6804 total measurement error is 2.2 mV //MUST be less than CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE
	#define CELL_BALANCE_MAX_TEMP_C             40
	//#define ONLY_BALANCE_CELLS_WHEN_GRID_CHARGER_PLUGGED_IN //uncomment to disable keyOFF cell balancing (unless the grid charger is plugged in)

	//temp setpoints
	#define COOL_BATTERY_ABOVE_TEMP_C_KEYOFF       36 //cabin air cooling
	#define COOL_BATTERY_ABOVE_TEMP_C_GRIDCHARGING 30
	#define COOL_BATTERY_ABOVE_TEMP_C_KEYON        30
	#define HEAT_BATTERY_BELOW_TEMP_C_KEYON        16 //cabin air heating, or heater PCB (if installed)
	#define HEAT_BATTERY_BELOW_TEMP_C_GRIDCHARGING 16
	#define HEAT_BATTERY_BELOW_TEMP_C_KEYOFF       10
	//other temp settings
	#define KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC 50 //when keyOFF (unless grid charger plugged in) //set to 100 to disable when keyOFF
	#define OEM_FAN_INSTALLED //comment if OEM fan removed

	#define LTC68042_ENABLE_C19_VOLTAGE_CORRECTION //uncomment if using stock Honda 5AhG3 lithium modules

	#define KEYOFF_DELAY_LIBCM_TURNOFF_MINUTES 10 //Even with low SoC, LiBCM will remain on for this many minutes after keyOFF.
		//to turn LiBCM back on: turn ignition 'ON', or turn IMA switch off and on, or plug in USB cable

	//////////////////////////////////////////////////////////////////

	//All remaining settings are for debug testing only:

	//#define RUN_BRINGUP_TESTER //requires external test PCB (that you don't have)

	#define CHECK_FOR_SAFETY_COVER //comment if testing LiBCM without the cover

	#define DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS 1000 //JTS2doLater: Model after "debugUSB_printLatestData"

	//#define DISABLE_ASSIST //uncomment to (always) disable assist
	//#define DISABLE_REGEN  //uncomment to (always) disable regen
	//#define REDUCE_BACKGROUND_REGEN_UNLESS_BRAKING //EXPERIMENTAL! //JTS2doLater: Make this work (for Balto)

	//choose which functions control the LEDs
		#define LED_NORMAL //enable "LED()" functions (see debug.c)
		//#define LED_DEBUG //enable "debugLED()" functions (FYI: blinkLED functions won't work)
#endif

/*
JTS2doLater:
	#define SERIAL_H_LINE_CONNECTED NO //H-Line wire connected to OEM BCM connector pin B01
	#define SERIAL_HMI_CONNECTED NO //Nextion touch screen connected to J14
	#define KEYOFF_TURNOFF_LIBCM_AFTER_HOURS 48 //LiBCM turns off this many hours after keyOFF.
*/
