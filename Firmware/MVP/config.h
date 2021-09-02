//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"  //For Arduino IDE compatibility

	#define FW_VERSION "0.1.5 VPIN"
  #define BUILD_DATE "2021SEP01"
  #define HW_REVB
	#define CPU_MAP_MEGA2560

	//choose ONE of the following:
		//#define SET_CURRENT_HACK_00 //OEM configuration (no current hack installed inside MCM)
		//#define SET_CURRENT_HACK_20 //+20%
		#define SET_CURRENT_HACK_40 //+40%
		//#define SET_CURRENT_HACK_60 //+60%

	#define LCD_4X20_CONNECTED  //Comment to disable all 4x20 LCD commands
	//Choose which I2C LCD driver to use for 4x20 display:
		//#define I2C_LIQUID_CRYSTAL //use "LiquidCrystal_I2C.h"
		//#define I2C_LCD            //use "TwiLiquidCrystal.h"
	  #define LCD_JTS            //use "lcd_I2C.h" (modified version ) 
	
	//#define PRINT_ALL_CELL_VOLTAGES_TO_USB //Print all cell voltages to USB Serial Monitor (slow) 

	//choose which functions control the LEDs
	#define LED_NORMAL //enable "     LED()" functions (see debug.c)
	//#define LED_DEBUG  //enable "debugLED()" functions (FYI: blinkLED functions won't work)

	#define PRINT_USB_DEBUG_TEXT //prints text sent via debugUSB_debugText() //JTS2doLater: NOT IMPLEMENTED YET

	#define LOOP_RATE_MS 10 // Superloop execution rate: 1/LOOP_RATE_MS (e.g. LOOP_RATE_MS==10 is 100 Hz)

	#define GRID_CHARGER_CELL_VMAX 39000 // Vcell = (GRID_CHARGER_CELL_VMAX * 0.0001 V) //cells charged to this voltage

#endif