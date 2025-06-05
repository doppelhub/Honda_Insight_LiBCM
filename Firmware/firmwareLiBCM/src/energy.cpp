//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//maintains battery assist and regen watt hour counts

#include "libcm.h"

uint16_t wattHours_assist = 0;
uint16_t wattHours_regen = 0;

/////////////////////////////////////////////////////////////////////////////////////////

uint16_t energy_getAssist_Wh(void) { return wattHours_assist; }
uint16_t energy_getRegen_Wh (void) { return wattHours_regen;  }

/////////////////////////////////////////////////////////////////////////////////////////

int32_t energySinceLastCall_uWh(void)
{
	static uint32_t timestamp_lastCall_ms = 0;

	uint32_t milliseconds_now = millis();
	uint8_t period_ms = (uint8_t)(milliseconds_now - timestamp_lastCall_ms);
	timestamp_lastCall_ms = milliseconds_now;
	
	if (period_ms > (time_loopPeriod_ms_get() << 2)) { return 0; } //ignore keyOn, chargeOn
	
	int32_t power_deciWatts = (int32_t)adc_getLatestBatteryCurrent_deciAmps() * LTC68042result_packVoltage_get();
	
	int32_t uWh_sinceLastUpdate = (power_deciWatts * period_ms * 114) >> 12; //divide by 36

	return uWh_sinceLastUpdate;
}

/////////////////////////////////////////////////////////////////////////////////////////

void energy_keyOn(void)
{
	wattHours_assist = 0;
	wattHours_regen  = 0;
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
			if (wattHours_regen < 32767) { wattHours_regen++; } //MSb stores charge source: grid or regen
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
			if (wattHours_assist < 65535) { wattHours_assist++; }
		}
		uWh_remainder_assist = uWh_helper;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

void energy_handler(void)
{
	if ((key_getSampledState() == KEYSTATE_ON) ||
		(gpio_isGridChargerChargingNow() == YES ) )
	{
		accumulate_uWh_to_Wh();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
