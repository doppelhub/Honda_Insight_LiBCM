//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"  //For Arduino IDE compatibility

	#define ENABLE_CURRENT_HACK true // true for +40% hack false for stock
	#define CPU_MAP_MEGA2560
	
	//Choose which I2C driver to use:
	#define I2C_LIQUID_CRYSTAL //use LiquidCrystal_I2C library for 4x20
	//#define I2C_TWI //use TwiLiquidCrystal library for 4x20


#endif