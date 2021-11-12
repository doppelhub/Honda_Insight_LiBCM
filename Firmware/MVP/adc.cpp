//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles all ADC calls
//JTS2doLater: Remove Arduino functions
//JTS2doLater: use interrupts, so ADC calls don't block execution (each conversion takes ~112 us)

#include "libcm.h"

int16_t packCurrent_spoofed = 0;
int8_t calibratedCurrentSensorOffset = 0; //calibrated each time key turns off

/////////////////////////////////////////////////////////////////////

uint8_t adc_packVoltage_VpinIn(void) //returns pack voltage (in volts)
{
	//Derivation:
	      //packVoltage_VpinIn  = VPIN_0to5v                               * 52 
	      //                      VPIN_0to5v = adc_VPIN_raw() * 5 ) / 1024      //adc returns 10b result
	      //packVoltage_VpinIn  =              adc_VPIN_raw() * 5 ) / 1024 * 52
	      //packVoltage_VpinIn  =              adc_VPIN_raw() * 260 / 1024
	      //packVoltage_VpinIn ~=              adc_VPIN_raw() /  4
	      //packVoltage_VpinIn ~=              adc_VPIN_raw() >> 2
	uint8_t packVoltage_VpinIn  =   (uint8_t)(analogRead(PIN_VPIN_IN) >> 2);

	return packVoltage_VpinIn; //pack voltage in volts
} 

/////////////////////////////////////////////////////////////////////

int16_t  latest_battCurrent_amps = 0;
int16_t  latest_battCurrent_counts = 0; //calibrated 10b result //0A is exactly 330 counts (i.e. ADC_NOMINAL_0A_COUNTS)

//sample ADC and return average battery current
//To prevent overflow, raw ADC result must not exceed 1191 counts (i.e. don't use an ADC with more than 10b!!)
//Returned current value is not accurate enough for coulomb counting (use stateOfCharge functions for that)
int16_t adc_measureBatteryCurrent_amps(void)
{
	static uint8_t adcSamplesTaken = 0; //samples acquired since last oversampled result
	static uint16_t adcAccumulator = 0; //raw 10b ADC results (accumulated)

	//Regardless of I2V resistance value, 0A (no regen or assist) is ~1.611 volts when VREF is 5V, which is 330 counts.
	//Actual VCC voltage doesn't matter, since ADC reference is also VCC.
	//As current increases, ADC result increases.

	//gather discrete samples
	for(int ii=0; ii<ADC_NUMSAMPLES_PER_CALL; ii++)
	{
		adcAccumulator += analogRead(PIN_BATTCURRENT);
		adcSamplesTaken++;
	}

	//process oversampled data
	if(adcSamplesTaken == ADC_NUMSAMPLES_PER_RESULT)
	{
		int16_t latest_battCurrent_counts_raw = (int16_t)(adcAccumulator >> ADC_NUMSAMPLES_2_TO_THE_N); //Average the oversampled data to a 10b result
		latest_battCurrent_counts = latest_battCurrent_counts_raw - calibratedCurrentSensorOffset; //subtract offset error

		//reset oversampler for next measurement
		adcAccumulator = 0;
		adcSamplesTaken = 0;
		
		//bound averaged ADC result to 10b unsigned
		if(latest_battCurrent_counts <    0) { latest_battCurrent_counts =    0; }
		if(latest_battCurrent_counts > 1023) { latest_battCurrent_counts = 1023; }

		SoC_integrateCharge_adcCounts(latest_battCurrent_counts);

		//convert current sensor result into approximate amperage for MCM & user-display
		#ifdef HW_REVB
			//The approximation equation below is accurate to within 3.7 amps of actual value
			latest_battCurrent_amps = ((int16_t)((((uint16_t)latest_battCurrent_counts) * 13) >> 6)) - 67;
		#elif defined HW_REVC
			//see SPICE simulation for complete derivation
			//see "RevC/V&V/OEM Current Sensor.ods" for measured results
			//The approximation equation below is accurate to within 1.0 amps of actual value
			latest_battCurrent_amps = ((int16_t)((((uint16_t)latest_battCurrent_counts) * 55) >> 8)) - 71;
		#endif
	}

	return latest_battCurrent_amps;
}

/////////////////////////////////////////////////////////////////////

int16_t adc_getLatestBatteryCurrent_amps(void) { return latest_battCurrent_amps; } //non-blocking

/////////////////////////////////////////////////////////////////////

int16_t adc_getLatestBatteryCurrent_counts(void) { return latest_battCurrent_counts; } //non-blocking

/////////////////////////////////////////////////////////////////////

int16_t adc_getLatestSpoofedCurrent_amps(void) { return packCurrent_spoofed; }

/////////////////////////////////////////////////////////////////////

void adc_updateBatteryCurrent(void)
{
	#if   defined(SET_CURRENT_HACK_60)
		packCurrent_spoofed = (int16_t)(adc_measureBatteryCurrent_amps() * 0.62); //160% current hack = tell MCM 62% actual
	#elif defined(SET_CURRENT_HACK_40) 
		packCurrent_spoofed = (int16_t)(adc_measureBatteryCurrent_amps() * 0.70); //140% current hack = tell MCM 70% actual
	#elif defined(SET_CURRENT_HACK_20)
		packCurrent_spoofed = (int16_t)(adc_measureBatteryCurrent_amps() * 0.83); //120% current hack = tell MCM 83% actual
	#elif defined(SET_CURRENT_HACK_00)
		packCurrent_spoofed = adc_measureBatteryCurrent_amps();
	#else
		#error (SET_CURRENT_HACK_xx value not selected in config.c)
	#endif

	BATTSCI_setSpoofedCurrent(packCurrent_spoofed);
}

/////////////////////////////////////////////////////////////////////

//only call this function when no current is flowing through the sensor (e.g. when key is off)
void adc_calibrateBatteryCurrentSensorOffset(void)
{
	uint16_t adcAccumulator   =     0;
	uint16_t minResult_counts = 65535;
	uint16_t maxResult_counts =     0;

	//gather current sensor samples
	for(uint8_t ii=0; ii<ADC_NUMSAMPLES_PER_RESULT; ii++) 
	{
		uint16_t adcResult = analogRead(PIN_BATTCURRENT);
		adcAccumulator += adcResult;
		
		if(adcResult < minResult_counts) { minResult_counts = adcResult; } //store max ADC result
		if(adcResult > maxResult_counts) { maxResult_counts = adcResult; } //store min ADC result
	}

	//verify returned values are in the right ballpark
	if( ((maxResult_counts - minResult_counts) < 2)       && /* verify all returned values are within 1 count */
		 (maxResult_counts < (ADC_NOMINAL_0A_COUNTS + 8)) && /* verify hardware offset tolerance isn't too high */
		 (minResult_counts > (ADC_NOMINAL_0A_COUNTS - 8)) )  /* verify hardware offset tolerance isn't too low  */ 
	{
		uint16_t adcZeroCrossing_counts = (adcAccumulator >> ADC_NUMSAMPLES_2_TO_THE_N);
		calibratedCurrentSensorOffset = adcZeroCrossing_counts - ADC_NOMINAL_0A_COUNTS;
		Serial.print(F("\nCurrent Sensor 0A set to (counts): "));
		Serial.print(String(adcZeroCrossing_counts));
	} 
	else
	{
		Serial.print(F("\nCurrent Sensor Offset exceeds limit. Using last valid offset. MAX:"));
		Serial.print(String(maxResult_counts));
		Serial.print(F(", MIN:"));
		Serial.print(String(minResult_counts));
	}
}

/////////////////////////////////////////////////////////////////////

uint16_t adc_getTemperature(uint8_t tempToMeasure)
{
	return analogRead(tempToMeasure);
}

/////////////////////////////////////////////////////////////////////