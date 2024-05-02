//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//config.h - compile time configuration parameters

#ifndef config_h
    #define config_h
    #include "src/libcm.h"

    #define FW_VERSION "0.9.4"
    #define BUILD_DATE "2024MAY01"

    //////////////////////////////////////////////////////////////////

    ///////////////////////////////////////
    //                                   //
    //  Hardware Specific Configuration  //
    //                                   //
    ///////////////////////////////////////

    //'choose' exactly one hardware option from each group below, which must match the actual installed hardware.
    //'choose' an option by removing both forward slashes ('//') at the beginning of that line.
    //all other options in each group MUST begin with two forward slashes (i.e. they are not chosen).
    //there are no default options because this firmware works with all LiBCM variants... you need to specify which hardware you have installed

    //choose your battery type:
        //#define BATTERY_TYPE_5AhG3 //if you're not sure, you probably have this battery
        //#define BATTERY_TYPE_47AhFoMoCo

    //choose how many cells are in series:
        //#define STACK_IS_48S //All 5AhG3 Kits & FoMoCo Kits with QTY4 modules
        //#define STACK_IS_60S //FoMoCo Kits with QTY5 modules

    //choose which grid charger is installed
        //#define GRIDCHARGER_IS_NOT_1500W //All 5AhG3 Kits & 'standard' 47Ah FoMoCo Kits
        //#define GRIDCHARGER_IS_1500W //'faster' 47Ah FoMoCo Kits only

    //choose ONE of the following
    //must match actual "current hack" hardware configuration:
        //#define SET_CURRENT_HACK_40 //actually +45.8% //most LiBCM users installed this hardware option
        //#define SET_CURRENT_HACK_20 //actually +25.0%
        //#define SET_CURRENT_HACK_00 //OEM configuration (no current hack installed inside MCM)

    //choose which display to use
    //using both displays simultaneously could cause timing issues (FYI: the Serial Monitor prints '*' each time the loop period is violated)
        #define LCD_4X20_CONNECTED  //display included with all LiBCM Kits
        //#define LIDISPLAY_CONNECTED //optional color touch screen display //JTS2doLater: mudder has not yet tested this code. Use at your own risk.


    //////////////////////////////////////////////////////////////////

    ////////////////////////
    //                    //
    //  Voltage Spoofing  //
    //                    //
    ////////////////////////

    //'choose' exactly one option from each group below.
    //the default values below should work in all cars.
    //modify these parameters if you want more power during heavy assist and/or regen.

    //48S ONLY: choose ONE of the following
    //60S MUST use 'VOLTAGE_SPOOFING_DISABLE':
        #define VOLTAGE_SPOOFING_DISABLE              //spoof maximum possible pack voltage at all times //closest to OEM behavior
        //#define VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE //increase assist power by variably   spoofing pack voltage during assist
        //#define VOLTAGE_SPOOFING_ASSIST_ONLY_BINARY   //increase assist power by statically spoofing pack voltage during heavy assist
        //#define VOLTAGE_SPOOFING_ASSIST_AND_REGEN     //increase assist and regen power by variably spoofing pack voltage //DEPRECATED (regen too strong)
        //#define VOLTAGE_SPOOFING_LINEAR               //increase assist and regen power by requesting peak current level as per OEM (compatible with 100A fuse)

    //48S ignores this parameter (choose any value)
    //60S ONLY: to increase assist power, choose the lowest spoofed voltage that doesn't cause p-codes during heavy assist (e.g. P1440)
        //#define MIN_SPOOFED_VOLTAGE_60S 180 //voltage spoofing related p-codes won't occur in any car
        //#define MIN_SPOOFED_VOLTAGE_60S 175
        #define MIN_SPOOFED_VOLTAGE_60S 170 //recommended starting value //choose higher voltage if p-codes occur during heavy assist
        //#define MIN_SPOOFED_VOLTAGE_60S 165
        //#define MIN_SPOOFED_VOLTAGE_60S 160
        //#define MIN_SPOOFED_VOLTAGE_60S 155
        //#define MIN_SPOOFED_VOLTAGE_60S 150 //voltage spoofing related p-codes will occur in most cars during heavy assist

    //////////////////////////////////////////////////////////////////

    /////////////////////////
    //                     //
    //  Firmware Settings  //
    //                     //
    /////////////////////////

    //the default values below will work in any car.
    //you only need to modify these parameters if you don't like the default behavior.

    #define STACK_SoC_MAX 85 //maximum state of charge before regen  is disabled
    #define STACK_SoC_MIN 10 //minimum state of charge before assist is disabled

    #define CELL_VMAX_REGEN                     43000 //43000 = 4.3000 volts
    #define CELL_VMIN_ASSIST                    31900
    #define CELL_VMAX_GRIDCHARGER               39600 //3.9 volts is 75% SoC //other values: See SoC.cpp //MUST be less than 'CELL_VREST_85_PERCENT_SoC'
    #define CELL_VMIN_GRIDCHARGER               30000 //grid charger will not charge severely empty cells
    #define CELL_VMIN_KEYOFF                    CELL_VREST_10_PERCENT_SoC //when car is off, LiBCM turns off below this voltage
    #define CELL_BALANCE_MIN_SoC                65    //when car is off, cell balancing is disabled when battery is less than this percent charged
    #define CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE 32    //'32' = 3.2 mV //CANNOT exceed 255 counts (25.5 mV)
    #define CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT 22    //'22' = 2.2 mV //LTC6804 measurement uncertainty is 2.2 mV //MUST be less than CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE
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

    //JTS2doNext: add LIBCM_KEYOFF_SLEEP_DELAY_HOURS
    #define KEYOFF_DELAY_LIBCM_TURNOFF_MINUTES 10 //When SoC is between 0 & 10%, LiBCM will remain on for this many minutes after keyOFF.
        //to turn LiBCM back on: turn ignition 'ON', or turn IMA switch off and on, or plug in USB cable

    //Choose which sign (±) the LCD displays when the battery is discharging
    #define DISPLAY_POSITIVE_SIGN_DURING_ASSIST //current is positive when battery is discharging
    //#define DISPLAY_NEGATIVE_SIGN_DURING_ASSIST //current is negative when battery is discharging

    //////////////////////////////////////////////////////////////////

    ////////////////////////
    //                    //
    //  Debug Parameters  //
    //                    //
    ////////////////////////

    //don't modify these parameters unless you know what you're doing.  They are primary for mudder's internal testing

    //#define RUN_BRINGUP_TESTER_MOTHERBOARD //requires external test PCB (that you don't have)
    //#define RUN_BRINGUP_TESTER_GRIDCHARGER //requires external test equipment
    //JTS2doNow: Add case that constantly displays "firmware not installed, see linsight.org/install/firmware"
    //JTS2doNow: Add a one-time "firmware updated" LCD message, each time the firmware updates

    #define CHECK_FOR_SAFETY_COVER //comment if testing LiBCM without the cover

    #define DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS 1000 //JTS2doLater: Model after "debugUSB_printLatestData"

    //#define DISABLE_ASSIST //uncomment to (always) disable assist
    //#define DISABLE_REGEN  //uncomment to (always) disable regen
    //#define REDUCE_BACKGROUND_REGEN_UNLESS_BRAKING //EXPERIMENTAL! //JTS2doLater: Make this work (for Balto)

    //choose which functions control the LEDs
        #define LED_NORMAL //enable "LED()" functions (see debug.c)
        //#define LED_DEBUG //enable "debugLED()" functions (FYI: blinkLED functions won't work)

    //#define LIDISPLAY_DEBUG_ENABLED //uncomment to enable updates to text box ID # T12 on LiDisplay driving page -- this shows raw comm data from LiDisplay to LiBCM
    #define LIDISPLAY_CELL_COLOR_BIN_SIZE_COUNTS 64 //64 = 6.4mV window between cell colours on the grid charging page.  Don't go below CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE
	#define LIDISPLAY_SPLASH_PAGE_MS 2000 //How long the splash page shows on LiDisplay.  Default 2000 (2 seconds)
	#define LIDISPLAY_GRID_CHARGE_PAGE_COOLDOWN_MS 3000 // Keep displaying the grid charging page this long before showing splash page when GC unplugged

    /*
    JTS2doLater:
        #define SERIAL_H_LINE_CONNECTED NO //H-Line wire connected to OEM BCM connector pin B01
        #define KEYOFF_TURNOFF_LIBCM_AFTER_HOURS 48 //LiBCM turns off this many hours after keyOFF.

        Change these #define statements so that all they do is reconfigure EEPROM values.
        Also, change this file so that all #define statements are commented out by default.
        With these two changes, a user only needs to edit config.h if they want to change a previously sent parameter.
        If user doesn't uncomment anything, then the previously uploaded value remains in EEPROM
    */

    //JTS2doLater: Implement this feature
    //if using 1500 watt charger with 120 volt extension cord, choose input current limit
        //#define CHARGER_INPUT_CURRENT__15A_MAX //select this option if using 12 AWG extension cord up to 100 feet, or 14 AWG up to 50 feet**, else if;
        //#define CHARGER_INPUT_CURRENT__13A_MAX //select this option if using 14 AWG extension cord up to 100 feet, or 16 AWG up to 50 feet**, else if;
        //#define CHARGER_INPUT_CURRENT__10A_MAX //select this option if using 16 AWG extension cord up to 100 feet, or 18 AWG up to 50 feet**.
            //**please verify maximum continuous current rating for your specific extension cord

#endif
