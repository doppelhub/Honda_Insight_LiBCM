//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LiDisplay (HMI) Serial Functions

#include "libcm.h"

#define LIDISPLAY_DRIVING_PAGE_ID 0
#define LIDISPLAY_SPLASH_PAGE_ID 1
#define LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID 2
#define LIDISPLAY_GRIDCHARGE_PAGE_ID 3
#define LIDISPLAY_SETTINGS_PAGE_ID 4
// #define CMD 0x23								// May be used when reading from nextion.  0x23 = "#"
#define LIDISPLAY_UPDATE_RATE_MILLIS 100		// One element is updated each time

uint8_t LiDisplayElementToUpdate = 0;
uint8_t LiDisplayCurrentPageNum = 0;
uint8_t LiDisplaySetPageNum = 0;
uint8_t LiDisplaySoCBarCount = 0;
uint8_t LiDisplayChrgAsstPicId = 22;

// Initializing to 100 (absurd number for all 4 variables) so that on first run they will be updated on screen
static uint8_t  LiDisplayPackVoltageActual_onScreen = 100;
static uint8_t  LiDisplaySoC_onScreen = 100;
static uint8_t  LiDisplayFanSpeed_onScreen = 100;
static uint8_t  LiDisplaySoCBars_onScreen = 100;
static uint8_t	LiDisplayTemp_onScreen = 0;

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

const String attrMap[3] = {
	"txt", "val", "pic"
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
		Serial1.begin(38400,SERIAL_8E1);

		LiDisplayElementToUpdate = 0;

		LiDisplaySplashPending = false;
		LiDisplayPowerOffPending = false;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateCorrectPage() {
	if (gpio_keyStateNow()) {
		// Key is On
		if (gpio_isGridChargerPluggedInNow()) {
			// Grid Charger Plugged In -- Display Warning Page
			LiDisplaySetPageNum = LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID;
		} else if (LiDisplaySplashPending) {
			LiDisplaySetPageNum = LIDISPLAY_SPLASH_PAGE_ID;
		} else if (LiDisplaySettingsPageRequested) {
			LiDisplaySetPageNum = LIDISPLAY_SETTINGS_PAGE_ID;
		} else {
			LiDisplaySetPageNum = LIDISPLAY_DRIVING_PAGE_ID;
		}
	} else {
		// Key is Off
		if (gpio_isGridChargerPluggedInNow()) {
			// Grid Charger Plugged In -- Display GC Page
			LiDisplaySetPageNum = LIDISPLAY_GRIDCHARGE_PAGE_ID;
		} else if (LiDisplaySplashPending) {
			LiDisplaySetPageNum = LIDISPLAY_SPLASH_PAGE_ID;
		} else if (LiDisplaySettingsPageRequested) {
			LiDisplaySetPageNum = LIDISPLAY_SETTINGS_PAGE_ID;
		}
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

		gc_connected_hours = (gc_connected_millis_most_recent_diff / 3600000);
		gc_connected_minutes = (gc_connected_millis_most_recent_diff / 60000) % 60;
		gc_connected_seconds = (gc_connected_millis_most_recent_diff / 1000) % 60;

		if (gc_connected_seconds > 9) {
			gc_sec_prefix = "";
		} else gc_sec_prefix = String(0);
		if (gc_connected_minutes > 9) {
			gc_min_prefix = "";
		} else gc_min_prefix = String(0);
		if (gc_connected_hours > 9) {
			gc_hour_prefix = "";
		} else gc_hour_prefix = String(0);

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
	int16_t packEHP = (LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps()) / 746; // USA Electrical Horsepower is defined as 746 Watts

	if (packEHP <= -18) {
		LiDisplayChrgAsstPicId = 58;
	} else if (packEHP <= -17) {
		LiDisplayChrgAsstPicId = 57;
	} else if (packEHP <= -16) {
		LiDisplayChrgAsstPicId = 56;
	} else if (packEHP <= -15) {
		LiDisplayChrgAsstPicId = 55;
	} else if (packEHP <= -14) {
		LiDisplayChrgAsstPicId = 54;
	} else if (packEHP <= -13) {
		LiDisplayChrgAsstPicId = 53;
	} else if (packEHP <= -12) {
		LiDisplayChrgAsstPicId = 52;
	} else if (packEHP <= -11) {
		LiDisplayChrgAsstPicId = 51;
	} else if (packEHP <= -10) {
		LiDisplayChrgAsstPicId = 50;
	} else if (packEHP <= -9) {
		LiDisplayChrgAsstPicId = 49;
	} else if (packEHP <= -8) {
		LiDisplayChrgAsstPicId = 48;
	} else if (packEHP <= -7) {
		LiDisplayChrgAsstPicId = 47;
	} else if (packEHP <= -6) {
		LiDisplayChrgAsstPicId = 46;
	} else if (packEHP <= -5) {
		LiDisplayChrgAsstPicId = 45;
	} else if (packEHP <= -4) {
		LiDisplayChrgAsstPicId = 44;
	} else if (packEHP <= -3) {
		LiDisplayChrgAsstPicId = 43;
	} else if (packEHP <= -2) {
		LiDisplayChrgAsstPicId = 42;
	} else if (packEHP <= -1) {
		LiDisplayChrgAsstPicId = 41;
	} else if (packEHP <= 0) {
		LiDisplayChrgAsstPicId = 22;
	} else if (packEHP <= 1) {
		LiDisplayChrgAsstPicId = 23;
	} else if (packEHP <= 2) {
		LiDisplayChrgAsstPicId = 24;
	} else if (packEHP <= 3) {
		LiDisplayChrgAsstPicId = 25;
	} else if (packEHP <= 4) {
		LiDisplayChrgAsstPicId = 26;
	} else if (packEHP <= 5) {
		LiDisplayChrgAsstPicId = 27;
	} else if (packEHP <= 6) {
		LiDisplayChrgAsstPicId = 28;
	} else if (packEHP <= 7) {
		LiDisplayChrgAsstPicId = 29;
	} else if (packEHP <= 8) {
		LiDisplayChrgAsstPicId = 30;
	} else if (packEHP <= 9) {
		LiDisplayChrgAsstPicId = 31;
	} else if (packEHP <= 10) {
		LiDisplayChrgAsstPicId = 32;
	} else if (packEHP <= 11) {
		LiDisplayChrgAsstPicId = 33;
	} else if (packEHP <= 12) {
		LiDisplayChrgAsstPicId = 34;
	} else if (packEHP <= 13) {
		LiDisplayChrgAsstPicId = 35;
	} else if (packEHP <= 14) {
		LiDisplayChrgAsstPicId = 36;
	} else if (packEHP <= 15) {
		LiDisplayChrgAsstPicId = 37;
	} else if (packEHP <= 16) {
		LiDisplayChrgAsstPicId = 38;
	} else if (packEHP <= 17) {
		LiDisplayChrgAsstPicId = 39;
	} else {
		LiDisplayChrgAsstPicId = 40;
	}
	// 2022 Sept 07 -- NM To Do: The assist display can only show up to 18 HP of assist, but LiBCM can put out over 20 HP
	// Need to edit the HMI file to have a graphical display of those extra HP, probably by further highlighting some of the assist bars


	// Next lines are for debugging and can be removed later.
/*	Serial.print(F("\n"));
	Serial.print("LiDisplay DEBUG -- PicId = ");
	Serial.print(String(LiDisplayChrgAsstPicId));
	Serial.print(" -- packEHP = ");
	Serial.print(String(packEHP));
	Serial.print(" -- amps = ");
	Serial.print(String(adc_getLatestBatteryCurrent_amps()));
	Serial.print(" -- volts = ");
	Serial.print(String(LTC68042result_packVoltage_get())); */

	// Debug output:

	// LiDisplay DEBUG -- PicId = 22 -- packEHP = 0 -- amps = -1 -- volts = 183
	// LiDisplay DEBUG -- PicId = 22 -- packEHP = 0 -- amps = 1 -- volts = 181
	// LiDisplay DEBUG -- PicId = 23 -- packEHP = 1 -- amps = 7 -- volts = 183
	// LiDisplay DEBUG -- PicId = 24 -- packEHP = 2 -- amps = 9 -- volts = 183
	// LiDisplay DEBUG -- PicId = 24 -- packEHP = 2 -- amps = 12 -- volts = 182
	// LiDisplay DEBUG -- PicId = 25 -- packEHP = 3 -- amps = 16 -- volts = 182
	// LiDisplay DEBUG -- PicId = 26 -- packEHP = 4 -- amps = 19 -- volts = 182
	// LiDisplay DEBUG -- PicId = 27 -- packEHP = 5 -- amps = 21 -- volts = 185
	// LiDisplay DEBUG -- PicId = 30 -- packEHP = 8 -- amps = 36 -- volts = 182
	// LiDisplay DEBUG -- PicId = 37 -- packEHP = 15 -- amps = 63 -- volts = 181

	// LiDisplay DEBUG -- PicId = 43 -- packEHP = -3 -- amps = -15 -- volts = 184
	// LiDisplay DEBUG -- PicId = 44 -- packEHP = -4 -- amps = -19 -- volts = 187
	// LiDisplay DEBUG -- PicId = 45 -- packEHP = -5 -- amps = -21 -- volts = 186
	// LiDisplay DEBUG -- PicId = 46 -- packEHP = -6 -- amps = -25 -- volts = 186
	// LiDisplay DEBUG -- PicId = 49 -- packEHP = -9 -- amps = -39 -- volts = 187
	// LiDisplay DEBUG -- PicId = 51 -- packEHP = -11 -- amps = -44 -- volts = 187
	// LiDisplay DEBUG -- PicId = 52 -- packEHP = -12 -- amps = -49 -- volts = 187

};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calculateSoCGaugeBars() {
  if (SoC_getBatteryStateNow_percent() >= 76) {
    LiDisplaySoCBarCount = 20;
  } else if (SoC_getBatteryStateNow_percent() >= 73) {
    LiDisplaySoCBarCount = 19;
  } else if (SoC_getBatteryStateNow_percent() >= 70) {
    LiDisplaySoCBarCount = 18;
  } else if (SoC_getBatteryStateNow_percent() >= 67) {
    LiDisplaySoCBarCount = 17;
  } else if (SoC_getBatteryStateNow_percent() >= 64) {
    LiDisplaySoCBarCount = 16;
  } else if (SoC_getBatteryStateNow_percent() >= 61) {
    LiDisplaySoCBarCount = 15;
  } else if (SoC_getBatteryStateNow_percent() >= 58) {
    LiDisplaySoCBarCount = 14;
  } else if (SoC_getBatteryStateNow_percent() >= 55) {
    LiDisplaySoCBarCount = 13;
  } else if (SoC_getBatteryStateNow_percent() >= 52) {
    LiDisplaySoCBarCount = 12;
  } else if (SoC_getBatteryStateNow_percent() >= 49) {
    LiDisplaySoCBarCount = 11;
  } else if (SoC_getBatteryStateNow_percent() >= 46) {
    LiDisplaySoCBarCount = 10;
  } else if (SoC_getBatteryStateNow_percent() >= 43) {
    LiDisplaySoCBarCount = 9;
  } else if (SoC_getBatteryStateNow_percent() >= 40) {
    LiDisplaySoCBarCount = 8;
  } else if (SoC_getBatteryStateNow_percent() >= 37) {
    LiDisplaySoCBarCount = 7;
  } else if (SoC_getBatteryStateNow_percent() >= 34) {
    LiDisplaySoCBarCount = 6;
  } else if (SoC_getBatteryStateNow_percent() >= 34) {
    LiDisplaySoCBarCount = 5;
  } else if (SoC_getBatteryStateNow_percent() >= 31) {
    LiDisplaySoCBarCount = 4;
  } else if (SoC_getBatteryStateNow_percent() >= 28) {
    LiDisplaySoCBarCount = 3;
  } else if (SoC_getBatteryStateNow_percent() >= 25) {
    LiDisplaySoCBarCount = 2;
  } else if (SoC_getBatteryStateNow_percent() >= 22) {
    LiDisplaySoCBarCount = 1;
  } else {
    LiDisplaySoCBarCount = 0;
  }
  return;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_calclateFanSpeedStr() {
	if (fan_getCurrentSpeed(FAN_PCB) == 'H') {
		currentFanSpeed = 2;
	} else if (fan_getCurrentSpeed(FAN_PCB) == 'L') {
		currentFanSpeed = 1;
	} else {
		currentFanSpeed = 0;
	}
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

void LiDisplay_refresh(void)
{
	#ifdef LIDISPLAY_CONNECTED

		static uint32_t millis_previous = 0;
		static uint32_t splash_millis = 0;
		static uint32_t hmi_read_millis = 0;
		static uint8_t maxElementId = 8;

		// Below stuff doesn't work properly yet.
		static uint8_t cmd = 0;
		static String cmd_str = "";

		if (cmd = LiDisplay_bytesAvailableToRead()) {
			cmd_str = cmd;
			cmd_str = cmd_str + " " + String(LiDisplay_readByte());
			cmd_str = cmd_str + " " + String(LiDisplay_readByte());
			cmd_str = cmd_str + " " + String(LiDisplay_readByte());
			cmd_str = cmd_str + " " + String(LiDisplay_readByte());
			cmd_str = cmd_str + " " + String(LiDisplay_readByte());
			cmd_str = cmd_str + " " + String(LiDisplay_readByte());

			Serial.print(F("\ncmd_str: "));
			Serial.print(cmd_str);
		}
		// Above stuff doesn't work properly yet.

		LiDisplay_calculateCorrectPage();

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
				LiDisplay_updateNumericVal(1, "n0", 0, String(REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS - EEPROM_uptimeStoredInEEPROM_hours_get()));
				return;
			} else if ((millis() - splash_millis) > (LIDISPLAY_SPLASH_PAGE_MS)) {
				gpio_turnHMI_off();
				LiDisplayPowerOffPending = false;
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
				hmi_power_millis = millis();
				return;
			}
			if ((millis() - hmi_power_millis) < 100) { // ensure at least 100ms have passed since screen turned on
				return;
			} else {
				// Ready to show GC page, but first check if key is on
				// 2022 Sept 07 -- NM To Do: We may be able to delete this now because of LiDisplay_calculateCorrectPage()
				// We may need to create a LiDisplay_calculateCorrectPowerState() function instead of this
				if (gpio_keyStateNow()) {
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
					if (LiDisplayElementToUpdate > 2) {
						LiDisplayElementToUpdate = 0;
					}
					switch(LiDisplayElementToUpdate)
					{
						case 0:
							LiDisplay_updateStringVal(1, "t1", 0, String(FW_VERSION));
							break;
						case 1:
							LiDisplay_updateStringVal(1, "t3", 0, String(REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS - EEPROM_uptimeStoredInEEPROM_hours_get()));
							break;
					}
					LiDisplayElementToUpdate += 1;
				}
			} else {	// Main page loop
				// 2022 Sept 11 -- NM To Do:  Replace grid charge display code w/ code that only updates elements which have changed since last cycle.
				switch(LiDisplayCurrentPageNum) {
					case LIDISPLAY_DRIVING_PAGE_ID:

						maxElementId = 4;
						switch(LiDisplayElementToUpdate)
						{
							// 4 elements update very frequently so we won't track their previous value
							case 0: LiDisplay_updateStringVal(0, "t3", 0, String((LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps())/1000)); break;
							case 1: LiDisplay_calculateChrgAsstGaugeBars(); LiDisplay_updateNumericVal(0, "p1", 2, String(LiDisplayChrgAsstPicId));	break;
							case 2: LiDisplay_updateStringVal(0, "t9", 0, (String((LTC68042result_hiCellVoltage_get()/10)))); break;
							case 3: LiDisplay_updateStringVal(0, "t6", 0, (String((LTC68042result_loCellVoltage_get()/10)))); break;
							// The other 4 elements update less frequently.  We will update 1 of them.
							// Priority is from least-likely to change to most-likely to change.
							case 4:
								if (LiDisplayFanSpeed_onScreen != currentFanSpeed) {
									LiDisplay_calclateFanSpeedStr();
									LiDisplay_updateStringVal(0, "b1", 0, (String(fanSpeedDisplay[currentFanSpeed])));
									LiDisplayFanSpeed_onScreen = currentFanSpeed;
								} else if (LiDisplaySoCBars_onScreen != LiDisplaySoCBarCount) {
									LiDisplay_calculateSoCGaugeBars();
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
						/*
						maxElementId = 8;
						switch(LiDisplayElementToUpdate)
						{
							case 0: LiDisplay_calculateChrgAsstGaugeBars(); LiDisplay_updateNumericVal(0, "p1", 2, String(LiDisplayChrgAsstPicId));	break;
							case 1: LiDisplay_updateStringVal(0, "t1", 0, (String(SoC_getBatteryStateNow_percent()) + "%")); break;
							case 2: LiDisplay_updateStringVal(0, "t3", 0, String(LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps()); break;
							case 3: LiDisplay_calculateSoCGaugeBars(); LiDisplay_updateNumericVal(0, "p0", 2, String(LiDisplaySoCBarCount));	break;
							case 4: LiDisplay_calclateFanSpeedStr(); LiDisplay_updateStringVal(0, "b1", 0, (String(fanSpeedDisplay[currentFanSpeed]))); break;
							case 5: LiDisplay_calculateChrgAsstGaugeBars(); LiDisplay_updateNumericVal(0, "p1", 2, String(LiDisplayChrgAsstPicId));	break;
							case 6: LiDisplay_updateStringVal(0, "t4", 0, String(LTC68042result_packVoltage_get()));	break;
							case 7: LiDisplay_updateStringVal(0, "t9", 0, (String((LTC68042result_hiCellVoltage_get()/10)))); break;
							case 8: LiDisplay_updateStringVal(0, "t6", 0, (String((LTC68042result_loCellVoltage_get()/10)))); break;
						}
						*/
					break;
					case LIDISPLAY_GRIDCHARGE_WARNING_PAGE_ID:
						if (!gpio_keyStateNow()) {
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
						maxElementId = 7;
						LiDisplay_calculateGCTimeStr();
						switch(LiDisplayElementToUpdate)
						{
							case 0:
								if (gpio_isGridChargerChargingNow()) {
									LiDisplay_updateStringVal(3, "t7", 0, "CHARGING");
								} else LiDisplay_updateStringVal(3, "t7", 0, "NOT CHARGING");
							break;
							case 1: LiDisplay_updateStringVal(3, "t1", 0, (String(SoC_getBatteryStateNow_percent()) + "%")); break;
							case 2: LiDisplay_updateStringVal(3, "t3", 0, String(adc_getLatestBatteryCurrent_amps())); break;
							case 3: LiDisplay_calculateSoCGaugeBars(); LiDisplay_updateNumericVal(3, "p0", 2, String(LiDisplaySoCBarCount));	break;
							case 4: LiDisplay_updateStringVal(3, "t8", 0, String(gc_time));	break;
							case 5: LiDisplay_updateStringVal(3, "t4", 0, String(LTC68042result_packVoltage_get()));	break;
							case 6: LiDisplay_updateStringVal(3, "t10", 0, (String(gc_begin_soc_str))); break;
							case 7: LiDisplay_calclateFanSpeedStr(); LiDisplay_updateStringVal(3, "b1", 0, (String(fanSpeedDisplay[currentFanSpeed]))); break;
							// case 8: LiDisplay_calculateSoCGaugeBars(); LiDisplay_updateNumericVal(3, "p0", 2, String(LiDisplaySoCBarCount));	break;
						}
					break;
					default : break;
				}

				LiDisplayElementToUpdate += 1;

				if (LiDisplayElementToUpdate > maxElementId) {
					LiDisplayElementToUpdate = 0;
				}
			}
			/*
			uint8_t updateAttempts = 0;
			do
			{
				if( (++LiDisplayElementToUpdate) > LIDISPLAYUPDATE_MAX_VALUE ) { LiDisplayElementToUpdate = 1; } //reset to first element
				updateAttempts++;
			} while( (lcd_updateValue(LiDisplayElementToUpdate) == SCREEN_DIDNT_UPDATE) && (updateAttempts < MAX_LIDISPLAYUPDATE_ATTEMPTS) );
			*/
		}

	#endif
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_keyOn(void) {
	#ifdef LIDISPLAY_CONNECTED
		Serial.print(F("\nLiDisplay_keyOn"));
		Serial.print(F("\nLiDisplay HMI Power On"));
		gpio_turnHMI_on();
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
		Serial.print(F("\nnLiDisplay_keyOff:  gpio_HMIStateNow = "));
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
			hmi_power_millis = millis();
		}
		LiDisplayOnGridChargerConnected = true;
		gc_connected_millis = millis();
		gc_connected_seconds = 0;
		gc_connected_minutes = 0;
		gc_connected_hours = 0;
		gc_begin_soc_str = (String(SoC_getBatteryStateNow_percent()) + "%");
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LiDisplay_gridChargerUnplugged(void) {
	#ifdef LIDISPLAY_CONNECTED
		Serial.print(F("\nLiDisplay_gridChargerUnplugged"));
		gc_connected_millis_most_recent_diff = 0;
		// Check if gpio HMI was already off
		if (gpio_HMIStateNow()) {
			if (!gpio_keyStateNow()) {
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
