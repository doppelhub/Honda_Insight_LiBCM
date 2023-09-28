//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LiDisplay (HMI) Serial Functions

#include "libcm.h"

#define LIDISPLAY_DRIVING_PAGE_ID 0
#define LIDISPLAY_SPLASH_PAGE_ID 1
#define LIDISPLAY_GRIDCHARGE_PAGE_ID 3
#define LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID 2
#define LIDISPLAY_SETTINGS_PAGE_ID 4
// #define CMD 0x23								// May be used when reading from nextion.  0x23 = "#"
#define LIDISPLAY_UPDATE_RATE_MILLIS 40		// One element is updated each time

#ifdef STACK_IS_48S
	#define MAX_CELL_INDEX 47
#elif defined STACK_IS_60S
	#define MAX_CELL_INDEX 59
#endif

uint8_t LiDisplayElementToUpdate = 0;
uint8_t LiDisplayCurrentPageNum = 0;
uint8_t LiDisplaySetPageNum = 0;
uint8_t LiDisplaySoCBarCount = 0;
uint8_t LiDisplayChrgAsstPicId = 22;
uint8_t LiDisplayWaitingForCommand = 0;

// Initializing to 100 (absurd number for all 4 variables) so that on first run they will be updated on screen
static uint8_t  LiDisplayPackVoltageActual_onScreen = 100;
static uint8_t  LiDisplaySoC_onScreen = 100;
static uint8_t  LiDisplayFanSpeed_onScreen = 100;
static uint8_t  LiDisplaySoCBars_onScreen = 100;
static uint8_t	LiDisplayTemp_onScreen = 0;
static uint16_t	LiDisplayAverageCellVoltage = 0;
static uint8_t maxElementId = 8;

bool LiDisplaySplashPending = false;
bool LiDisplayPowerOffPending = false;
bool LiDisplayOnGridChargerConnected = false;
bool LiDisplaySettingsPageRequested = false;

static uint32_t hmi_power_millis = 0;
static uint32_t gc_connected_millis = 0;
static uint32_t gc_connected_millis_most_recent_diff = 0;
static uint16_t gc_connected_seconds = 0;
static uint8_t gc_connected_minutes = 0;
static uint8_t gc_connected_hours = 0;

static String gc_begin_soc_str = "0%";
static String gc_time = "00:00:00";
static uint8_t currentFanSpeed = 0;

bool gc_sixty_s_fomoco_e_block_enabled = false;

const String attrMap[4] = {
	"txt",	// text value
	"val",	// numerical value
	"pic",	// picture id
	"bco"	// background colour
};
const String fanSpeedDisplay[3] = {
	"FAN OFF", "FAN LOW", "FAN HIGH"
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_begin(void)
{
	#ifdef LIDISPLAY_CONNECTED
 		Serial.print(F("\nLiDisplay BEGIN"));
		//Serial1.begin(9600,SERIAL_8E1);
		//Serial1.begin(38400,SERIAL_8E1); // Only turn on when display is on.

		#ifdef BATTERY_TYPE_47AhFoMoCo
			#undef LIDISPLAY_GRIDCHARGE_PAGE_ID
			#define LIDISPLAY_GRIDCHARGE_PAGE_ID 5
		#endif

		LiDisplayElementToUpdate = 0;

		LiDisplaySplashPending = false;
		LiDisplayPowerOffPending = false;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateCorrectPage() {
	if (key_getSampledState() == KEYSTATE_ON) {
		// Key is On
		if (gpio_isGridChargerPluggedInNow()) { LiDisplaySetPageNum = LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID; } // Grid Charger Plugged In -- Display Warning Page
		else if (LiDisplaySplashPending) { LiDisplaySetPageNum = LIDISPLAY_SPLASH_PAGE_ID; }
		else if (LiDisplaySettingsPageRequested) { LiDisplaySetPageNum = LIDISPLAY_SETTINGS_PAGE_ID; }
		else { LiDisplaySetPageNum = LIDISPLAY_DRIVING_PAGE_ID; }
	} else {
		// Key is Off
		if (gpio_isGridChargerPluggedInNow()) { LiDisplaySetPageNum = LIDISPLAY_GRIDCHARGE_PAGE_ID; } // Grid Charger Plugged In -- Display GC Page
		else if (LiDisplaySplashPending) { LiDisplaySetPageNum = LIDISPLAY_SPLASH_PAGE_ID; }
		else if (LiDisplaySettingsPageRequested) { LiDisplaySetPageNum = LIDISPLAY_SETTINGS_PAGE_ID; }
	}
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updateNumericVal(uint8_t page, String elementName, uint8_t elementAttrIndex, String value) {
	#ifdef LIDISPLAY_CONNECTED
		static String LiDisplay_Number_Str;

		LiDisplay_Number_Str = "page" + String(page) + "." + String(elementName) + "." + attrMap[elementAttrIndex] + "=" + value;

		Serial1.print(LiDisplay_Number_Str);
		Serial1.write(0xFF);
		Serial1.write(0xFF);
		Serial1.write(0xFF);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updateStringVal(uint8_t page, String elementName, uint8_t elementAttrIndex, String value) {
	#ifdef LIDISPLAY_CONNECTED
		static String LiDisplay_String_Str;

		LiDisplay_String_Str = "page" + String(page) + "." + String(elementName) + "." + attrMap[elementAttrIndex] + "=" + String('"') + value + String('"');

		Serial1.print(LiDisplay_String_Str);
		Serial1.write(0xFF);
		Serial1.write(0xFF);
		Serial1.write(0xFF);
	#endif
}

LiDisplay_updateNextCellValue() {
	static String LiDisplay_Color_Str;
	static uint8_t cellToUpdate = 0;
	static uint8_t ic_index = 0;
	static uint8_t ic_cell_num = 0;
	static uint16_t cell_avg_voltage = 0;
	static String cell_color_number = "2016";
	static int cell_voltage_diff_from_avg = 0;
	static int temp_cell_voltage = 0;


	if (cellToUpdate > MAX_CELL_INDEX) cellToUpdate = 0;

	// NM To Do: Cells indexed 18 to 35 are the central block in the IMA battery.
	// We need to add a config variable for 18S+ or 18S- otherwise they may display out of order left-to-right for some installations
	// Right now (09 Feb 2023) it starts with cell index 18 on the left of the display, corresponding to the front of the vehicle.
	if (cellToUpdate <= 11) { ic_index = 0; ic_cell_num = (cellToUpdate); }
	else if (cellToUpdate <= 23) { ic_index = 1; ic_cell_num = (cellToUpdate - 12); }
	else if (cellToUpdate <= 35) { ic_index = 2; ic_cell_num = (cellToUpdate - 24); }
	else if (cellToUpdate <= 47) { ic_index = 3; ic_cell_num = (cellToUpdate - 36); }
	else if (cellToUpdate <= 59) { ic_index = 4; ic_cell_num = (cellToUpdate - 48); }

	// 09 Feb 2023 -- cell_avg_voltage is a crude approximation of the centre of the voltage range.  Ideally this would be replaced with the median cell voltage.
	LiDisplayAverageCellVoltage = ((LTC68042result_hiCellVoltage_get() - LTC68042result_loCellVoltage_get()) * 0.5);

	cell_avg_voltage = (LiDisplayAverageCellVoltage + LTC68042result_loCellVoltage_get());
	LiDisplayAverageCellVoltage = (cell_avg_voltage); // TODO_NATALYA - get rid of cell_avg_voltage
	temp_cell_voltage = LTC68042result_specificCellVoltage_get(ic_index, ic_cell_num);

	cell_voltage_diff_from_avg = cell_avg_voltage - temp_cell_voltage;

	// 04 Feb 2023 -- CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE is 32 so we are centering on -16 to +16, and then using increments of 32 to determine bar colour.
	if (cell_voltage_diff_from_avg >= 80) { cell_color_number = "63488"; }			// 63488 = Red
	else if (cell_voltage_diff_from_avg >= 48) { cell_color_number = "64480"; }	// 64480 = Orange
	else if (cell_voltage_diff_from_avg >= 16) { cell_color_number = "65504"; }	// 65504 = Yellow
	else if (cell_voltage_diff_from_avg >= -16) { cell_color_number = "2016"; }	// 2016 = Green
	else if (cell_voltage_diff_from_avg >= -48) { cell_color_number = "2047"; }	// 2047 = Cyan
	else { cell_color_number = "31"; }								// 31 = Blue
	// We could add a darker blue on this line if we wanted to

	LiDisplay_Color_Str = "page" + String(LIDISPLAY_GRIDCHARGE_PAGE_ID) + ".j" + String(cellToUpdate) + ".pco" + "=" + cell_color_number;

	Serial1.print(LiDisplay_Color_Str);
	Serial1.write(0xFF);
	Serial1.write(0xFF);
	Serial1.write(0xFF);

	cellToUpdate += 1;
/*
	Serial.print(F("\n"));
	Serial.print("LiDisplay ColorStr ");
	Serial.print(LiDisplay_Color_Str);
	Serial.print(F("\n"));
	Serial.print("LiDisplay cell_avg_voltage ");
	Serial.print(cell_avg_voltage);
	Serial.print("  V:");
	Serial.print(String(temp_cell_voltage));
	Serial.print("  IC:");
	Serial.print(ic_index);
	Serial.print("  Cell:");
	Serial.print(ic_cell_num);
	Serial.print("  cell_voltage_diff_from_avg:");
	Serial.print(cell_voltage_diff_from_avg);
*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateGCTimeStr() {
	static String gc_sec_prefix = "0";
	static String gc_min_prefix = "0";
	static String gc_hour_prefix = "0";
	static uint16_t temp_gc_millis = 0;
	static bool gc_was_paused = false;

	// Increment time only while charging
	if (gpio_isGridChargerChargingNow()) {
		if (gc_was_paused) {
			gc_connected_millis = (millis() - gc_connected_millis_most_recent_diff);
			gc_was_paused = false;
		}

		gc_connected_millis_most_recent_diff = (millis() - gc_connected_millis);

		// 09 Feb 2023 -- NM To Do:  Get rid of modulo and division
		// Note to JTS: LiDisplay_calculateGCTimeStr() is only run while the grid charger is plugged in AND key is off.
		// I'd like to address this issue later if possible because it doesn't affect key-on cycle or driving cycle of LiBCM.
		gc_connected_hours = (gc_connected_millis_most_recent_diff / 3600000);
		gc_connected_minutes = (gc_connected_millis_most_recent_diff / 60000) % 60;
		gc_connected_seconds = (gc_connected_millis_most_recent_diff / 1000) % 60;

		if (gc_connected_seconds > 9) { gc_sec_prefix = ""; }
		else gc_sec_prefix = String(0);
		if (gc_connected_minutes > 9) { gc_min_prefix = ""; }
		else gc_min_prefix = String(0);
		if (gc_connected_hours > 9) { gc_hour_prefix = ""; }
		else gc_hour_prefix = String(0);

		//gc_time = String("M:") + String(gc_connected_minutes) + String(" S:") + String(gc_connected_seconds);
		gc_time = String(gc_hour_prefix) + String(gc_connected_hours) + String(":") + String(gc_min_prefix) + String(gc_connected_minutes) + String(":") + String(gc_sec_prefix) + String(gc_connected_seconds);
	} else {
		// Still plugged in but not charging
		gc_was_paused = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	else if (packEHP <= -9) { LiDisplayChrgAsstPicId = 49; }
	else if (packEHP <= -8) { LiDisplayChrgAsstPicId = 48; }
	else if (packEHP <= -7) { LiDisplayChrgAsstPicId = 47; }
	else if (packEHP <= -6) { LiDisplayChrgAsstPicId = 46; }
	else if (packEHP <= -5) { LiDisplayChrgAsstPicId = 45; }
	else if (packEHP <= -4) { LiDisplayChrgAsstPicId = 44; }
	else if (packEHP <= -3) { LiDisplayChrgAsstPicId = 43; }
	else if (packEHP <= -2) { LiDisplayChrgAsstPicId = 42; }
	else if (packEHP <= -1) { LiDisplayChrgAsstPicId = 41; }
	else if (packEHP <= 0) { LiDisplayChrgAsstPicId = 22; }
	else if (packEHP <= 1) { LiDisplayChrgAsstPicId = 23; }
	else if (packEHP <= 2) { LiDisplayChrgAsstPicId = 24; }
	else if (packEHP <= 3) { LiDisplayChrgAsstPicId = 25; }
	else if (packEHP <= 4) { LiDisplayChrgAsstPicId = 26; }
	else if (packEHP <= 5) { LiDisplayChrgAsstPicId = 27; }
	else if (packEHP <= 6) { LiDisplayChrgAsstPicId = 28; }
	else if (packEHP <= 7) { LiDisplayChrgAsstPicId = 29; }
	else if (packEHP <= 8) { LiDisplayChrgAsstPicId = 30; }
	else if (packEHP <= 9) { LiDisplayChrgAsstPicId = 31; }
	else if (packEHP <= 10) { LiDisplayChrgAsstPicId = 32; }
	else if (packEHP <= 11) { LiDisplayChrgAsstPicId = 33; }
	else if (packEHP <= 12) { LiDisplayChrgAsstPicId = 34; }
	else if (packEHP <= 13) { LiDisplayChrgAsstPicId = 35; }
	else if (packEHP <= 14) { LiDisplayChrgAsstPicId = 36; }
	else if (packEHP <= 15) { LiDisplayChrgAsstPicId = 37; }
	else if (packEHP <= 16) { LiDisplayChrgAsstPicId = 38; }
	else if (packEHP <= 17) { LiDisplayChrgAsstPicId = 39; }
	else { LiDisplayChrgAsstPicId = 40; }
	// 2022 Sept 07 -- NM To Do: The assist display can only show up to 18 HP of assist, but LiBCM can put out over 20 HP
	// Need to edit the HMI file to have a graphical display of those extra HP, probably by further highlighting some of the assist bars
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
  else if (SoC_getBatteryStateNow_percent() >= 43) { LiDisplaySoCBarCount = 9; }
  else if (SoC_getBatteryStateNow_percent() >= 40) { LiDisplaySoCBarCount = 8; }
  else if (SoC_getBatteryStateNow_percent() >= 37) { LiDisplaySoCBarCount = 7; }
  else if (SoC_getBatteryStateNow_percent() >= 34) { LiDisplaySoCBarCount = 6; }
  else if (SoC_getBatteryStateNow_percent() >= 34) { LiDisplaySoCBarCount = 5; }
  else if (SoC_getBatteryStateNow_percent() >= 31) { LiDisplaySoCBarCount = 4; }
  else if (SoC_getBatteryStateNow_percent() >= 28) { LiDisplaySoCBarCount = 3; }
  else if (SoC_getBatteryStateNow_percent() >= 25) { LiDisplaySoCBarCount = 2; }
  else if (SoC_getBatteryStateNow_percent() >= 22) { LiDisplaySoCBarCount = 1; }
  else { LiDisplaySoCBarCount = 0; }
  return;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calclateFanSpeedStr() {
	if (fan_getSpeed_now() == FAN_HIGH) {
		currentFanSpeed = 2;
	} else if (fan_getSpeed_now() == FAN_LOW) {
		currentFanSpeed = 1;
	} else {
		currentFanSpeed = 0;
	}
	/*
	Serial.print(F("\n"));
	Serial.print("LiDisplay currentFanSpeed: ");
	Serial.print(currentFanSpeed);
	Serial.print("   fan speed now: ");
	Serial.print(String(fan_getSpeed_now()));
	*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updatePage() {
	#ifdef LIDISPLAY_CONNECTED
		static String LiDisplay_Page_Str;
		LiDisplay_Page_Str = "page " + String(LiDisplaySetPageNum);

		Serial.print(F("\n"));
		Serial.print("LiDisplay_updatePage ");
		Serial.print(LiDisplay_Page_Str);

		Serial1.print(LiDisplay_Page_Str);

		Serial1.write(0xFF);
		Serial1.write(0xFF);
		Serial1.write(0xFF);

		LiDisplayCurrentPageNum = LiDisplaySetPageNum;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_updateDebugTextBox(String raw_data_string) {
	#ifdef LIDISPLAY_DEBUG_ENABLED
		LiDisplay_updateStringVal(0, "t12", 0, raw_data_string);	// T12 is a text box on the bottom of the driving page screen.
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_handler(void)
{
	#ifdef LIDISPLAY_CONNECTED

		static uint32_t millis_previous = 0;
		static uint32_t splash_millis = 0;
		static uint32_t hmi_read_millis = 0;

		// Below stuff doesn't work properly yet.
		static String cmd_str = "";

		// 27 Sept 2023 -- This is the beginning of LiBCM trying to receive commands from the Nextion.  It doesn't work 100% yet.

		if (LiDisplayWaitingForCommand > 0) {
			LiDisplayWaitingForCommand -= 1;
		}

		if (LiDisplayWaitingForCommand == 1) {
			LiDisplayWaitingForCommand -= 1;

			while (Serial1.available()) {
				cmd_str += char(Serial1.read());
			};

			LiDisplay_updateDebugTextBox(String(cmd_str));

			Serial.print(F("\n"));
			Serial.print(String(cmd_str));

			cmd_str = "";
		}

		if (Serial1.available() && (LiDisplayWaitingForCommand == 0)) {
			LiDisplayWaitingForCommand = 100;	// 100 ms cooldown
		}
		// Above stuff doesn't work properly yet.


		LiDisplay_calculateCorrectPage();


		// TODO_NATALYA: Probably should put this into a new function -- LiDisplay_calculateCorrectPowerState()
		if (LiDisplayPowerOffPending) {
			if (LiDisplayOnGridChargerConnected) {
				// GC plugged in before relay in IPU compartment clicked off, or just after, but before splash screen complete.
				Serial.print(F("\nLiDisplayPowerOffPending CANCELLED because grid charger was plugged in."));
				LiDisplaySplashPending = false;
				LiDisplayPowerOffPending = false;
			} else if ((millis() - hmi_power_millis) < 100) { // ensure at least 100ms have passed since last refresh loop
				return;
			} else if (LiDisplaySplashPending) {
				LiDisplay_updatePage();
				LiDisplaySplashPending = false;
				splash_millis = millis();
				return;
			} else if ((millis() - splash_millis) == (LIDISPLAY_UPDATE_RATE_MILLIS)) {
				LiDisplay_updateNumericVal(1, "n0", 0, String(REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS - eeprom_uptimeStoredInEEPROM_hours_get()));
				return;
			} else if ((millis() - splash_millis) > (LIDISPLAY_SPLASH_PAGE_MS)) {
				gpio_turnHMI_off();
				LiDisplayPowerOffPending = false;
				Serial1.end(); // Turn Serial1 off to empty buffer.
				return;
			}
		} else if (LiDisplaySplashPending) {
			if (millis() - hmi_power_millis < 100) {
				return;
			} else {
				LiDisplaySplashPending = false;
				LiDisplay_updatePage();
				splash_millis = millis();
				return;
			}
		}

		if (LiDisplayOnGridChargerConnected) {
			if (!gpio_HMIStateNow()) {
				// If screen power is off we need to turn on LiDisplay
				gpio_turnHMI_on();
				Serial1.begin(38400,SERIAL_8E1);
				hmi_power_millis = millis();
				return;
			}
			if ((millis() - hmi_power_millis) < 400) { // ensure at least 400ms have passed since screen turned on.  Note Sept 2023 -- Smaller values like 100ms were not enough
				return;
			} else {
				// Ready to show GC page, but first check if key is on
				// 2022 Sept 07 -- TODO_NATALYA: We may be able to delete this now because of LiDisplay_calculateCorrectPage()
				if (key_getSampledState() == KEYSTATE_ON) {
					LiDisplay_setPageNumber(LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID);
				} else {
					LiDisplay_setPageNumber(LIDISPLAY_GRIDCHARGE_PAGE_ID);
				}
				LiDisplay_updatePage();
				LiDisplayOnGridChargerConnected = false;
				return;
			}
		}

		if(millis() - millis_previous > LIDISPLAY_UPDATE_RATE_MILLIS)
		{
			millis_previous = millis();

			if (LiDisplaySetPageNum != LiDisplayCurrentPageNum) {
				LiDisplay_updatePage();
				if (LiDisplaySetPageNum == LIDISPLAY_SPLASH_PAGE_ID) {
					splash_millis = millis();
				}
				return;
			} else if (LiDisplayCurrentPageNum == LIDISPLAY_SPLASH_PAGE_ID) {	// Splash page loop
				if (millis() - splash_millis > LIDISPLAY_SPLASH_PAGE_MS) {
					// Splash page has shown long enough.  Switch to main driving screen.
					LiDisplay_setPageNumber(LIDISPLAY_DRIVING_PAGE_ID);;
				} else {
					if (LiDisplayElementToUpdate > 2) { LiDisplayElementToUpdate = 0; }
					switch(LiDisplayElementToUpdate)
					{
						case 0: LiDisplay_updateStringVal(1, "t1", 0, String(FW_VERSION)); break;
						case 1: LiDisplay_updateStringVal(1, "t3", 0, String(REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS - eeprom_uptimeStoredInEEPROM_hours_get())); break;
					}
					LiDisplayElementToUpdate += 1;
				}
			} else {	// Main page loop
				if(!gpio_HMIStateNow()) return; //LiDisplay is off, so we don't need to run the loop.

				switch(LiDisplayCurrentPageNum) {
					case LIDISPLAY_DRIVING_PAGE_ID:

						maxElementId = 4;
						switch(LiDisplayElementToUpdate)
						{
							// 4 elements update very frequently so we won't track their previous value
							case 0: LiDisplay_updateStringVal(0, "t3", 0, String((LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps())/1000)); break;
							case 1: LiDisplay_calculateChrgAsstGaugeBars(); LiDisplay_updateNumericVal(0, "p1", 2, String(LiDisplayChrgAsstPicId));	break;
							case 2: LiDisplay_updateStringVal(0, "t9", 0, (String((LTC68042result_hiCellVoltage_get() * 0.0001),3))); break;
							case 3: LiDisplay_updateStringVal(0, "t6", 0, (String((LTC68042result_loCellVoltage_get() * 0.0001),3))); break;
							// The other 4 elements update less frequently.  We will update 1 of them.
							// Priority is from least-likely to change to most-likely to change.
							case 4:
								LiDisplay_calclateFanSpeedStr();
								LiDisplay_calculateSoCGaugeBars();
								if (LiDisplayFanSpeed_onScreen != currentFanSpeed) {
									LiDisplay_updateStringVal(0, "b1", 0, (String(fanSpeedDisplay[currentFanSpeed])));
									LiDisplayFanSpeed_onScreen = currentFanSpeed;
								} else if (LiDisplaySoCBars_onScreen != LiDisplaySoCBarCount) {
									LiDisplay_updateNumericVal(0, "p0", 2, String(LiDisplaySoCBarCount));
									LiDisplaySoCBars_onScreen = LiDisplaySoCBarCount;
								} else if (LiDisplaySoC_onScreen != SoC_getBatteryStateNow_percent()) {
									LiDisplay_updateStringVal(0, "t1", 0, (String(SoC_getBatteryStateNow_percent()) + "%"));
									LiDisplaySoC_onScreen = SoC_getBatteryStateNow_percent();
								} else if (LiDisplayPackVoltageActual_onScreen != LTC68042result_packVoltage_get()) {
									LiDisplay_updateStringVal(0, "t4", 0, String(LTC68042result_packVoltage_get()));
									LiDisplayPackVoltageActual_onScreen = LTC68042result_packVoltage_get();
								} else if (LiDisplayTemp_onScreen != temperature_battery_getLatest()) {
									LiDisplay_updateStringVal(0, "t11", 0, (String(temperature_battery_getLatest()) + "C"));
									LiDisplayPackVoltageActual_onScreen = temperature_battery_getLatest();
								} else {
									// Nothing else needed to update so we will update the chrg asst bar display again instead.
									LiDisplay_calculateChrgAsstGaugeBars();
									LiDisplay_updateNumericVal(0, "p1", 2, String(LiDisplayChrgAsstPicId));
								}
								break;
						}
					break;
					case LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID:
						if (key_getSampledState() != KEYSTATE_ON) {
							if (gpio_isGridChargerPluggedInNow()) {
								LiDisplay_setPageNumber(LIDISPLAY_GRIDCHARGE_PAGE_ID);
								LiDisplay_updatePage();
							} else {
								LiDisplaySplashPending = true;
								LiDisplay_setPageNumber(LIDISPLAY_SPLASH_PAGE_ID);
								LiDisplay_updatePage();
							}
						}
						break;
					case LIDISPLAY_GRIDCHARGE_PAGE_ID:

						LiDisplay_calculateGCTimeStr();
						switch(LiDisplayElementToUpdate)
						{
							// 4 elements update very frequently so we won't track their previous value
							case 0:	// This one doesn't update frequently, but its priority is high because we want to notify the user the instant it does update.
								if (gpio_isGridChargerChargingNow()) {
									LiDisplay_updateStringVal(3, "t7", 0, "CHARGING");
								} else LiDisplay_updateStringVal(3, "t7", 0, "NOT CHARGING");

							break;
							case 1: LiDisplay_updateStringVal(3, "t3", 0, String(LiDisplayAverageCellVoltage * 0.0001,3)); break;
							case 2: LiDisplay_updateNextCellValue();	break;
							case 3: LiDisplay_updateStringVal(3, "t8", 0, String(gc_time));	break;
							case 4:
								LiDisplay_calclateFanSpeedStr();
								if (!gc_sixty_s_fomoco_e_block_enabled && (MAX_CELL_INDEX == 59)) {
									LiDisplay_updateNumericVal(3, "t16", 3, "65516"); // E block label will be missing on a 60S FoMoCo pack display if we don't run this once.
									gc_sixty_s_fomoco_e_block_enabled = true;
								} else if (LiDisplayFanSpeed_onScreen != currentFanSpeed) {
									LiDisplay_updateStringVal(3, "b1", 0, (String(fanSpeedDisplay[currentFanSpeed])));
									LiDisplayFanSpeed_onScreen = currentFanSpeed;
								} else if (LiDisplaySoC_onScreen != SoC_getBatteryStateNow_percent()) {
									LiDisplay_updateStringVal(3, "t1", 0, (String(SoC_getBatteryStateNow_percent()) + "%"));
									LiDisplaySoC_onScreen = SoC_getBatteryStateNow_percent();
								} else if (LiDisplayPackVoltageActual_onScreen != LTC68042result_packVoltage_get()) {
									LiDisplay_updateStringVal(3, "t4", 0, String(LTC68042result_packVoltage_get()));
									LiDisplayPackVoltageActual_onScreen = LTC68042result_packVoltage_get();
								} else LiDisplay_updateNextCellValue();		break;

							case 5: LiDisplay_updateNextCellValue();	break;
							case 6: maxElementId = 5; LiDisplay_updateStringVal(3, "t10", 0, (String(gc_begin_soc_str))); break;
						}
					break;
					default : break;
				}

				LiDisplayElementToUpdate += 1;

				if (LiDisplayElementToUpdate > maxElementId) {
					LiDisplayElementToUpdate = 0;
				}
			}
		}

	#endif
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_keyOn(void) {
	#ifdef LIDISPLAY_CONNECTED
		Serial.print(F("\nLiDisplay_keyOn"));
		Serial.print(F("\nLiDisplay HMI Power On"));
		gpio_turnHMI_on();
		Serial1.begin(38400,SERIAL_8E1);
		hmi_power_millis = millis();
		LiDisplaySplashPending = true;
		LiDisplaySetPageNum = 1;

		// Reset these
		LiDisplayPackVoltageActual_onScreen = 100;
		LiDisplaySoC_onScreen = 100;
		LiDisplayFanSpeed_onScreen = 100;
		LiDisplaySoCBars_onScreen = 100;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_keyOff(void) {
	#ifdef LIDISPLAY_CONNECTED
		// Check if gpio HMI was already off
		Serial.print(F("\nLiDisplay_keyOff:  gpio_HMIStateNow = "));
		Serial.print(String(gpio_HMIStateNow()));

		if (gpio_HMIStateNow()) {
			if (!gpio_isGridChargerPluggedInNow()) {
				hmi_power_millis = millis();
				LiDisplaySplashPending = true;
				LiDisplayPowerOffPending = true;
			}
		}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_gridChargerPluggedIn(void) {
	#ifdef LIDISPLAY_CONNECTED
		Serial.print(F("\nLiDisplay_gridChargerPluggedIn"));
		Serial.print(F("\nLiDisplay HMI Power On"));
		Serial.print(F("\ngpio_HMIStateNow() = "));
		Serial.print(String(gpio_HMIStateNow()));
		if (!gpio_HMIStateNow()) {
			gpio_turnHMI_on();
			Serial1.begin(38400,SERIAL_8E1);
			hmi_power_millis = millis();
		}
		LiDisplayOnGridChargerConnected = true;
		gc_connected_millis = millis();
		gc_connected_seconds = 0;
		gc_connected_minutes = 0;
		gc_connected_hours = 0;
		gc_begin_soc_str = (String(SoC_getBatteryStateNow_percent()) + "%");
		LiDisplaySoC_onScreen = 100;
		LiDisplayFanSpeed_onScreen = 100;
		maxElementId = 6;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_gridChargerUnplugged(void) {
	#ifdef LIDISPLAY_CONNECTED
		Serial.print(F("\nLiDisplay_gridChargerUnplugged"));
		gc_connected_millis_most_recent_diff = 0;
		// Check if gpio HMI was already off
		if (gpio_HMIStateNow()) {
			if (key_getSampledState() == KEYSTATE_OFF) {
				hmi_power_millis = millis();
				LiDisplaySplashPending = true;
				LiDisplayPowerOffPending = true;
			} else {
				LiDisplay_keyOn();
			}
		}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_setPageNumber(uint8_t page) {
	LiDisplaySetPageNum = page;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t LiDisplay_bytesAvailableForWrite(void)
{
  return Serial1.availableForWrite();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t LiDisplay_writeByte(uint8_t data)
{
  Serial1.write(data);
  return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t LiDisplay_readByte(void)
{
  return Serial1.read();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t LiDisplay_bytesAvailableToRead()
{
  return Serial1.available();
}
