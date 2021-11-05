//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles all ADC calls
//JTS2doLater: Remove Arduino functions
//JTS2doLater: use interrupts, so ADC calls don't block execution (each conversion takes ~112 us)

#include "libcm.h"

int16_t packCurrent_spoofed = 0;

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

int16_t latest_battCurrent_amps = 0;
int16_t latest_battCurrent_counts = 0;

//sample ADC and returns battery
int16_t adc_measureBatteryCurrent_amps(void)
{
	#define NUM_ADCSAMPLES_PER_RESULT 64 //Valid values: 1,2,4,8,16,32,64 //MUST ALSO CHANGE next line!
	#define NUM_ADCSAMPLES_2_TO_THE_N  6 //Valid values: 0,1,2,3, 4, 5, 6 //2^N = NUM_ADCSAMPLES_PER_RESULT
	#define NUM_ADCSAMPLES_PER_CALL    4 //Must be divisible into NUM_ADCSAMPLES_PER_RESULT!
	
	static uint8_t adcSamplesTaken = 0; //samples acquired since last oversampled result
	static uint16_t adcAccumulator = 0; //raw 10b ADC results

	for(int ii=0; ii<NUM_ADCSAMPLES_PER_CALL; ii++)
	{
		adcAccumulator += analogRead(PIN_BATTCURRENT);
		adcSamplesTaken++;
	}

	if(adcSamplesTaken == NUM_ADCSAMPLES_PER_RESULT)
	{
		latest_battCurrent_counts = (int16_t)( (adcAccumulator >> NUM_ADCSAMPLES_2_TO_THE_N) ); //Shift must match
		adcAccumulator = 0;
		adcSamplesTaken = 0;

		//convert current sensor result into approximate amperage for MCM & user-display
		//don't use this result for current accumulation... it's not accurate enough (FYI: SoC accumulates raw ADC result)
		//Regardless of I2V resistance (R50||R53||R516), 0A (regen/assist) is 1.621 volts (332 counts with 10b ADC)
		//As current increases, ADC result increase.
		//Actual VCC voltage doesn't matter, since ADC reference is also VCC (e.g. ADC result @ 0A is always 332 counts)
		#ifdef HW_REVB
			latest_battCurrent_amps = ( (latest_battCurrent_counts * 13) >> 6) - 67; //Accurate to within 3.7 amps of actual value
		
		#elif defined HW_REVC
			//see SPICE simulation for complete derivation
			//@-70A regen,  ADC result is 009 counts
			//@140A assist, ADC result is 979 counts
			latest_battCurrent_amps = ( (latest_battCurrent_counts * 14) >> 6) -73; //Accurate to within 1.2 amps of actual value
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

uint16_t adc_getTemperature(uint8_t tempToMeasure)
{
	return analogRead(tempToMeasure);
}

