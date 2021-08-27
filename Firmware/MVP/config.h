//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"  //For Arduino IDE compatibility

	#define ENABLE_CURRENT_HACK // Commented: OEM current sensor behavior //Uncommented: +40% current PCB installed (inside MCM)
	#define CPU_MAP_MEGA2560

	#define FW_VERSION "0.1.1 VPIN"
  #define BUILD_DATE "2021AUG26"
  #define HW_REVB
	
	//Choose which I2C LCD driver to use for 4x20 display:
//#define I2C_LIQUID_CRYSTAL //use "LiquidCrystal_I2C.h"
//#define I2C_LCD            //use "TwiLiquidCrystal.h"
  #define LCD_JTS            //use "lcd_I2C.h" (modified version ) 

  #define LCD_4X20_CONNECTED  //Comment to disable all 4x20 LCD commands
	
  //#define PRINT_ALL_CELL_VOLTAGES_TO_USB //Print all cell voltages to USB Serial Monitor (slow) 

	//choose which functions control the LEDs
//#define LED_NORMAL //enable "     LED()" functions (see debug.c)
  #define LED_DEBUG  //enable "debugLED()" functions (FYI: blinkLED functions won't work)

	#define PRINT_USB_DEBUG_TEXT //prints text sent via debugUSB_debugText() //JTS2doLater: NOT IMPLEMENTED YET

#endif