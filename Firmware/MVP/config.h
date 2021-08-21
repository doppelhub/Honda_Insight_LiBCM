//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"  //For Arduino IDE compatibility

	#define ENABLE_CURRENT_HACK true // true for +40% hack false for stock
	#define CPU_MAP_MEGA2560

	#define FW_VERSION "x.x.x VPIN BRANCH"
  	#define BUILD_DATE "2021AUG17"
  	#define HW_REVB
	
	//Choose which I2C driver to use:
	#define I2C_LIQUID_CRYSTAL //use LiquidCrystal_I2C library for 4x20
	//#define I2C_TWI //use TwiLiquidCrystal library for 4x20
		//In my testing, both libraries cause "P1648 hang" if SDA is manually pulled to GND.
		//This is a low-level Arduino Wire bug (i.e. while loop waiting for SDA to return high).
		//Workaround: pull SDA/CLK to VCC with 10k, which reduces aggressor noise onto serial bus.

#endif