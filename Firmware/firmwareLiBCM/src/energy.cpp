//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//maintains battery assist and regen watt hour counts

#include "libcm.h"


/////////////////////////////////////////////////////////////////////////////////////////

void energy_integrate_centiJoules(void)
{
	static uint32_t timestamp_lastCall_ms = 0;

	uint32_t milliseconds_now = millis();
	uint8_t periodBetweenCalls_ms = (uint8_t)(milliseconds_now - timestamp_lastCall_ms);
	timestamp_lastCall_ms = milliseconds_now;

	uint32_t power_deciWatts = LTC68042result_packVoltage_get() * (uint32_t)adc_getLatestBatteryCurrent_deciAmps();

    //       centiJoules_sinceLastUpdate =            (power_deciWatts / 10) *        (loopPeriod_ms / 1000) * (100 cJ/J)
    //       centiJoules_sinceLastUpdate =            (power_deciWatts       *         loopPeriod_ms       ) /  100
    //       centiJoules_sinceLastUpdate =            (power_deciWatts       *         loopPeriod_ms       ) *  0.01
    //       centiJoules_sinceLastUpdate =           ((power_deciWatts       *         loopPeriod_ms       ) * 41) >> 12
	uint16_t centiJoules_sinceLastUpdate = (uint16_t)((power_deciWatts       * periodBetweenCalls_ms       ) * 41) >> 12;
}

/////////////////////////////////////////////////////////////////////////////////////////
