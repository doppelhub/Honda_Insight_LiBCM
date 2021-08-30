//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef adc_h
	#define adc_h

	uint8_t adc_packVoltage_VpinIn(void);

	int16_t adc_measureBatteryCurrent_amps(void);

	int16_t adc_getLatestBatteryCurrent_amps(void);

	int16_t adc_getLatestBatteryCurrent_counts(void);
#endif