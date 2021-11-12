//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef adc_h
	#define adc_h

	uint8_t adc_packVoltage_VpinIn(void);

	int16_t adc_measureBatteryCurrent_amps(void);

	int16_t adc_getLatestBatteryCurrent_amps(void);

	int16_t adc_getLatestBatteryCurrent_counts(void);

	void adc_updateBatteryCurrent(void);

	int16_t adc_getLatestSpoofedCurrent_amps(void);

	uint16_t adc_getTemperature(uint8_t tempToMeasure);

	void adc_calibrateBatteryCurrentSensorOffset(void);

#endif

/*
To implement later:

#define ADC_DDR DDRF
	#define ADC_DIR	PORTF
	#define ADC_CURRENT_BIT  0 //mega pin A0 PF0
	#define ADC_TEMP_YEL_BIT 3 //mega pin A3 PF3
	#define ADC_TEMP_GRN_BIT 4 //mega pin A4 PF4
	#define ADC_TEMP_WHT_BIT 5 //mega pin A5 PF5
	#define ADC_TEMP_BLU_BIT 6 //mega pin A6 PF6
	#define ADC_VPIN_IN_BIT  7 //mega pin A7 PF7
	#define ADC_MASK ((1<<ADC_CURRENT_BIT)|(1<<ADC_TEMP_YEL_BIT)|(1<<ADC_TEMP_GRN_BIT)|(1<<ADC_TEMP_WHT_BIT)|(1<<ADC_TEMP_BLU_BIT)|(1<<ADC_VPIN_IN_BIT))
*/