//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"  //For Arduino IDE compatibility

	#define ENABLE_CURRENT_HACK true // true for +40% hack false for stock
	#define CPU_MAP_MEGA2560

	#define FW_VERSION "x.x.x VPIN"
  	#define BUILD_DATE "2021AUG17"
  	#define HW_REVB
	
	//Choose which I2C LCD driver to use for 4x20 display:
  //#define I2C_LIQUID_CRYSTAL //use "LiquidCrystal_I2C.h"
  //#define I2C_LCD            //use "TwiLiquidCrystal.h"
    #define LCD_JTS            //use "lcd_I2C.h" (modified version ) 

    //#define LCD_4X20_CONNECTED  //Comment to disable all 4x20 LCD commands 
#endif