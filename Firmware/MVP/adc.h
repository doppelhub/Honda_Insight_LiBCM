//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef adc_h
	#define adc_h

	uint8_t adc_packVoltage_VpinIn(void);

	int16_t adc_measureBatteryCurrent_Amps(void);

	int16_t adc_getLatestBatteryCurrent_Amps(void);
#endif