//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//maintains battery assist and regen watt hour counts

#include "libcm.h"

uint16_t wattHours_assist = 0;
uint16_t wattHours_regen = 0;
uint16_t wattHours_gridCharger = 0;

// Users with LiDisplay will have their own trip meter they can reset, most likely at each fill up.
static uint16_t tripmeter_wattHours_assist = 0;
static uint16_t tripMeter_wattHours_regen = 0;
static uint16_t tripMeter_wattHours_gridCharger = 0;

/////////////////////////////////////////////////////////////////////////////////////////

uint16_t energy_getAssist_Wh(void) { return wattHours_assist; }
uint16_t energy_getRegen_Wh (void) { return wattHours_regen;  }
uint16_t energy_getGridCharger_Wh (void) { return wattHours_gridCharger;  }

uint16_t energy_getTripMeterAssist_Wh(void) { return tripmeter_wattHours_assist; }
uint16_t energy_getTripMeterRegen_Wh (void) { return tripMeter_wattHours_regen;  }
uint16_t energy_getTripMeterGridCharge_Wh(void) { return tripMeter_wattHours_gridCharger; }

/////////////////////////////////////////////////////////////////////////////////////////

int32_t energySinceLastCall_uWh(void)
{
	static uint32_t timestamp_lastCall_ms = 0;

	uint32_t milliseconds_now = millis();
	uint8_t period_ms = (uint8_t)(milliseconds_now - timestamp_lastCall_ms);
	timestamp_lastCall_ms = milliseconds_now;

	if (period_ms > (time_loopPeriod_ms_get() << 2)) { return 0; } //ignore keyOn

	int32_t power_deciWatts = (int32_t)adc_getLatestBatteryCurrent_deciAmps() * LTC68042result_packVoltage_get();

	int32_t uWh_sinceLastUpdate = (power_deciWatts * period_ms * 114) >> 12; //divide by 36

	return uWh_sinceLastUpdate;
}

/////////////////////////////////////////////////////////////////////////////////////////

void energy_zeroWh(void)
{
	wattHours_assist = 0;
	wattHours_regen  = 0;
	wattHours_gridCharger = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

void energy_storeTrip(void)
{
	uint16_t distance_TBD = 0; //JTS2doLater: Add distance //requires SPI link to LiControl

	if ((wattHours_regen  > MIN_Wh_TO_STORE_TRIP) ||
		(wattHours_assist > MIN_Wh_TO_STORE_TRIP)  )
	{
		eeprom_wattHourHistory_storeSession(time_sinceLatestKeyOn_seconds(),
											distance_TBD,
											wattHours_assist,
											wattHours_regen               );
	}

	energy_zeroWh();
}

/////////////////////////////////////////////////////////////////////////////////////////

void energy_zeroWhTripMeter(void)
{
	tripmeter_wattHours_assist = 0;
	tripMeter_wattHours_regen  = 0;
	tripMeter_wattHours_gridCharger = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

void energy_storeTripMeter(uint16_t drive_assist_wh, uint16_t drive_regen_wh)
{
	tripmeter_wattHours_assist += drive_assist_wh;
	tripMeter_wattHours_regen  += drive_regen_wh;
}

/////////////////////////////////////////////////////////////////////////////////////////

void energy_storeTripMeterGridCharge(uint16_t grid_charger_wh)
{
	tripMeter_wattHours_gridCharger += grid_charger_wh;
}

/////////////////////////////////////////////////////////////////////////////////////////

void accumulate_uWh_to_Wh(void)
{
	static uint32_t uWh_remainder_assist = 0;
	static uint32_t uWh_remainder_regen  = 0;
	int32_t uWh_thisLoop = energySinceLastCall_uWh();

	if (uWh_thisLoop < 0)
	{
		//regen
		uint32_t uWh_helper = uWh_remainder_regen - uWh_thisLoop; //adding, uWh_thisLoop is negative

		while (uWh_helper >= 1000000)
		{
			uWh_helper -= 1000000;
			if (wattHours_regen < 0xFFFF) {
				if (gpio_isGridChargerPluggedInNow() == YES) { wattHours_gridCharger++; }
				else wattHours_regen++;
			}
		}
		uWh_remainder_regen = uWh_helper;
	}
	else
	{
		//assist
		uint32_t uWh_helper = uWh_remainder_assist + uWh_thisLoop;

		while (uWh_helper >= 1000000)
		{
			uWh_helper -= 1000000;
			if (wattHours_assist < 0xFFFF) { wattHours_assist++; }
		}
		uWh_remainder_assist = uWh_helper;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

void energy_handler(void)
{
	if (key_getSampledState() == KEYSTATE_ON) { accumulate_uWh_to_Wh(); }
	#ifdef LIDISPLAY_CONNECTED
		if (gpio_isGridChargerPluggedInNow() == YES) { accumulate_uWh_to_Wh(); }
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
