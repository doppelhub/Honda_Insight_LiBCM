//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"  //For Arduino IDE compatibility

	#define FW_VERSION "0.5.2"
    #define BUILD_DATE "2021DEC27"

	#define CPU_MAP_MEGA2560

    //chose ONE of the following:
    	//#define HW_REVB
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

	//#define PRINT_ALL_CELL_VOLTAGES_TO_USB //Uncomment to print all cell voltages while driving //Grid charger always prints all cell voltages

	#define MCME_VOLTAGE_OFFSET_ADJUST 5 //difference between OBDIIC&C and LiBCM spoofed pack voltage (Subtract LiBCM voltage from OBDIIC&C Bvo.  Default is 12.)

	#define LCD_4X20_CONNECTED  //Comment to disable all 4x20 LCD commands

	//choose which functions control the LEDs
	#define LED_NORMAL //enable "     LED()" functions (see debug.c)
	//#define LED_DEBUG //enable "debugLED()" functions (FYI: blinkLED functions won't work)

	#define PRINT_USB_DEBUG_TEXT //prints text sent via debugUSB_debugText() //JTS2doLater: NOT IMPLEMENTED YET
	#define DEBUG_USB_UPDATE_PERIOD_MS 250 //250 = send data every 250 ms

	#define LOOP_RATE_MILLISECONDS 10 // Superloop execution rate: 1/LOOP_RATE_MILLISECONDS (e.g. LOOP_RATE_MILLISECONDS==10 is 100 Hz)

	#define CELL_MAX_ALLOWED_VOLTAGE_REGEN 42000
	#define CELL_MIN_ALLOWED_VOLTAGE_ASSIST 32000

	#define CELL_VMAX_GRIDCHARGER 39500 //39000 = 3.9000 volts
	#define CELL_VMAX_KEYON       42500 //42500 = 4.2500 volts //LiBCM disables regen above this voltage
	#define CELL_VMIN_KEYON       32500 //32500 = 3.2500 volts //LiBCM disables assist below this voltage
	#define CELL_VMIN_KEYOFF      31000 //33800 = 3.1000 volts //LiBCM turns off below this voltage

	#define LTC68042_ENABLE_C19_VOLTAGE_CORRECTION //Uncomment if using 18S Honda EHW5 modules

	#define STACK_mAh 5000 //nominal pack size (0:100% SoC) //LiBCM uses this value until it determines the actual pack capacity
	#define STACK_SoC_MIN 10 //minimum state of charge before assist is disabled
	#define STACK_SoC_MAX 85 //maximum state of charge before regen  is disabled
	#define STACK_SoC_SETPOINT_GRID_CHARGE 70 //target SoC while grid charging

	#define SOC_SEND_MCM_MAX_ASSIST_THRESHOLD 45 // minimum state of charge we are sending to MCM before we are going to spoof +20ºC to always get max assist

	//#define RUN_BRINGUP_TESTER //requires external test PCB (that you don't have)
#endif

/*
Features to add later:

#define PCODE_IF_COVER_NOT_INSTALLED

//Define realtime commands that are immediately picked off from the serial stream.
//These characters are not passed to the serial parser, and are executed immediately.
#define CMD_RESET '|'
#define CMD_STATUS_REPORT

//Define battery parameters
#define STACK_CELLS_IN_SERIES 48
#define STACK_CURRENT_MAX_ASSIST 200 //disable assist above this current
#define STACK_CURRENT_MAX_REGEN 100 //disable regen above this current

//Configure when LiBCM turns off when the key is not on.
//LiBCM will turn off when ANY condition below occurs
#define KEYOFF_TURNOFF_HOURS 4       //LiBCM turns off after this much time, -1 to disable
#define KEYOFF_TURNOFF_BELOW_SoC 50  //LiBCM turns off when SoC drops below this value

//Configure fan behavior when key is off
#define KEYOFF_FANS_ALLOWED YES //'NO' to prevent fan usage when key is off
#define KEYOFF_FANS_MIN_SoC 60 //Fans are disabled below this SoC

//Configure fan temperature setpoints
//All temperatures are in Celsius
#define TEMP_FAN_LOW 30  //enable OEM fan at low speed above this value
#define TEMP_OEMFAN_HIGH 40 //enable OEM fan at high speed above this value
#define TEMP_FAN_MIN 30 //enable onboard fans at lowest speed
#define TEMP_FAN_MAX 40 //enable onboard fans at highest speed
"Allow Cooling When Key OFF"
"When key OFF, Turn LiBCM off below ____ SoC" ("Key OFF fan minimum SoC Level")
#SPOOF_TEMP_FOR_MAX_POWER YES

//Define which parameters are reported over the USB serial bus
#define USB_REPORT_ALLOWED YES	//if disabled, no data is reported
#define USB_REPORT_CELL_VOLTAGE_MAX YES
#define USB_REPORT_CELL_VOLTAGE_MIN YES
#define USB_REPORT_CELL_VOLTAGE_ALL YES
#define USB_REPORT_TEMP_MAX YES
#define USB_REPORT_TEMP_MIN YES
#define USB_REPORT_TEMP_ALL YES
#define USB_REPORT_KEY_STATE YES
#define USB_REPORT_METSCI_DATA YES
#define USB_REPORT_BATTSCI_DATA NO
#define USB_REPORT_CURRENT YES
#define USB_REPORT_PACK_VOLTAGE YES
#define USB_REPORT_PACK_POWER YES
#define USB_REPORT_FAN_STATUS YES
#define USB_REPORT_FANOEM_STATUS YES
#define USB_REPORT_SoC YES

//Grid charger behavior
#define GRID_CHARGE_ALLOWED YES
#define GRID_CHARGE_TEMP_MIN //grid charging disabled below this temperature
#define GRID_CHARGE_TEMP_MAX //grid charging disabled above this temperature
#define GRID_CHARGE_CURRENT_MAX_mA //specifies grid charger's maximum current output in mA

#define SERIAL_H_LINE_CONNECTED NO //H-Line wire connected to OEM BCM connector pin B01
#define SERIAL_I2C_CONNECTED YES //Serial display connected to SDA/SDL lines
#define SERIAL_HMI_CONNECTED NO //Nextion touch screen connected to J14
*/
