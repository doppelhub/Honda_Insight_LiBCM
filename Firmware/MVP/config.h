//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"  //For Arduino IDE compatibility

	#define FW_VERSION "0.7.2"
    #define BUILD_DATE "2022MAR17"

	#define CPU_MAP_MEGA2560
    #define HW_REVC

	//choose ONE of the following:
		//#define SET_CURRENT_HACK_00 //OEM configuration (no current hack installed inside MCM)
		//#define SET_CURRENT_HACK_20 //+20%
		#define SET_CURRENT_HACK_40 //+40%
		//#define SET_CURRENT_HACK_60 //+60% //Note: LiBCM can only measure between 71 A regen & 147 A assist //higher current values will (safely) rail the ADC

	//choose ONE of the following:
		//#define VOLTAGE_SPOOFING_DISABLE              //closest to OEM IMA behavior
		//#define VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE //only spoof during assist, using variable voltage
		//#define VOLTAGE_SPOOFING_ASSIST_ONLY_BINARY   //only spoof during assist, using either 120 volts or (vPackActual-12)
		#define VOLTAGE_SPOOFING_ASSIST_AND_REGEN     //always spoof voltage (enables stronger regen)

	//#define DISABLE_ASSIST //uncomment to (always) disable assist
	//#define DISABLE_REGEN  //uncomment to (always) disable regen
	//#define REDUCE_BACKGROUND_REGEN_UNLESS_BRAKING //EXPERIMENTAL! //JTS2doNow: Make it better

	//#define PRINT_ALL_CELL_VOLTAGES_TO_USB //Uncomment to print all cell voltages while driving //Grid charger always prints all cell voltages

	#define MCME_VOLTAGE_OFFSET_ADJUST 12 //difference between OBDIIC&C and LiBCM spoofed pack voltage (Subtract LiBCM voltage from OBDIIC&C Bvo.  Default is 12.)

	#define LCD_4X20_CONNECTED  //Comment to disable all 4x20 LCD commands

	//choose which functions control the LEDs
		#define LED_NORMAL //enable "LED()" functions (see debug.c)
		//#define LED_DEBUG //enable "debugLED()" functions (FYI: blinkLED functions won't work)

	#define DEBUG_USB_UPDATE_PERIOD_KEYON_mS 250 //250 = send data every 250 ms
	#define DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS 1000

	#define LOOP_RATE_MILLISECONDS 10 // Superloop execution rate: 1/LOOP_RATE_MILLISECONDS //'10' = 100 Hz

	#define STACK_mAh_NOM 5000 //nominal pack size (0:100% SoC) //LiBCM uses this value until it determines the actual pack capacity
	#define STACK_SoC_MAX 85 //maximum state of charge before regen  is disabled
	#define STACK_SoC_MIN 10 //minimum state of charge before assist is disabled
	#define CELL_VREST_85_PERCENT_SoC 40100 //for maximum life, resting cell voltage should remain below 85% SoC
	#define CELL_VREST_10_PERCENT_SoC 34700 //for maximum life, resting cell voltage should remain above 10% SoC

	#define CELL_VMAX_REGEN                     42000 //42000 = 4.2000 volts
	#define CELL_VMIN_ASSIST                    31900 //allows for ESR-based voltage drop
	#define CELL_VMAX_GRIDCHARGER               39000 //3.9 volts is 75% SoC //other values: See SoC.cpp //MUST be less than 'CELL_VREST_85_PERCENT_SoC'
	#define CELL_VMIN_GRIDCHARGER               30000 //grid charger will not charge severely empty cells
	#define CELL_VMIN_KEYOFF                    CELL_VREST_10_PERCENT_SoC //when car is off, LiBCM turns off below this voltage  //JTS2doLater: Change to higher SoC
	#define CELL_BALANCE_MIN_SoC                40    //when car is off, cell balancing is disabled below this percentage
	#define CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE 20    //'20' = 2.0 mV //CANNOT exceed 255 counts (25.5 mV)
	#define CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT 5     // '5' = 0.5 mV //MUST be less than CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE

	//#define ONLY_BALANCE_CELLS_WHEN_GRID_CHARGER_PLUGGED_IN //uncomment to prevent keyOFF cell balancing, unless the grid charger is plugged in  

	#define LTC68042_ENABLE_C19_VOLTAGE_CORRECTION //uncomment if using stock Honda EHW5 lithium modules

	//#define KEYOFF_TURNOFF_LIBCM_AFTER_HOURS 48 //LiBCM turns off this many hours after keyOFF. //JTS2doLater: Not implemented yet.
	#define KEYOFF_TURNOFF_LIBCM_DELAY_MINUTES 10 //Even with low SoC, LiBCM will remain on for this many minutes after keyOFF.
	//to turn LiBCM back on: turn ignition to 'ON', or turn IMA switch off and on, or plug in USB cable

	#define PREVENT_BOOT_WITHOUT_SAFETY_COVER //comment if testing LiBCM without the cover

	//#define RUN_BRINGUP_TESTER //requires external test PCB (that you don't have)
#endif

/*
Features to add later:

#define DISPLAY_OEM_CURRENT_SIGN //JTS2doNow: add feature

//Define stack parameters
#define STACK_CELLS_IN_SERIES 48

//Configure fan behavior when key is off
#define KEYOFF_FAN_COOLING_ALLOWED YES //'NO' to prevent fan usage when key is off
#define KEYOFF_FAN_COOLING_MIN_SoC 60 //Fans are disabled below this SoC

//Configure fan temperature setpoints
//All temperatures are in Celsius
#define TEMP_FAN_LOW 30  //enable OEM fan at low speed above this value
#define TEMP_OEMFAN_HIGH 40 //enable OEM fan at high speed above this value
#define TEMP_FAN_MIN 30 //enable onboard fans at lowest speed
#define TEMP_FAN_MAX 40 //enable onboard fans at highest speed

#define SERIAL_H_LINE_CONNECTED NO //H-Line wire connected to OEM BCM connector pin B01
#define SERIAL_I2C_CONNECTED YES //Serial display connected to SDA/SDL lines
#define SERIAL_HMI_CONNECTED NO //Nextion touch screen connected to J14
*/