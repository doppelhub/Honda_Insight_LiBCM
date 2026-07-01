//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//config.h - compile time configuration parameters

#ifndef config_h
    #define config_h
    #include "src/libcm.h"

    #define FW_VERSION "0.9.6b"
    #define BUILD_DATE "2026APR20"

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
    //these values are stored in EEPROM (see "Firmware Settings" note below), but unlike those, there is no hardcoded fallback:
    //LiBCM refuses to run (loud warning + halt) until each of these groups has been set at least once via config.h.

    //choose your battery type:
       //#define CONFIG_BATTERY_TYPE_5AhG3 //if you're not sure, you probably have this battery
        //#define CONFIG_BATTERY_TYPE_47Ah  //aka FoMoCo //aka Samsung SDI modules

    //choose how many cells are in series:
        //#define CONFIG_STACK_IS_48S //All 5AhG3 Kits & 47Ah Kits with QTY4 modules
        //#define CONFIG_STACK_IS_60S //47Ah Kits with QTY5 modules //MUST use voltage-spoofing mode DISABLE (its default)

    //choose which grid charger is installed
        //#define CONFIG_GRIDCHARGER_IS_NOT_1500W //All 5AhG3 Kits & 'standard' 47Ah Kits
        //#define CONFIG_GRIDCHARGER_IS_1500W     //47Ah Kits with 'fast' 6.5A charger

    //choose ONE of the following
    //must match actual "current hack" hardware configuration:
        //#define CONFIG_SET_CURRENT_HACK_60 //actually +56.3% //undocumented hardware option -- confirm with mudder before using
        //#define CONFIG_SET_CURRENT_HACK_40 //actually +45.8% //most LiBCM users installed this hardware option
        //#define CONFIG_SET_CURRENT_HACK_20 //actually +25.0%
        //#define CONFIG_SET_CURRENT_HACK_00 //OEM configuration (no current hack installed inside MCM)

    //choose which display to use
    //using both displays simultaneously could cause timing issues (FYI: the Serial Monitor prints '*' each time the loop period is violated)
        #define LCD_4X20_CONNECTED  //display included with all LiBCM Kits
        //#define LIDISPLAY_CONNECTED //optional color touch screen display //JTS2doLater: mudder has not yet tested this code. Use at your own risk.

    //you don't need to change any settings below this line (but you can if you know what you're doing):
    //__________________________________________________________________________________________________

    //////////////////////////////////////////////////////////////////

    ////////////////////////
    //                    //
    //  Voltage Spoofing  //
    //                    //
    ////////////////////////

    //'choose' exactly one option from each group below.
    //the default (DISABLE) should work in all cars, and is used until you uncomment a different option below.
    //modify these parameters if you want more power during heavy assist and/or regen.
    //this value is stored in EEPROM (see "Firmware Settings" note above): leaving every line below commented out
    //leaves the existing EEPROM value untouched; uncommenting one writes it to EEPROM once at the next keyOff.

    //48S ONLY: choose ONE of the following
    //60S MUST use 'CONFIG_VOLTAGE_SPOOFING_DISABLE' (or leave all lines commented out, since DISABLE is also the default):
        //#define CONFIG_VOLTAGE_SPOOFING_DISABLE              //spoof maximum possible pack voltage at all times //closest to OEM behavior
        //#define CONFIG_VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE //increase assist power by variably   spoofing pack voltage during assist
        //#define CONFIG_VOLTAGE_SPOOFING_ASSIST_ONLY_BINARY   //increase assist power by statically spoofing pack voltage during heavy assist
        //#define CONFIG_VOLTAGE_SPOOFING_ASSIST_AND_REGEN     //increase assist and regen power by variably spoofing pack voltage //DEPRECATED (regen too strong)
        //#define CONFIG_VOLTAGE_SPOOFING_LINEAR               //increase assist and regen power by requesting peak current level as per OEM (compatible with 100A fuse)

    //48S ignores this parameter (choose any value)
    //60S ONLY: to increase assist power, choose the lowest spoofed voltage that doesn't cause p-codes during heavy assist (e.g. P1440)
    //This value is stored in EEPROM (see "Firmware Settings" note above): leaving this commented out leaves the existing
    //EEPROM value (default 170) untouched; uncommenting one line below writes it to EEPROM once at the next boot.
        //#define CONFIG_MIN_SPOOFED_VOLTAGE_60S 180 //voltage spoofing related p-codes won't occur in any car
        //#define CONFIG_MIN_SPOOFED_VOLTAGE_60S 175
        //#define CONFIG_MIN_SPOOFED_VOLTAGE_60S 170 //recommended starting value //choose higher voltage if p-codes occur during heavy assist
        //#define CONFIG_MIN_SPOOFED_VOLTAGE_60S 165
        //#define CONFIG_MIN_SPOOFED_VOLTAGE_60S 160
        //#define CONFIG_MIN_SPOOFED_VOLTAGE_60S 155
        //#define CONFIG_MIN_SPOOFED_VOLTAGE_60S 150 //voltage spoofing related p-codes will occur in most cars during heavy assist

    //////////////////////////////////////////////////////////////////

    /////////////////////////
    //                     //
    //  Firmware Settings  //
    //                     //
    /////////////////////////

    //These parameters are stored in EEPROM, not compiled into the firmware image.
    //All lines below are commented out by default: leaving a line commented out leaves its EEPROM value untouched
    //(whatever was previously uploaded, or the firmware's hardcoded default on a brand new/factory-blank chip).
    //Uncommenting a line and re-uploading writes that value into EEPROM ONCE at the next boot; you can comment it
    //out again afterwards (or leave it uncommented -- it won't be re-written unless you change the value again).

    //#define CONFIG_STACK_SoC_MAX 85 //maximum state of charge before regen  is disabled
    //#define CONFIG_STACK_SoC_MIN 10 //minimum state of charge before assist is disabled

    //#define CONFIG_CELL_VMAX_REGEN                     43000 //43000 = 4.3000 volts
    //#define CONFIG_CELL_VMIN_ASSIST                    31900
    //#define CONFIG_CELL_VMAX_GRIDCHARGER               39600 //MUST be less than the 85%-SoC resting voltage for your battery type (SoC_cellVrest085PercentSoC_get())
    //#define CONFIG_CELL_VMIN_GRIDCHARGER               30000 //grid charger will not charge severely empty cells
    //#define CONFIG_CELL_VMIN_KEYOFF                    SoC_cellVrest010PercentSoC_get() //when car is off, LiBCM turns off below this voltage
    //#define CONFIG_CELL_BALANCE_MIN_SoC                65    //when car is off, cell balancing is disabled when battery is less than this percent charged
    //#define CONFIG_CELL_BALANCE_MAX_TEMP_C             40

    //temp setpoints
    //#define CONFIG_COOL_BATTERY_ABOVE_TEMP_C_KEYOFF       36 //cabin air cooling
    //#define CONFIG_COOL_BATTERY_ABOVE_TEMP_C_GRIDCHARGING 30
    //#define CONFIG_COOL_BATTERY_ABOVE_TEMP_C_KEYON        30
    //#define CONFIG_HEAT_BATTERY_BELOW_TEMP_C_KEYON        16 //cabin air heating, or heater PCB (if installed)
    //#define CONFIG_HEAT_BATTERY_BELOW_TEMP_C_GRIDCHARGING 16
    //#define CONFIG_HEAT_BATTERY_BELOW_TEMP_C_KEYOFF       10

    //power saving
    //#define CONFIG_KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC_PERCENT 50 //when keyOFF (unless grid charger plugged in) //set to 100 to disable when keyOFF
    //#define CONFIG_POWEROFF_DELAY_AFTER_KEYOFF_PACK_EMPTY_MINUTES      10 //When SoC is below 10%, LiBCM will remain on for this many minutes after keyOFF.
    //#define CONFIG_POWEROFF_DELAY_AFTER_KEYOFF_DAYS                     5 //LiBCM turns off this many days after keyOff, unless grid charger powered //set to 0 to disable
    //to turn LiBCM back on: turn ignition 'ON', or turn IMA switch off and on, or plug in USB cable

    //Choose which sign (±) the LCD displays when the battery is discharging
    //#define CONFIG_DISPLAY_POSITIVE_SIGN_DURING_ASSIST //current is positive when battery is discharging
    //#define CONFIG_DISPLAY_NEGATIVE_SIGN_DURING_ASSIST //current is negative when battery is discharging

    //////////////////////////////////////////////////////////////////

    ////////////////////////
    //                    //
    //  Debug Parameters  //
    //                    //
    ////////////////////////

    //don't modify these parameters unless you know what you're doing.  They are primary for mudder's internal testing

    //#define RUN_BRINGUP_TESTER_MOTHERBOARD //requires external test PCB (that you don't have)
    //#define RUN_BRINGUP_TESTER_GRIDCHARGER //requires external test equipment

    #define CHECK_FOR_SAFETY_COVER //comment if testing LiBCM without the cover

    //#define CONFIG_DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS 1000 //JTS2doLater: Model after "debugUSB_printLatestData"

    //#define CONFIG_DISABLE_ASSIST //uncomment to (always) disable assist
    //#define CONFIG_DISABLE_REGEN  //uncomment to (always) disable regen
    //#define REDUCE_BACKGROUND_REGEN_UNLESS_BRAKING //EXPERIMENTAL! //JTS2doLater: Make this work (for Balto)
    //#define CONFIG_IGNORE_CELL_VOLTAGE_MISMATCH //prevents fatal error if pack size doesn't match user selection

    //choose which functions control the LEDs
        #define LED_NORMAL //enable "LED()" functions (see debug.c)
        //#define LED_DEBUG //enable "debugLED()" functions (FYI: blinkLED functions won't work)

    //#define LIDISPLAY_DEBUG_ENABLED //uncomment to enable updates to text box ID # T12 on LiDisplay driving page -- this shows raw comm data from LiDisplay to LiBCM
	//#define LIDISPLAY_USE_NERD_SCREEN // uncomment to enable the "Nerd Screen" which is an alternative display layout for the driving page.

    //JTS2doLater: Implement this feature
    //if using 1500 watt charger with 120 volt extension cord, choose input current limit
        //#define CHARGER_INPUT_CURRENT__15A_MAX //select this option if using 12 AWG extension cord up to 100 feet, or 14 AWG up to 50 feet**, else if;
        //#define CHARGER_INPUT_CURRENT__13A_MAX //select this option if using 14 AWG extension cord up to 100 feet, or 16 AWG up to 50 feet**, else if;
        //#define CHARGER_INPUT_CURRENT__10A_MAX //select this option if using 16 AWG extension cord up to 100 feet, or 18 AWG up to 50 feet**.
            //**please verify maximum continuous current rating for your specific extension cord

#endif
