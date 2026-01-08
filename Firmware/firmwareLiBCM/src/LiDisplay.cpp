//
//github.com/doppelhub/Honda_Insight_LiBCM

//LiDisplay (HMI) Serial Functions

#include "libcm.h"

// August 2025 -- Nerd Screen needs these to be variables instead of defined values
static uint8_t LiDisplay_DrivingPageId = 0;
static uint8_t LiDisplay_DrivingPageReqId = 0;

#define LIDISPLAY_SPLASH_PAGE_ID 1
#define LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID 2
#define LIDISPLAY_GRIDCHARGE_PAGE_ID 3
#define LIDISPLAY_SETTINGS_PAGE_ID 4

// These are the numbers of updatable elements on the respective screens
#define LIDISPLAY_DRIVING_PAGE_INTITIAL_MAX_ELEMENT_ID 7
#define LIDISPLAY_SPLASH_PAGE_INTITIAL_MAX_ELEMENT_ID 4
#define LIDISPLAY_GRIDCHARGE_PAGE_INTITIAL_MAX_ELEMENT_ID 7


#define LIDISPLAY_BUTTON_ID_SCREEN 0
#define LIDISPLAY_BUTTON_ID_FAN 1
#define LIDISPLAY_BUTTON_ID_BRIGHT 2

// The Nextion takes some time to power on.  Commands sent before it's fully online will not be received or acted upon.
// This causes problems if it's turning on because the grid charger was connected.
// We need to wait for it to power up before we tell it to go to the grid charger page.
#define LIDISPLAY_MINIMUM_TIME_TO_UPDATE_AFTER_POWER_ON_MILLIS 400 // Note Sept 2025 -- Smaller values were not enough.  Even 200ms was not enough.

#define LIDISPLAY_UPDATE_RATE_MILLIS 20			// One element is updated each time
#define LIDISPLAY_COMMAND_COOLDOWN_FRAMES 100	// 2024AUG19 -- We may be able to use a smaller value like 50

#ifdef STACK_IS_48S
    #define MAX_CELL_INDEX 47
#elif defined STACK_IS_60S
    #define MAX_CELL_INDEX 59
#endif

static bool LiDisplay_ThisIsFirstKeyOn = true;

uint8_t LiDisplayElementToUpdate = 0;
uint8_t LiDisplayCurrentPageNum = 0;
uint8_t LiDisplaySetPageNum = LiDisplay_DrivingPageId;

uint8_t LiDisplaySoCBarCount = 0;
uint8_t LiDisplayChrgAsstPicId = 22;
uint8_t LiDisplayWaitingForCommand = 0;

static uint8_t LiDisplay_currentParamId = 0;	// Settings Page currently selected parameter
static uint16_t LiDisplay_currentParamVal = 0;
static uint16_t LiDisplay_currentGlobalNumVal = 0;
static String LiDisplay_paramName_onScreen = "";
static uint16_t LiDisplay_paramVal_onScreen = 0;
static String Lidisplay_paramDesc_onScreen = "";

// WH accumulated during current LiDisplay power on (either current drive or grid charger plugged in)
static uint16_t LiDisplay_energyWHAssist = 0;
static uint16_t LiDisplay_energyWHRegen = 0;
static uint16_t LiDisplay_energyWHGridCharge = 0;
static uint16_t LiDisplay_lastWHGridCharge_onScreen = 9999999;


// Initializing to an absurd number for all 7 variables so that on first run they will be updated on screen
static uint16_t  LiDisplay_AvgCellVoltage_onScreen = 9999;
static uint8_t  LiDisplay_BattTemp_onScreen = 100;
static uint8_t  LiDisplay_FanSpeed_onScreen = 100;
static uint8_t  LiDisplay_PackVoltageActual_onScreen = 100;
static uint8_t  LiDisplay_PackVoltageSpoofed_onScreen = 100;
static uint8_t  LiDisplay_SoC_onScreen = 100;
static uint8_t  LiDisplay_SoCBars_onScreen = 100;

// Nerd Screen Only
static uint8_t	LiDisplay_NS_loCellNum_onScreen = 100;
static uint8_t	LiDisplay_NS_hiCellNum_onScreen = 100;


static uint16_t LiDisplay_AvgCellVoltage = 0;
static uint8_t maxElementId = 8;
static uint8_t LiDisplay_powerState = 0; // 0=Key off GC unplug    1=Key on GC unplug    2=Key off GC plugged    3=Key on GC plugged
static bool LiDisplay_heaterState_onScreen = true;	// Initializing to true because, by default, when a screen with T22 load, T22 is displayed

bool LiDisplaySplashPending = false;
bool LiDisplaySplashFromGridCharger = false;
bool LiDisplayPowerOffPending = false;
bool LiDisplayOnKeyOnWithNerdScreenEnabled = false;
bool LiDisplayOnGridChargerConnected = false;
bool LiDisplaySettingsPageRequested = false;

//bool LiDisplayGridChargerPageRequested = false;
static bool LiDisplayNeedToVerifyPowerState = false;
static uint16_t total_splash_page_delay_ms = 250; // Has to be at least 150 ms because of Nextion delays.

static uint32_t new_power_state_millis = 0;
static uint32_t new_page_millis = 0;
static uint32_t hmi_power_millis = 0;

static uint32_t gc_connected_millis_most_recent_diff = 0;
static bool LiDisplay_BuzzerRequested = false;
static uint32_t LiDisplay_buzzerRequestMS = 0;

static String gc_begin_soc_str = "0%";
static String gc_time = "00:00:00";
static String key_time = "00:00:00";
String gc_currently_selected_cell_id_str = "99";    // An absurd initialization value.

static uint32_t key_time_begin_ms = 0;
static uint32_t gc_chg_time_begin_millis = 0;

static uint8_t currentFanSpeed = 0;
static uint8_t LiDisplay_brightness = 100;

bool gc_sixty_s_fomoco_e_block_enabled = false;

const String attrMap[5] = {
    "txt",  // text value
    "val",  // numerical value
    "pic",  // picture id
    "bco",  // background colour
    "pco"   // primary colour
};

const uint8_t maxParamID = 1;
const String fanSpeedDisplay[4] = { "FAN OFF", "FAN LOW", "FAN MED", "FAN HIGH" };
const String editableParamMap[2] = { "CELL_VMAX_GRIDCHARGER", "LiDisp Cell Bal Res Window" };
const String editableParamDescriptions[2] = {
    "Charge cells up to this voltage\r\nMin: 37000\r\nMax: 41000\r\nDefault: 39600",
    "Higher numbers mean cell colours\r\nchange less frequently.\r\nMin: 32  Max: 255\r\nDefault: 64"
};
const uint16_t editableParamMinMax[2][2] = {
    {37000,41000},
    {   32,  255}
};

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_begin(void)
{
    #ifdef LIDISPLAY_CONNECTED

        #ifdef BATTERY_TYPE_47Ah
            #undef LIDISPLAY_GRIDCHARGE_PAGE_ID
            #define LIDISPLAY_GRIDCHARGE_PAGE_ID 5
        #endif

        LiDisplayElementToUpdate = 0;
        LiDisplaySplashPending = false;
        LiDisplayPowerOffPending = false;
		new_power_state_millis = 0;

    #elif defined RUN_BRINGUP_TESTER_MOTHERBOARD //do nothing
    #else
        power_usart1_disable(); //disable USART1 clock to save power
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updateNumericVal(uint8_t page, String elementName, uint8_t elementAttrIndex, uint16_t value) {
    #ifdef LIDISPLAY_CONNECTED
        String LiDisplay_Number_Str;

        LiDisplay_Number_Str = "page" + String(page) + "." + String(elementName) + "." + attrMap[elementAttrIndex] + "=" + value;

        LiDisplay_printString(LiDisplay_Number_Str);
		LiDisplay_writeInstructionTerminationBytes();
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updateStringVal(uint8_t page, String elementName, uint8_t elementAttrIndex, String value) {
    #ifdef LIDISPLAY_CONNECTED
        String LiDisplay_String_Str;

        LiDisplay_String_Str = "page" + String(page) + "." + String(elementName) + "." + attrMap[elementAttrIndex] + "=" + String('"') + value + String('"');

        LiDisplay_printString(LiDisplay_String_Str);
        LiDisplay_writeInstructionTerminationBytes();
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updateGlobalVariable(String elementName, uint16_t value) {
	// Use to update sys0, sys1, and sys2 global variables on the Nextion
    #ifdef LIDISPLAY_CONNECTED
        String LiDisplay_GlobalVarUpdate_Str;

        LiDisplay_GlobalVarUpdate_Str = String(elementName) + "=" + value;

        LiDisplay_printString(LiDisplay_GlobalVarUpdate_Str);
        LiDisplay_writeInstructionTerminationBytes();
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updateDebugTextBox(String raw_data_string) {
    #ifdef LIDISPLAY_DEBUG_ENABLED
		// Debug box is T12 on driving pages, but T20 on gc pages
		if (LiDisplayCurrentPageNum == LIDISPLAY_GRIDCHARGE_PAGE_ID) { LiDisplay_updateStringVal(LiDisplayCurrentPageNum, "t20", 0, raw_data_string); }
        else { LiDisplay_updateStringVal(LiDisplayCurrentPageNum, "t12", 0, raw_data_string); }
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateCorrectPage()
{
    if (key_getSampledState() == KEYSTATE_ON)
    {
        // Key is On
        if ( gpio_isGridChargerPluggedInNow())    { LiDisplaySetPageNum = LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID; }
        else if  (LiDisplaySplashPending)         { LiDisplaySetPageNum = LIDISPLAY_SPLASH_PAGE_ID;             }
        else if  (LiDisplaySettingsPageRequested) { LiDisplaySetPageNum = LIDISPLAY_SETTINGS_PAGE_ID;           }
        else                                      { LiDisplaySetPageNum = LiDisplay_DrivingPageReqId;            }
    }
	else
	{
        // Key is Off
        if (gpio_isGridChargerPluggedInNow()) {
            if (LiDisplaySettingsPageRequested) { LiDisplaySetPageNum = LIDISPLAY_SETTINGS_PAGE_ID; }
            else LiDisplaySetPageNum = LIDISPLAY_GRIDCHARGE_PAGE_ID;
        }
        else if (LiDisplaySplashPending) { LiDisplaySetPageNum = LIDISPLAY_SPLASH_PAGE_ID; }
        else if (LiDisplaySettingsPageRequested) { LiDisplaySetPageNum = LIDISPLAY_SETTINGS_PAGE_ID; }
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_resetDrivingPageVariables()
{
	maxElementId = LIDISPLAY_DRIVING_PAGE_INTITIAL_MAX_ELEMENT_ID;
	LiDisplayElementToUpdate = 0;
	// Set all the onScreen variables to their initialization values.
	LiDisplay_AvgCellVoltage_onScreen = 9999;		// T28
	LiDisplay_BattTemp_onScreen = 100;
	LiDisplay_FanSpeed_onScreen = 100;
	LiDisplay_heaterState_onScreen = true;			// T22
	LiDisplay_NS_loCellNum_onScreen = 100;			// T21
	LiDisplay_NS_hiCellNum_onScreen = 100;			// T20
	LiDisplay_PackVoltageActual_onScreen = 100;
	LiDisplay_PackVoltageSpoofed_onScreen = 100;	// T24
	LiDisplay_SoC_onScreen = 100;
	LiDisplay_SoCBars_onScreen = 100;

	LiDisplay_updateGlobalVariable("sys2", LIDISPLAY_SPLASH_PIC);
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_resetGridChargerPageVariables()
{
	maxElementId = LIDISPLAY_GRIDCHARGE_PAGE_INTITIAL_MAX_ELEMENT_ID;
	LiDisplayElementToUpdate = 0;

	gc_sixty_s_fomoco_e_block_enabled = false;
	LiDisplay_BattTemp_onScreen = 100;
	LiDisplay_heaterState_onScreen = true;			// T22
	LiDisplay_PackVoltageActual_onScreen = 100;
	LiDisplay_SoC_onScreen = 100;
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_resetSplashPageVariables()
{
	// Splash page is only shown for a few seconds
	// When we go to the splash page we want to make the correct updates (firmware hours and version) as fast as possible
	maxElementId = LIDISPLAY_SPLASH_PAGE_INTITIAL_MAX_ELEMENT_ID;
	LiDisplayElementToUpdate = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_resetSettingsPageVariables(bool resetGlobalVar) {
    // Start at CELL_VMAX_GRIDCHARGER
	LiDisplay_currentParamId = 0;
	LiDisplay_currentParamVal = 0;
	LiDisplay_currentGlobalNumVal = 0;	// Number input
	LiDisplay_paramName_onScreen = "";
	LiDisplay_paramVal_onScreen = 0;
	LiDisplay_lastWHGridCharge_onScreen = 9999999;
	if (resetGlobalVar) { Lidisplay_paramDesc_onScreen = ""; }
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_handleKeyOrGCStateChange()
{
	// TODO_NATALYA (2024 Jan) -- hmi_power_millis and gc_connected_millis timers to ensure screen is changed can probably be dealt with in here
	// A new variable new_power_state_millis might be able to replace both.
	// Nextion does NOT allow Arduino to ask "what page are you on?" which is why we will need delays around key cycle/gc plugging events

	if (millis() < 50)
	{
		Serial.print(F("\nLiDisplay_handleKeyOrGCStateChange - millis less than 50 - not changing power state."));
		return;	// key_getSampledState might not be accurate yet.
	}

	uint8_t new_power_state = 0;

	if (key_getSampledState() == KEYSTATE_ON) { new_power_state += 1; }
	if (gpio_isGridChargerPluggedInNow() == YES) { new_power_state += 2; }

	if (LiDisplay_powerState != new_power_state)
	{
		switch (LiDisplay_powerState) {
			case 0:	// Key is OFF and Grid Charger is unplugged
				switch(new_power_state) {
					case 0: break;	// Should never end up here
					case 1: /*Serial.print(F("\nLiDisplay_handleKeyOrGCStateChange - calling LiDisplay_keyOn()"));*/ LiDisplay_keyOn(); break; // Key now ON
					case 2: LiDisplay_gridChargerPluggedIn(); break; // GC now plugged in
					case 3: LiDisplay_keyOn(); LiDisplay_gridChargerPluggedIn(); break; // Driver Key ON and plugged in GC in same frame (unlikely to happen)
				}
				break;
			case 1: // Key is ON or possibly off but contactor relay hasn't opened yet
				switch(new_power_state) {
					case 0: fan_requestSpeed(FAN_REQUESTOR_USER, FAN_OFF); LiDisplay_keyOff(); total_splash_page_delay_ms = 250; break; // Contactor relay finally opened
					case 1: break;	// Should never end up here
					case 2: LiDisplay_keyOff(); LiDisplay_gridChargerPluggedIn(); break; // Driver plugged in GC on same frame as contactor relay opened (unlikely to happen)
					case 3: LiDisplay_gridChargerPluggedIn(); break; // Driver plugged in GC, should get LiDisplay warning page and LiBCM will beep
				}
				break;
			case 2: // Grid Charger is plugged in
				switch(new_power_state) {
					case 0: fan_requestSpeed(FAN_REQUESTOR_USER, FAN_OFF); LiDisplay_gridChargerUnplugged(); total_splash_page_delay_ms = (250 + LIDISPLAY_GRID_CHARGE_PAGE_COOLDOWN_MS); break;
					case 1: LiDisplay_gridChargerUnplugged(); LiDisplay_keyOn(); break; // Driver unplugged GC on same frame as Key ON (unlikely to happen)
					case 2: break;	// Should never end up here
					case 3: LiDisplay_keyOn(); break; // GC is already plugged in, Driver turned Key ON, LiDisplay needs to display warning, LiBCM will beep
				}
				break;
			case 3: // Key On and GC plugged in -- LiBCM should be beeping at driver, driver likely to take action
				switch(new_power_state) {
					case 0: fan_requestSpeed(FAN_REQUESTOR_USER, FAN_OFF); LiDisplay_keyOff(); LiDisplay_gridChargerUnplugged(); total_splash_page_delay_ms = (250 + LIDISPLAY_GRID_CHARGE_PAGE_COOLDOWN_MS); break; // Driver unplugged GC at exact instant contactor relay opened (might happen -- edge case)
					case 1: LiDisplay_gridChargerUnplugged(); break; // Driver unplugged GC
					case 2: LiDisplay_keyOff(); break; // Driver keyed OFF, contactor finally opened
					case 3: break;	// Should never end up here
				}
				break;
		}
		new_power_state_millis = millis();
		LiDisplayNeedToVerifyPowerState = true;
		LiDisplay_updateDebugTextBox("Power state eval pending...");
	}
	LiDisplay_powerState = new_power_state;
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updateGlobalObjectVal(String elementName, uint8_t elementAttrIndex, String value) {
	// This is used for the number input on the settings page
    #ifdef LIDISPLAY_CONNECTED
        String LiDisplay_ObjectUpdate_Str;

        LiDisplay_ObjectUpdate_Str = String(elementName) + "." + attrMap[elementAttrIndex] + "=" + value;

        LiDisplay_printString(LiDisplay_ObjectUpdate_Str);
        LiDisplay_writeInstructionTerminationBytes();
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

String LiDisplay_getCellVoltage(String cell_id_str) {
    uint8_t cell_id = cell_id_str.toInt();
    uint8_t ic_index = 0;
    uint8_t ic_cell_num = 0;
    String formatted_cell_voltage = "0.0000";
    String returned_voltage = "";


    if (cell_id > MAX_CELL_INDEX) { return "ERROR"; }

    if (cell_id <= 11) { ic_index = 0; ic_cell_num = (cell_id); }
    else if (cell_id <= 23) { ic_index = 1; ic_cell_num = (cell_id - 12); }
    else if (cell_id <= 35) { ic_index = 2; ic_cell_num = (cell_id - 24); }
    else if (cell_id <= 47) { ic_index = 3; ic_cell_num = (cell_id - 36); }
    else if (cell_id <= 59) { ic_index = 4; ic_cell_num = (cell_id - 48); }

    returned_voltage = String(LTC68042result_specificCellVoltage_get(ic_index, ic_cell_num));
    formatted_cell_voltage[0] = returned_voltage[0];
    formatted_cell_voltage[2] = returned_voltage[1];
    formatted_cell_voltage[3] = returned_voltage[2];
    formatted_cell_voltage[4] = returned_voltage[3];
    formatted_cell_voltage[5] = returned_voltage[4];

    return formatted_cell_voltage;
}

/////////////////////////////////////////////////////////////////////////////////////////

LiDisplay_updateNextCellValue() {
    String LiDisplay_Color_Str;
    static uint8_t cellToUpdate = 0;
    uint8_t ic_index = 0;
    uint8_t ic_cell_num = 0;
    String cell_color_number = "2016";	// 2016 = Green
    int cell_voltage_diff_from_avg = 0;


    if (cellToUpdate > MAX_CELL_INDEX) cellToUpdate = 0;

    // NM To Do: Cells indexed 18 to 35 are the central block in the IMA battery.
    // We need to add a config variable for 18S+ or 18S- otherwise they may display out of order left-to-right for some installations
    // Right now (09 Feb 2023) it starts with cell index 18 on the left of the display, corresponding to the front of the vehicle.
    if (cellToUpdate <= 11) { ic_index = 0; ic_cell_num = (cellToUpdate); }
    else if (cellToUpdate <= 23) { ic_index = 1; ic_cell_num = (cellToUpdate - 12); }
    else if (cellToUpdate <= 35) { ic_index = 2; ic_cell_num = (cellToUpdate - 24); }
    else if (cellToUpdate <= 47) { ic_index = 3; ic_cell_num = (cellToUpdate - 36); }
    else if (cellToUpdate <= 59) { ic_index = 4; ic_cell_num = (cellToUpdate - 48); }

    LiDisplay_AvgCellVoltage = (LTC68042result_deltaCellVoltage_get() >> 1 ) + LTC68042result_loCellVoltage_get();
	cell_voltage_diff_from_avg = LiDisplay_AvgCellVoltage - LTC68042result_specificCellVoltage_get(ic_index, ic_cell_num);

	if (cell_voltage_diff_from_avg >= (LIDISPLAY_CELL_COLOR_BIN_SIZE_COUNTS * 2.5)) { cell_color_number = NEXTION_RED; }        // 63488 = Red
    else if (cell_voltage_diff_from_avg >= (LIDISPLAY_CELL_COLOR_BIN_SIZE_COUNTS * 1.5)) { cell_color_number = NEXTION_ORN; }   // 64480 = Orange
    else if (cell_voltage_diff_from_avg >= (LIDISPLAY_CELL_COLOR_BIN_SIZE_COUNTS * 0.5)) { cell_color_number = NEXTION_YEL; }   // 65504 = Yellow
    else if (cell_voltage_diff_from_avg >= (LIDISPLAY_CELL_COLOR_BIN_SIZE_COUNTS * -0.5)) { cell_color_number = NEXTION_GRN; }   // 2016 = Green
    else if (cell_voltage_diff_from_avg >= (LIDISPLAY_CELL_COLOR_BIN_SIZE_COUNTS * -1.5)) { cell_color_number = NEXTION_CYN; }   // 2047 = Cyan
    else if (cell_voltage_diff_from_avg >= (LIDISPLAY_CELL_COLOR_BIN_SIZE_COUNTS * -2.5)) { cell_color_number = NEXTION_BLU; }     // 31 = Blue
    else { cell_color_number = NEXTION_PUR; }	// 22556 = Purple

    LiDisplay_Color_Str = "page" + String(LIDISPLAY_GRIDCHARGE_PAGE_ID) + ".j" + String(cellToUpdate) + ".pco" + "=" + cell_color_number;

    LiDisplay_printString(LiDisplay_Color_Str);
    LiDisplay_writeInstructionTerminationBytes();

	// When the driver presses a cell it displays the voltage in white text.
	// After cycling through all cells in the pack, when we get to the selected cell we want to gray that text because the voltage might have changed.
    if (gc_currently_selected_cell_id_str.toInt() == cellToUpdate)
	{
        LiDisplay_updateNumericVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t17", 4, NEXTION_GRY);	// 44373 = Gray
        gc_currently_selected_cell_id_str = "99";	//	Set to an impossible number so we don't end up here again until the user presses another cell.
    }

    cellToUpdate += 1;
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateKeyTimeStr(bool reset) {
    // TODO_NATALYA: Figure out how to use time_sinceLatestKeyOn_seconds() without modulo or division to get hours and minutes and without a gigantic for loop
    uint32_t current_key_on_ms = 0;
    uint16_t current_key_time_seconds = 0;
    static uint8_t kt_s = 0;
    static uint8_t kt_m = 0;
    static uint8_t kt_h = 0;

    if (reset) { kt_s = 0; kt_m = 0; kt_h = 0; }

    current_key_on_ms = (uint32_t)(millis() - key_time_begin_ms);
    current_key_time_seconds = (uint16_t)(current_key_on_ms * 0.001);

    if (current_key_time_seconds >= 1)
	{
        kt_s += current_key_time_seconds;
        key_time_begin_ms += current_key_on_ms; // Ratcheting key_time_begin_ms upwards so we don't introduce an error of more than 1s
        key_time_begin_ms += 13;    // 12ms still too little.  time_loopPeriod_ms_get was also too little.
    }

    if (kt_s >= 60)
	{   // Assumes < 60s passing between runs of this function.  If that's not the case there is a much more serious issue at hand.
        kt_s -= 60;
        kt_m += 1;
    }
    if (kt_m >= 60)
	{
        kt_m -= 60;
        kt_h += 1;
    }
    if (kt_h >= 99) { kt_h = 0; }  // Will someone leave the car key-on for +99 hours?

    key_time = "";
    (kt_h > 9) ? key_time = key_time + kt_h : key_time = key_time + "0" + kt_h;
    key_time = key_time + ":";
    (kt_m > 9) ? key_time = key_time + kt_m : key_time = key_time + "0" + kt_m;
    key_time = key_time + ":";
    (kt_s > 9) ? key_time = key_time + kt_s : key_time = key_time + "0" + kt_s;

}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateGCTimeStr(bool reset) {
	uint32_t current_gc_charging_ms = 0;
    uint16_t current_gc_charging_time_seconds = 0;
    static uint8_t gc_t_s = 0;
    static uint8_t gc_t_m = 0;
    static uint8_t gc_t_h = 0;
    static bool gc_was_paused = false;

	if (reset) { gc_t_s = 0; gc_t_m = 0; gc_t_h = 0; }

	// Increment time only while charging
    if (gpio_isGridChargerChargingNow())
	{
		if (gc_was_paused)
		{
            gc_chg_time_begin_millis = (millis() - gc_connected_millis_most_recent_diff);
            gc_was_paused = false;
        }

        gc_connected_millis_most_recent_diff = (millis() - gc_chg_time_begin_millis);

		current_gc_charging_ms = (uint32_t)(millis() - gc_chg_time_begin_millis);
		current_gc_charging_time_seconds = (uint16_t)(current_gc_charging_ms * 0.001);

		if (current_gc_charging_time_seconds >= 1)
		{
	        gc_t_s += current_gc_charging_time_seconds;
	        gc_chg_time_begin_millis += current_gc_charging_ms;	// Ratcheting gc_connected_millis upwards so we don't introduce an error of more than 1s
	        gc_chg_time_begin_millis += 13;						// Account for 13ms delay
	    }


		if (gc_t_s >= 60)
		{   // Assumes < 60s passing between runs of this function.  If that's not the case there is a much more serious issue at hand.
	        gc_t_s -= 60;
	        gc_t_m += 1;
	    }
	    if (gc_t_m >= 60)
		{
	        gc_t_m -= 60;
	        gc_t_h += 1;
	    }
	    if (gc_t_h >= 99) { gc_t_h = 0; }  // Will the grid charger actively be charging for +99 hours?

	    gc_time = "";
	    (gc_t_h > 9) ? gc_time = gc_time + gc_t_h : gc_time = gc_time + "0" + gc_t_h;
	    gc_time = gc_time + ":";
	    (gc_t_m > 9) ? gc_time = gc_time + gc_t_m : gc_time = gc_time + "0" + gc_t_m;
	    gc_time = gc_time + ":";
	    (gc_t_s > 9) ? gc_time = gc_time + gc_t_s : gc_time = gc_time + "0" + gc_t_s;


	} else { gc_was_paused = true; } // Still plugged in but not charging
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateChrgAsstGaugeBars() {
    // 22 is empty, 23 is 1 bar asst, 40 is 18 bars asst, 41 is 1 bar chrg, 58 is 18 bars chrg
    int16_t packEHP = (LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps()) * 0.00134; // USA Electrical Horsepower is defined as 746 Watts

    if (packEHP <= -18) { LiDisplayChrgAsstPicId = 58; }
    else if (packEHP <= -17) { LiDisplayChrgAsstPicId = 57; }
    else if (packEHP <= -16) { LiDisplayChrgAsstPicId = 56; }
    else if (packEHP <= -15) { LiDisplayChrgAsstPicId = 55; }
    else if (packEHP <= -14) { LiDisplayChrgAsstPicId = 54; }
    else if (packEHP <= -13) { LiDisplayChrgAsstPicId = 53; }
    else if (packEHP <= -12) { LiDisplayChrgAsstPicId = 52; }
    else if (packEHP <= -11) { LiDisplayChrgAsstPicId = 51; }
    else if (packEHP <= -10) { LiDisplayChrgAsstPicId = 50; }
    else if (packEHP <=  -9) { LiDisplayChrgAsstPicId = 49; }
    else if (packEHP <=  -8) { LiDisplayChrgAsstPicId = 48; }
    else if (packEHP <=  -7) { LiDisplayChrgAsstPicId = 47; }
    else if (packEHP <=  -6) { LiDisplayChrgAsstPicId = 46; }
    else if (packEHP <=  -5) { LiDisplayChrgAsstPicId = 45; }
    else if (packEHP <=  -4) { LiDisplayChrgAsstPicId = 44; }
    else if (packEHP <=  -3) { LiDisplayChrgAsstPicId = 43; }
    else if (packEHP <=  -2) { LiDisplayChrgAsstPicId = 42; }
    else if (packEHP <=  -1) { LiDisplayChrgAsstPicId = 41; }
    else if (packEHP <=   0) { LiDisplayChrgAsstPicId = 22; }
    else if (packEHP <=   1) { LiDisplayChrgAsstPicId = 23; }
    else if (packEHP <=   2) { LiDisplayChrgAsstPicId = 24; }
    else if (packEHP <=   3) { LiDisplayChrgAsstPicId = 25; }
    else if (packEHP <=   4) { LiDisplayChrgAsstPicId = 26; }
    else if (packEHP <=   5) { LiDisplayChrgAsstPicId = 27; }
    else if (packEHP <=   6) { LiDisplayChrgAsstPicId = 28; }
    else if (packEHP <=   7) { LiDisplayChrgAsstPicId = 29; }
    else if (packEHP <=   8) { LiDisplayChrgAsstPicId = 30; }
    else if (packEHP <=   9) { LiDisplayChrgAsstPicId = 31; }
    else if (packEHP <=  10) { LiDisplayChrgAsstPicId = 32; }
    else if (packEHP <=  11) { LiDisplayChrgAsstPicId = 33; }
    else if (packEHP <=  12) { LiDisplayChrgAsstPicId = 34; }
    else if (packEHP <=  13) { LiDisplayChrgAsstPicId = 35; }
    else if (packEHP <=  14) { LiDisplayChrgAsstPicId = 36; }
    else if (packEHP <=  15) { LiDisplayChrgAsstPicId = 37; }
    else if (packEHP <=  16) { LiDisplayChrgAsstPicId = 38; }
    else if (packEHP <=  17) { LiDisplayChrgAsstPicId = 39; }
    else                     { LiDisplayChrgAsstPicId = 40; }
    // 2022 Sept 07 -- NM To Do: The assist display can only show up to 18 HP of assist, but LiBCM can put out over 20 HP
    // Need to edit the HMI file to have a graphical display of those extra HP, probably by further highlighting some of the assist bars
};

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateSoCGaugeBars() {
  if (SoC_getBatteryStateNow_percent() >= 76) { LiDisplaySoCBarCount = 20; }
  else if (SoC_getBatteryStateNow_percent() >= 73) { LiDisplaySoCBarCount = 19; }
  else if (SoC_getBatteryStateNow_percent() >= 70) { LiDisplaySoCBarCount = 18; }
  else if (SoC_getBatteryStateNow_percent() >= 67) { LiDisplaySoCBarCount = 17; }
  else if (SoC_getBatteryStateNow_percent() >= 64) { LiDisplaySoCBarCount = 16; }
  else if (SoC_getBatteryStateNow_percent() >= 61) { LiDisplaySoCBarCount = 15; }
  else if (SoC_getBatteryStateNow_percent() >= 58) { LiDisplaySoCBarCount = 14; }
  else if (SoC_getBatteryStateNow_percent() >= 55) { LiDisplaySoCBarCount = 13; }
  else if (SoC_getBatteryStateNow_percent() >= 52) { LiDisplaySoCBarCount = 12; }
  else if (SoC_getBatteryStateNow_percent() >= 49) { LiDisplaySoCBarCount = 11; }
  else if (SoC_getBatteryStateNow_percent() >= 46) { LiDisplaySoCBarCount = 10; }
  else if (SoC_getBatteryStateNow_percent() >= 43) { LiDisplaySoCBarCount =  9; }
  else if (SoC_getBatteryStateNow_percent() >= 40) { LiDisplaySoCBarCount =  8; }
  else if (SoC_getBatteryStateNow_percent() >= 37) { LiDisplaySoCBarCount =  7; }
  else if (SoC_getBatteryStateNow_percent() >= 34) { LiDisplaySoCBarCount =  6; }
  else if (SoC_getBatteryStateNow_percent() >= 34) { LiDisplaySoCBarCount =  5; }
  else if (SoC_getBatteryStateNow_percent() >= 31) { LiDisplaySoCBarCount =  4; }
  else if (SoC_getBatteryStateNow_percent() >= 28) { LiDisplaySoCBarCount =  3; }
  else if (SoC_getBatteryStateNow_percent() >= 25) { LiDisplaySoCBarCount =  2; }
  else if (SoC_getBatteryStateNow_percent() >= 22) { LiDisplaySoCBarCount =  1; }
  else                                             { LiDisplaySoCBarCount =  0; }
  return;
};

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateFanSpeedStr() {
    if (fan_getSpeed_now() == FAN_HIGH) {
        currentFanSpeed = 3;
    } else if (fan_getSpeed_now() == FAN_MED) {
        currentFanSpeed = 2;
    } else if (fan_getSpeed_now() == FAN_LOW) {
        currentFanSpeed = 1;
    } else {
        currentFanSpeed = 0;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_checkFirmwareExpiration() {
	if ((REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS - eeprom_hoursSinceLastFirmwareUpdate_get()) <= 0)
	{
		#ifdef LIDISPLAY_DEBUG_ENABLED
			#undef LIDISPLAY_DEBUG_ENABLED
	    #endif
		LiDisplay_updateStringVal(0, "t12", 0, "FIRMWARE EXPIRED");
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updatePage() {
    #ifdef LIDISPLAY_CONNECTED
        static String LiDisplay_Page_Str;
        LiDisplay_Page_Str = "page " + String(LiDisplaySetPageNum);

		new_page_millis = millis();

        Serial.print(F("\n"));
        Serial.print("LiDisplay_updatePage ");
        Serial.print(LiDisplay_Page_Str);

		if (LiDisplaySetPageNum == LIDISPLAY_SETTINGS_PAGE_ID) LiDisplay_resetSettingsPageVariables(true);
		if (LiDisplaySetPageNum == LIDISPLAY_SPLASH_PAGE_ID) LiDisplay_resetSplashPageVariables();
		if (LiDisplaySetPageNum == LIDISPLAY_GRIDCHARGE_PAGE_ID) LiDisplay_resetGridChargerPageVariables();
		if (LiDisplaySetPageNum == LiDisplay_DrivingPageReqId) LiDisplay_resetDrivingPageVariables();

        LiDisplay_printString(LiDisplay_Page_Str);
        LiDisplay_writeInstructionTerminationBytes();

        LiDisplayCurrentPageNum = LiDisplaySetPageNum;
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////


String LiDisplay_readCommand() {
    String ret = "";
    char buffer = "";

    while (LiDisplay_bytesAvailableToRead() > 0) {
        buffer = LiDisplay_readByte();
        if ((uint8_t)buffer != 0xff) {  // Ignore Termination character
            if ((uint8_t)buffer != 26) ret += buffer;   // Ignore Empty Spaces
        }
    };
	//LiDisplay_updateDebugTextBox(ret);
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_processCommand(String cmd_str) {
    uint8_t cmd_page_id = 0;
    char cmd_obj_type = "";
    String cmd_obj_id_str = "";
    uint8_t ic_cell_address[2] = {0,0};
	String instruction_str = "";

    cmd_page_id = cmd_str[1] - '0'; // Subtract '0' from a char to get the actual integer value.
    cmd_obj_type = cmd_str[3];

    /*  2023 OCT 17
    Command strings are sent by Nextion to LiBCM when you press, then (usually) release, certain objects on the screen.

    Command string format:
    page . object . action

    Examples:
    p0.b0.rel       // On page 0, button 0 was pressed then released
    p3.j04.rel      // On page 3, bar-graph 04 was pressed then released

    Most commands will be either 9 or 10 characters.

    On the grid charging pages, cells are represented as rectangles, which in the Nextion software are bar-graph objects.
    For simplicity, they are always full (1 solid colour) and we colour-code them to show balance relative to the other cells.
    */

    if (String(cmd_obj_type) == "b")
	{
        // Button Pressed
        if ((cmd_page_id == (uint8_t)LiDisplay_DrivingPageId) || (cmd_page_id == (uint8_t)LiDisplay_DrivingPageReqId) || (cmd_page_id == (uint8_t)LIDISPLAY_GRIDCHARGE_PAGE_ID))
		{
            if ((cmd_str[4] - '0') == (uint8_t)LIDISPLAY_BUTTON_ID_SCREEN)
			{	// Screen Button from either Driving or GC Page
				LiDisplaySettingsPageRequested = true;
			}
            else if ((cmd_str[4] - '0') == (uint8_t)LIDISPLAY_BUTTON_ID_FAN)
			{
				// Fan Button from driving page pressed
				switch (fan_getSpeed_now()) {
					case FAN_HIGH: fan_requestSpeed(FAN_REQUESTOR_USER, FAN_OFF); LiDisplay_updateDebugTextBox("Requested Fan Off"); break;
					case FAN_LOW: fan_requestSpeed(FAN_REQUESTOR_USER, FAN_HIGH); LiDisplay_updateDebugTextBox("Requested Fan High"); break;
					default: fan_requestSpeed(FAN_REQUESTOR_USER, FAN_LOW); LiDisplay_updateDebugTextBox("Requested Fan Low"); break;
				}
			}
			else if ((cmd_str[4] - '0') == (uint8_t)LIDISPLAY_BUTTON_ID_BRIGHT)
			{
				// Brightness button pressed
				if (LiDisplay_brightness == 100) { instruction_str = "dim=33"; LiDisplay_brightness = 33; LiDisplay_updateDebugTextBox("Req'd Bright 33"); }
				else if (LiDisplay_brightness == 33) { instruction_str = "dim=66"; LiDisplay_brightness = 66; LiDisplay_updateDebugTextBox("Req'd Bright 66"); }
				else if (LiDisplay_brightness == 66) { instruction_str = "dim=100"; LiDisplay_brightness = 100; LiDisplay_updateDebugTextBox("Req'd Bright 100"); }
				LiDisplay_printString(instruction_str);
				LiDisplay_writeInstructionTerminationBytes();
			}
        }
		else if (cmd_page_id == (uint8_t)LIDISPLAY_SETTINGS_PAGE_ID)
		{
            if ((cmd_str[4] - '0') == (uint8_t)LIDISPLAY_BUTTON_ID_SCREEN)
			{
				// Screen Button was pressed -- return to either driving or gridcharge page
				LiDisplaySettingsPageRequested = false;
				LiDisplay_calculateCorrectPage();
			}
			else if ((cmd_str[4] - '0') == 1)
			{
				// Left Arrow Pressed
				if (LiDisplay_currentParamId > 0) { LiDisplay_currentParamId -= 1;}
			}
			else if ((cmd_str[4] - '0') == 2)
			{
				// Right Arrow Pressed
				if (LiDisplay_currentParamId < maxParamID) { LiDisplay_currentParamId += 1;}
			}
			else if ((cmd_str[4] - '0') == 5)
			{
				// Trip Reset Button Pressed
				energy_zeroWhTripMeter();
				LiDisplay_updateDebugTextBox("Clearing LiDisplay Trip Meter");
			}
        }
    }
	else if (String(cmd_obj_type) == "j")
	{
        // Bar-Graph Pressed
        cmd_obj_id_str = (String(cmd_str[4]) + String(cmd_str[5]));

        LiDisplay_updateStringVal(cmd_page_id, "t17", 0, ("Cell " + cmd_obj_id_str + ": " + LiDisplay_getCellVoltage(cmd_obj_id_str) + "V"));
        if (cmd_obj_id_str.toInt() < 10) cmd_obj_id_str = cmd_obj_id_str[1];    							// Nextion gets confused by leading 0.
        LiDisplay_updateNumericVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, String("j" + cmd_obj_id_str), 4, NEXTION_WHT);	// Setting cell bar and text colour to white
        LiDisplay_updateNumericVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t17", 4, NEXTION_WHT);						// 65535 = White

        gc_currently_selected_cell_id_str = cmd_obj_id_str;
    }
	buzzer_requestTone(BUZZER_REQUESTOR_USER, BUZZER_LOW);
	LiDisplay_BuzzerRequested = true;
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_enforceCorrectPowerState() {
	switch (LiDisplay_powerState)
	{
		default:
			// For reasons unknown, case 1 doesn't work, so we have to use default
			if (!gpio_HMIStateNow()) {
				gpio_turnHMI_on();
				LiDisplay_brightness = 100;
			}
			else if (gpio_HMIStateNow())
			{
				LiDisplayNeedToVerifyPowerState = false;
				LiDisplay_updateDebugTextBox(" "); // clear on-screen debug text
			}
			break;
		case 0:
			if (gpio_HMIStateNow()) {
				if (((millis() - new_power_state_millis) > total_splash_page_delay_ms) && (LiDisplaySplashPending))
				{
					LiDisplaySetPageNum = LIDISPLAY_SPLASH_PAGE_ID;
					energy_storeTripMeter(LiDisplay_energyWHAssist, LiDisplay_energyWHRegen);
					energy_storeTripMeterGridCharge(LiDisplay_energyWHGridCharge);
					LiDisplay_updatePage(); // If this isn't here the splash page may not appear after key-off.
					LiDisplaySplashPending = false;
				}
				if ((millis() - new_power_state_millis) > (total_splash_page_delay_ms + LIDISPLAY_SPLASH_PAGE_MS))
				{
					gpio_turnHMI_off();
					LiDisplayPowerOffPending = false;
					LiDisplayNeedToVerifyPowerState = false;
					LiDisplaySplashFromGridCharger = false;
					LiDisplay_energyWHAssist = 0;
					LiDisplay_energyWHRegen = 0;
					LiDisplay_energyWHGridCharge = 0;
				}
			}
			break;
		case 2: if (!gpio_HMIStateNow()) gpio_turnHMI_on(); LiDisplay_brightness = 100; LiDisplayNeedToVerifyPowerState = false; break;
		case 3: if (!gpio_HMIStateNow()) gpio_turnHMI_on(); LiDisplay_brightness = 100; LiDisplayNeedToVerifyPowerState = false; break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_userInputHandler() {
	//TODO_NATALYA (JTS added): Global formatting request: please either "vertically align brackets", or "place single line code on same line" (both shown below)
	String cmd_str = "";

	if (LiDisplayWaitingForCommand > 0) { LiDisplayWaitingForCommand -= 1; }
	if (LiDisplayWaitingForCommand == 1)
	{
		LiDisplayWaitingForCommand -= 1;
		cmd_str = LiDisplay_readCommand();

		if (cmd_str != "")
		{
			LiDisplay_updateDebugTextBox(String(cmd_str));
			LiDisplay_processCommand(cmd_str);
		}
	}
	if (LiDisplay_bytesAvailableToRead() && (LiDisplayWaitingForCommand == 0)) { LiDisplayWaitingForCommand = LIDISPLAY_COMMAND_COOLDOWN_FRAMES; }  // Wait LIDISPLAY_COMMAND_COOLDOWN_FRAMES frames before checking for another command from the user
}

/////////////////////////////////////////////////////////////////////////////////////////

bool LiDisplay_checkForPendingPageUpdate() {
	if ((LiDisplaySetPageNum != LiDisplayCurrentPageNum) && (!LiDisplayNeedToVerifyPowerState))
	{
		Serial.print(F("\nLiDisplay_checkForPendingPageUpdate -- LiDisplaySetPageNum != LiDisplayCurrentPageNum -- updating page"));
		LiDisplay_updatePage();
		return true;
	}
	else return false;
}

/////////////////////////////////////////////////////////////////////////////////////////

String LiDisplay_formatSpoofedValueDisplayStr(uint16_t spoofedValToFormat, bool use_decimal) {
	String formattedSpoofedVal = "";
	if (!use_decimal) { formattedSpoofedVal = String( String("(") + spoofedValToFormat + String(")") ); }
	else { formattedSpoofedVal = String( String("(") + String((spoofedValToFormat * 0.1),1) + String(")") ); }
	return formattedSpoofedVal;
}

/////////////////////////////////////////////////////////////////////////////////////////

uint16_t LiDisplay_calculateAvgCellVoltage() {
	#ifdef STACK_IS_48S
		return (LTC68042result_packVoltage_get() * 0.020833);
	#endif
	#ifdef STACK_IS_60S
		return (LTC68042result_packVoltage_get() * 0.016666);
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_SettingsPageValSwitch() {
	switch (LiDisplay_currentParamId)
	{
		case 0: LiDisplay_currentParamVal = CELL_VMAX_GRIDCHARGER; break;
		case 1: LiDisplay_currentParamVal = LIDISPLAY_CELL_COLOR_BIN_SIZE_COUNTS; break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updateElement() {
	// Each page has different elements on it, and a different quantity of elements.
	// Some elements need rapid updating, others rarely change.
	// During a frame, if we get to this function we will try to update 1 screen element, prioritizing elements that change frequently and skipping over ones that don't unless their value has changed.
	switch (LiDisplayCurrentPageNum)
	{
		case 0:
		case 7:
			switch (LiDisplayElementToUpdate)
			{
				// 7 elements update very frequently so we won't track their previous value
				case 0: LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t3", 0, String((LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps())*0.001)); break;
				case 1:
					if (LiDisplay_DrivingPageId == 0) {
						LiDisplay_calculateChrgAsstGaugeBars();
						LiDisplay_updateNumericVal(0, "p1", 2, LiDisplayChrgAsstPicId);
					} else {
						LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t23", 0, LiDisplay_formatSpoofedValueDisplayStr(BATTSCI_lastSpoofedSoC_deciPercent_get(), true)); // Spoofed SoC sent to MCM
					}
					break;
				case 2: LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t9", 0, (String((LTC68042result_hiCellVoltage_get() * 0.0001),3))); break;
				case 3: LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t6", 0, (String((LTC68042result_loCellVoltage_get() * 0.0001),3))); break;
				case 4: LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t13", 0, key_time); break;
				case 5: LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t14", 0, (String(((LTC68042result_hiCellVoltage_get() * 0.1) - (LTC68042result_loCellVoltage_get() * 0.1)),0)+"")); break;	// Delta mV
				case 6: LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t26", 0, String(adc_getLatestBatteryCurrent_amps())); break;	// Amps
				// The other elements update less frequently.  We will update 1 of them.
				// Priority is from least-likely to change to most-likely to change.
				case 7:
					LiDisplay_checkFirmwareExpiration();
					LiDisplay_calculateFanSpeedStr();
					LiDisplay_calculateSoCGaugeBars();

					if (energy_getAssist_Wh() > 0) { LiDisplay_energyWHAssist = energy_getAssist_Wh(); }
					if (energy_getRegen_Wh() > 0) { LiDisplay_energyWHRegen = energy_getRegen_Wh(); }

					if ((millis() - new_page_millis) < LIDISPLAY_MINIMUM_TIME_TO_UPDATE_AFTER_POWER_ON_MILLIS) { Serial.print(F("\nLiDisplay DP case 7 - Not enough time passed yet.  Breaking.")); break; }

					if (LiDisplay_heaterState_onScreen != gpio_isHeaterOnNow())
					{
						Serial.print(F("\nLiDisplay DP case 7 - LiDisplay_heaterState_onScreen = "));
						Serial.print(LiDisplay_heaterState_onScreen);
						if (gpio_isHeaterOnNow()) { LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t22", 0, (String("HEATER ON"))); }
						else if (!gpio_isHeaterOnNow()) {
							LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t22", 0, (String(" ")));
							Serial.print(F("\nLiDisplay DP case 7 - Setting T22 to blank")); }
						LiDisplay_heaterState_onScreen = gpio_isHeaterOnNow();
					}
					else if (LiDisplay_FanSpeed_onScreen != currentFanSpeed)
					{
						LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "b1", 0, (String(fanSpeedDisplay[currentFanSpeed])));
						LiDisplay_FanSpeed_onScreen = currentFanSpeed;
					}
					else if ((LiDisplay_DrivingPageId == 0) && (LiDisplay_SoCBars_onScreen != LiDisplaySoCBarCount))
					{
						LiDisplay_updateNumericVal(LiDisplay_DrivingPageId, "p0", 2, LiDisplaySoCBarCount);
						LiDisplay_SoCBars_onScreen = LiDisplaySoCBarCount;
					}
					else if (LiDisplay_SoC_onScreen != SoC_getBatteryStateNow_percent())
					{
						LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t1", 0, (String(SoC_getBatteryStateNow_percent()) + "%"));
						LiDisplay_SoC_onScreen = SoC_getBatteryStateNow_percent();
					}
					else if (LiDisplay_PackVoltageActual_onScreen != LTC68042result_packVoltage_get())
					{
						LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t4", 0, String(LTC68042result_packVoltage_get()));
						LiDisplay_PackVoltageActual_onScreen = LTC68042result_packVoltage_get();
					}
					else if (LiDisplay_PackVoltageSpoofed_onScreen != vPackSpoof_getSpoofedPackVoltage())
					{
						LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t24", 0, LiDisplay_formatSpoofedValueDisplayStr((uint16_t)vPackSpoof_getSpoofedPackVoltage(), false));
						LiDisplay_PackVoltageSpoofed_onScreen = vPackSpoof_getSpoofedPackVoltage();
					}
					else if ((LiDisplay_DrivingPageReqId == 7) && (LiDisplay_AvgCellVoltage_onScreen != LiDisplay_calculateAvgCellVoltage())) {
						//LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t28", 0, String((LiDisplay_calculateAvgCellVoltage() * 0.0001),3));
						LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t28", 0, String(LiDisplay_calculateAvgCellVoltage()));
						LiDisplay_AvgCellVoltage_onScreen = LiDisplay_calculateAvgCellVoltage();
					}
					else if ((LiDisplay_DrivingPageReqId == 7) && (LiDisplay_NS_hiCellNum_onScreen != LTC68042result_hiCellNum_get())) {
						LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t20", 0, String(LTC68042result_hiCellNum_get()));
						LiDisplay_NS_hiCellNum_onScreen = LTC68042result_hiCellNum_get();
					}
					else if ((LiDisplay_DrivingPageReqId == 7) && (LiDisplay_NS_loCellNum_onScreen != LTC68042result_loCellNum_get())) {
						LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t21", 0, String(LTC68042result_loCellNum_get()));
						LiDisplay_NS_loCellNum_onScreen = LTC68042result_loCellNum_get();
					}
					else if (LiDisplay_BattTemp_onScreen != temperature_battery_getLatest())
					{
						LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t11", 0, (String(temperature_battery_getLatest()) + char(176) + "C"));
						LiDisplay_BattTemp_onScreen = temperature_battery_getLatest();
					}
					else
					{
						if (LiDisplay_DrivingPageId == 0) {
							// Regular Driving Screen
							// Nothing else needed to update so we will update the chrg asst bar display again instead.
							LiDisplay_calculateChrgAsstGaugeBars();
							LiDisplay_updateNumericVal(LiDisplay_DrivingPageId, "p1", 2, LiDisplayChrgAsstPicId);
						}
						else {
							// Nerd Screen Only
							LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t17", 0, (String((LTC68042result_maxEverCellVoltage_get() * 0.0001),3))); // Peak cell V
							LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t19", 0, (String((LTC68042result_minEverCellVoltage_get() * 0.0001),3))); // Trough cell V
							LiDisplay_updateStringVal(LiDisplay_DrivingPageId, "t27", 0,
								String("KWh CHRG: ") + String(((energy_getTripMeterRegen_Wh() + LiDisplay_energyWHRegen) * 0.001),1) +
								"  ASST: " + String(((energy_getTripMeterAssist_Wh() + LiDisplay_energyWHAssist) * 0.001),1)
							);
						}
					}
				break;
				default: maxElementId = LIDISPLAY_DRIVING_PAGE_INTITIAL_MAX_ELEMENT_ID; LiDisplayElementToUpdate = 0; break;
			}
		break;

		case LIDISPLAY_SPLASH_PAGE_ID:
			if (LiDisplayElementToUpdate >= 5) { LiDisplayElementToUpdate = 0; }
			switch (LiDisplayElementToUpdate)
			{
				case 0: LiDisplay_updateStringVal(1, "t1", 0, String(FW_VERSION)); break;
				case 1: LiDisplay_updateStringVal(1, "t3", 0, String(REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS - eeprom_hoursSinceLastFirmwareUpdate_get())); break;
				case 2: LiDisplay_updateNumericVal(1, "p0", 2, LIDISPLAY_SPLASH_PIC); break;
				case 3: if (!LiDisplaySplashFromGridCharger) {
							LiDisplay_updateNumericVal(1, "t6", 4, NEXTION_WHT);	// T6 through T9 on splash page are initialized as black text.
							LiDisplay_updateNumericVal(1, "t8", 4, NEXTION_WHT);	// 65535 is Nextion code for white.
							// 2025 Dec 19 - NOTE_NATALYA: The + operator has to take a String on the left side, THEN there can be as many + const char* after as you like.
							LiDisplay_updateStringVal(1, "t6", 0, String("CHRG: ") + String((LiDisplay_energyWHRegen * 0.001),1) + "KWh");
							LiDisplay_updateStringVal(1, "t8", 0, String("ASST: ") + String((LiDisplay_energyWHAssist * 0.001),1) + "KWh");
						} else {
							LiDisplay_updateNumericVal(1, "t6", 4, NEXTION_WHT);
							LiDisplay_updateStringVal(1, "t6", 0, String("GRID: ") + String((LiDisplay_energyWHGridCharge * 0.001),1) + "KWh");
						} break;
				case 4: if (!LiDisplaySplashFromGridCharger) {
							LiDisplay_updateNumericVal(1, "t7", 4, NEXTION_WHT);
							LiDisplay_updateNumericVal(1, "t9", 4, NEXTION_WHT);
							LiDisplay_updateStringVal(1, "t7", 0, String("Trip: ") + String((energy_getTripMeterRegen_Wh() * 0.001),1) + "KWh");
							LiDisplay_updateStringVal(1, "t9", 0, String("Trip: ") + String((energy_getTripMeterAssist_Wh() * 0.001),1) + "KWh");
						} else {
							LiDisplay_updateStringVal(1, "t7", 0, String("Trip: ") + String((energy_getTripMeterGridCharge_Wh() * 0.001),1) + "KWh");
						}break;

				default: maxElementId = LIDISPLAY_SPLASH_PAGE_INTITIAL_MAX_ELEMENT_ID; break;
			}
		break;

		case LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID:
			// LiBCM should be beeping at the driver.  Check for key off or for grid charger unplugged.
			if (key_getSampledState() != KEYSTATE_ON) {
				if (gpio_isGridChargerPluggedInNow())
				{
					LiDisplay_setPageNumber(LIDISPLAY_GRIDCHARGE_PAGE_ID);
					LiDisplay_updatePage();
				}
				else
				{
					LiDisplaySplashPending = true;
					LiDisplay_setPageNumber(LIDISPLAY_SPLASH_PAGE_ID);
					LiDisplay_updatePage();
				}
			}
			break;
		case LIDISPLAY_GRIDCHARGE_PAGE_ID:

			LiDisplay_calculateGCTimeStr(false);
			switch (LiDisplayElementToUpdate)
			{
				// 4 elements update very frequently so we won't track their previous value
				case 0:
					// Store accumulated Grid Charging Wh
					if (energy_getGridCharger_Wh() > 0) { LiDisplay_energyWHGridCharge = energy_getGridCharger_Wh(); }
					// This element doesn't update very frequently, but its priority is high because we want to notify the user the instant it does update.
					if (gpio_isGridChargerChargingNow() && !cellBalance_areCellsBalancing()) { LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t7", 0,     "CHARGING"); }
					else if (gpio_isGridChargerChargingNow() && (cellBalance_areCellsBalancing())) { LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t7", 0, "CHRG + BLNC"); }
					else if ((!gpio_isGridChargerChargingNow()) && (cellBalance_areCellsBalancing())) { LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t7", 0, "BALANCING"); }
					else { LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t7", 0, "IDLE"); }

				break;
				case 1: LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t3", 0, String(LiDisplay_AvgCellVoltage * 0.0001,3)); break;
				case 2: LiDisplay_updateNextCellValue();    break;
				case 3: LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t8", 0, String(gc_time));  break;
				case 4:
					LiDisplay_calculateFanSpeedStr();
					if (!gc_sixty_s_fomoco_e_block_enabled && (MAX_CELL_INDEX == 59))
					{
						LiDisplay_updateNumericVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t16", 3, NEXTION_GRIDCCHARGE_60S_T16_CLR); // E block label will be missing on a 60S 47Ah pack display if we don't run this once.
						gc_sixty_s_fomoco_e_block_enabled = true;
					}
					else if (LiDisplay_FanSpeed_onScreen != currentFanSpeed)
					{
						LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "b1", 0, (String(fanSpeedDisplay[currentFanSpeed])));
						LiDisplay_FanSpeed_onScreen = currentFanSpeed;
					}
					else if (LiDisplay_SoC_onScreen != SoC_getBatteryStateNow_percent())
					{
						LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t1", 0, (String(SoC_getBatteryStateNow_percent()) + "%"));
						LiDisplay_SoC_onScreen = SoC_getBatteryStateNow_percent();
					}
					else if (LiDisplay_BattTemp_onScreen != temperature_battery_getLatest())
					{
						LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t19", 0, (String(temperature_battery_getLatest()) + "C"));
						LiDisplay_BattTemp_onScreen = temperature_battery_getLatest();
					}
					else if (LiDisplay_PackVoltageActual_onScreen != LTC68042result_packVoltage_get())
					{
						LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t4", 0, String(LTC68042result_packVoltage_get()));
						LiDisplay_PackVoltageActual_onScreen = LTC68042result_packVoltage_get();
					}
					else if (LiDisplay_heaterState_onScreen != gpio_isHeaterOnNow())
					{
						if (gpio_isHeaterOnNow()) { LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t22", 0, (String("HEATER ON"))); }
						else if (!gpio_isHeaterOnNow()) { LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t22", 0, (String(" "))); }
						LiDisplay_heaterState_onScreen = gpio_isHeaterOnNow();
					}
					else LiDisplay_updateNextCellValue();     break;

				case 5: LiDisplay_updateNextCellValue();    break;
				case 6: LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t26", 0, String(adc_getLatestBatteryCurrent_amps())); break;
				case 7: maxElementId = (LIDISPLAY_GRIDCHARGE_PAGE_INTITIAL_MAX_ELEMENT_ID - 1); LiDisplay_updateStringVal(LIDISPLAY_GRIDCHARGE_PAGE_ID, "t10", 0, (String(gc_begin_soc_str))); break;	// This should run only once per charge cycle.  It's going to display what the SoC was when the grid charger was plugged in.
				default: maxElementId = LIDISPLAY_GRIDCHARGE_PAGE_INTITIAL_MAX_ELEMENT_ID;	break;
			}
		break;
		case LIDISPLAY_SETTINGS_PAGE_ID: // Placeholder for now (19 June 2025)
			if (energy_getAssist_Wh() > 0) { LiDisplay_energyWHAssist = energy_getAssist_Wh(); }
			if (energy_getRegen_Wh() > 0) { LiDisplay_energyWHRegen = energy_getRegen_Wh(); }
			if (energy_getGridCharger_Wh() > 0) { LiDisplay_energyWHGridCharge = energy_getGridCharger_Wh(); }

			LiDisplay_SettingsPageValSwitch();
			if (LiDisplay_paramName_onScreen != editableParamMap[LiDisplay_currentParamId])
			{
				LiDisplay_updateStringVal(LIDISPLAY_SETTINGS_PAGE_ID, "t3", 0, editableParamMap[LiDisplay_currentParamId]);
				LiDisplay_paramName_onScreen = editableParamMap[LiDisplay_currentParamId];
			} else if (LiDisplay_paramVal_onScreen != LiDisplay_currentParamVal) {
				LiDisplay_updateStringVal(LIDISPLAY_SETTINGS_PAGE_ID, "t4", 0, String(LiDisplay_currentParamVal));
				LiDisplay_paramVal_onScreen = LiDisplay_currentParamVal;
			} else if (Lidisplay_paramDesc_onScreen != editableParamDescriptions[LiDisplay_currentParamId]) {
				LiDisplay_updateStringVal(LIDISPLAY_SETTINGS_PAGE_ID, "t5", 0, editableParamDescriptions[LiDisplay_currentParamId]);
				Lidisplay_paramDesc_onScreen = editableParamDescriptions[LiDisplay_currentParamId];
			} else if (LiDisplay_currentGlobalNumVal != LiDisplay_currentParamVal) {
				LiDisplay_updateGlobalObjectVal("n0", 1, String(LiDisplay_currentParamVal));
				LiDisplay_currentGlobalNumVal = LiDisplay_currentParamVal;
			} else {
				// Grid Charger Litre and GGE display require division, but they only need to be updated on first screen load if car is being driven
				uint16_t WHGridCharge_onScreen = 0;
				WHGridCharge_onScreen = (energy_getTripMeterGridCharge_Wh() + LiDisplay_energyWHGridCharge);
				if (LiDisplay_lastWHGridCharge_onScreen != WHGridCharge_onScreen) {
					// We should only get here one time when the settings page is loaded if the car is driving
					// This will update every so often if the grid charger is plugged in and charging
					// Canada Natural Resources dept definition is 8.9 KWh / litre gasoline
					// US DoE KWh to US Gallon Gasoline Equivalent is 33.4 KWh / US Gallon gasoline
					LiDisplay_updateStringVal(LIDISPLAY_SETTINGS_PAGE_ID, "t7", 0,
						String("GRID Litre Equiv: ") + String((WHGridCharge_onScreen / 8900.0),1) +
						"  GGE: " + String((WHGridCharge_onScreen / 33400.0),1)
					);
					LiDisplay_lastWHGridCharge_onScreen = WHGridCharge_onScreen;
				}

				// Next to "CLEAR TRIP" button we will show current trip KWh totals all in 1 text box
				// We will include the current drive or grid charge cycle in these totals even though they're not saved to the trip yet.
				LiDisplay_updateStringVal(LIDISPLAY_SETTINGS_PAGE_ID, "t6", 0,
					String("KWh CHRG ") + String(((energy_getTripMeterRegen_Wh() + LiDisplay_energyWHRegen) * 0.001),1) +
					"  ASST " + String(((energy_getTripMeterAssist_Wh() + LiDisplay_energyWHAssist) * 0.001),1) +
					"  GRID " + String((WHGridCharge_onScreen * 0.001),1)
				);
			}
		break;
		default : break;
	}

	LiDisplayElementToUpdate += 1;

	if (LiDisplayElementToUpdate > maxElementId) { LiDisplayElementToUpdate = 0; }
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_handler(void)
{
	#ifdef LIDISPLAY_CONNECTED
        static uint32_t millis_previous = 0;

		if ((LiDisplay_BuzzerRequested) && ((millis - LiDisplay_buzzerRequestMS)) > 200) {
			buzzer_requestTone(BUZZER_REQUESTOR_USER, BUZZER_OFF);
			LiDisplay_BuzzerRequested = false;
		}

		LiDisplay_handleKeyOrGCStateChange();
		if (LiDisplayNeedToVerifyPowerState) LiDisplay_enforceCorrectPowerState();

		if (!gpio_HMIStateNow()) { return; } // LiDisplay is off, so we can exit this function.

		LiDisplay_userInputHandler();	// Check if user has pressed a button on the LiDisplay screen.
		LiDisplay_calculateCorrectPage();

		if ((millis() - hmi_power_millis) < LIDISPLAY_MINIMUM_TIME_TO_UPDATE_AFTER_POWER_ON_MILLIS) { return; } // ensure at least 400ms have passed since screen turned on.

        if (LiDisplayOnGridChargerConnected || LiDisplayOnKeyOnWithNerdScreenEnabled)
		{
			// When powered on the Nextion automatically always displays page 0 which is the normal driving screen
			// If they plugged in the grid charger, OR if they want to use the nerd screen we need to wait about 400ms before we tell the Nextion to switch to the correct screen
            LiDisplay_updatePage();
			if (LiDisplayOnGridChargerConnected) { LiDisplayOnGridChargerConnected = false; }
			if (LiDisplayOnKeyOnWithNerdScreenEnabled) { LiDisplayOnKeyOnWithNerdScreenEnabled = false; }
			return;
        }


        if ((millis() - millis_previous) > LIDISPLAY_UPDATE_RATE_MILLIS)
        {
            millis_previous = millis();

            if (LiDisplay_checkForPendingPageUpdate()) { return; } // If the page had to be changed then we are not updating any elements on it this frame.
            if (key_getSampledState() == KEYSTATE_ON) { LiDisplay_calculateKeyTimeStr(false); }  // Increment key time here in case driver switches to settings page

			LiDisplay_updateElement();	// Update 1 element on the screen.
        }

    #endif
}


/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_keyOn(void)
{
	#ifdef LIDISPLAY_USE_NERD_SCREEN
		// Nerd Screen driving page is named page6
		// Nerd Screen driving page index is 7
		// This means to switch to the page we need to send 7, but to change elements on the page, they need a 6

		LiDisplay_DrivingPageId = 6;
		LiDisplay_DrivingPageReqId = 7;
		LiDisplayOnKeyOnWithNerdScreenEnabled = true;
	#endif
    #ifdef LIDISPLAY_CONNECTED
        Serial.print(F("\nLiDisplay_keyOn"));
        Serial.print(F("\nLiDisplay HMI Power On"));
        gpio_turnHMI_on();
		LiDisplay_brightness = 100;
        LiDisplay_serialBegin();
        hmi_power_millis = millis();
        key_time_begin_ms = millis();
        LiDisplay_calculateKeyTimeStr(true);
		LiDisplayCurrentPageNum = 100;	// When the Nextion is turned on set this to a nonsensical number to initialize it.
        LiDisplaySetPageNum = LiDisplay_DrivingPageReqId;
		LiDisplaySplashPending = false;
		LiDisplayPowerOffPending = false;
		LiDisplay_energyWHAssist = 0;
		LiDisplay_energyWHRegen = 0;

		LiDisplay_resetDrivingPageVariables();

    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_keyOff(void)
{
    #ifdef LIDISPLAY_CONNECTED
        // Check if gpio HMI was already off
        Serial.print(F("\nLiDisplay_keyOff:  gpio_HMIStateNow = "));
        Serial.print(String(gpio_HMIStateNow()));
        LiDisplaySettingsPageRequested = false;

        if (gpio_HMIStateNow())
		{
            if (!gpio_isGridChargerPluggedInNow())
			{
                hmi_power_millis = millis();
                LiDisplaySplashPending = true;
                LiDisplayPowerOffPending = true;
            }
        }
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_gridChargerPluggedIn(void)
{
    #ifdef LIDISPLAY_CONNECTED
        Serial.print(F("\nLiDisplay_gridChargerPluggedIn"));
        Serial.print(F("\nLiDisplay HMI Power On"));
        Serial.print(F("\ngpio_HMIStateNow() = "));
        Serial.print(String(gpio_HMIStateNow()));
        if (!gpio_HMIStateNow())
		{
            gpio_turnHMI_on();
			LiDisplay_brightness = 100;
            LiDisplay_serialBegin();
            hmi_power_millis = millis();
        }

		LiDisplay_energyWHGridCharge = 0;

		LiDisplay_resetGridChargerPageVariables();
		LiDisplay_calculateGCTimeStr(true);				// Reset GC charge time clock

        LiDisplayOnGridChargerConnected = true;
        gc_chg_time_begin_millis = millis();

        gc_begin_soc_str = (String(SoC_getBatteryStateNow_percent()) + "%");
        LiDisplay_SoC_onScreen = 100;
        LiDisplay_FanSpeed_onScreen = 100;
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_gridChargerUnplugged(void)
{
    #ifdef LIDISPLAY_CONNECTED
        Serial.print(F("\nLiDisplay_gridChargerUnplugged"));
        gc_connected_millis_most_recent_diff = 0;
        // Check if gpio HMI was already off
        if (gpio_HMIStateNow())
		{
            if (key_getSampledState() == KEYSTATE_OFF)
			{
                hmi_power_millis = millis();
				LiDisplaySplashFromGridCharger = true;
                LiDisplaySplashPending = true;
                LiDisplayPowerOffPending = true;
            }
			else { LiDisplay_keyOn(); }
        }
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_setPageNumber(uint8_t page)
{
    #ifdef LIDISPLAY_CONNECTED
        LiDisplaySetPageNum = page;
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_writeInstructionTerminationBytes()
{
	LiDisplay_writeByte(0xFF); LiDisplay_writeByte(0xFF); LiDisplay_writeByte(0xFF);
}

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// All Abstracted Serial.1 functions are below.
// Serial1 should not appear anywhere above this section.
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_serialBegin() {
	#ifdef LIDISPLAY_CONNECTED
		Serial1.begin(57600,SERIAL_8N1);    // 2023 OCT -- Credit to IC User AfterEffect for finding that SERIAL_8N1 fixes comms issues with the Nextion
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t LiDisplay_bytesAvailableForWrite()
{
    #ifdef LIDISPLAY_CONNECTED
        return Serial1.availableForWrite();
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

String LiDisplay_printString(String data)
{
    #ifdef LIDISPLAY_CONNECTED
        Serial1.print(data);
        return data;
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t LiDisplay_writeByte(uint8_t data)
{
    #ifdef LIDISPLAY_CONNECTED
        Serial1.write(data);
        return data;
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t LiDisplay_readByte()
{
    #ifdef LIDISPLAY_CONNECTED
        return Serial1.read();
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t LiDisplay_bytesAvailableToRead()
{
    #ifdef LIDISPLAY_CONNECTED
        return Serial1.available();
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////
