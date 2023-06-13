//Copyright 2021-2023(c) John Sullivan
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

	#define ADC_NUMSAMPLES_PER_RESULT  8 //Valid values: 1,2,4,8,16,32,64 //MUST ALSO CHANGE next line!
	#define ADC_NUMSAMPLES_2_TO_THE_N  3 //Valid values: 0,1,2,3, 4, 5, 6 //2^N = ADC_NUMSAMPLES_PER_RESULT
	#define ADC_NUMSAMPLES_PER_CALL    2 //Must be divisible into ADC_NUMSAMPLES_PER_RESULT!
	#define ADC_NUMLOOPS_PER_RESULT (ADC_NUMSAMPLES_PER_RESULT / ADC_NUMSAMPLES_PER_CALL) //division constants are handled by pre-processor

	#define ADC_NOMINAL_0A_COUNTS 332 //calculated ADC 10b result when no current flows through sensor
	#define ADC_MILLIAMPS_PER_COUNT 215 //Derivation here: ~/Electronics/PCB (KiCAD)/RevC/V&V/OEM Current Sensor.ods

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