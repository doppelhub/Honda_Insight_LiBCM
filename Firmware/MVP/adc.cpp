//handles all ADC calls
//JTS2doLater: Remove Arduino functions
//JTS2doLater: use interrupts, so ADC calls don't block execution (each conversion takes ~112 us)

#include "libcm.h"

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

int16_t adc_batteryCurrent_Amps(void)
{
	#define NUM_ADCSAMPLES_PER_RESULT 64 //Valid values: 1,2,4,8,16,32,64 //MUST ALSO CHANGE next line!
	#define NUM_ADCSAMPLES_2_TO_THE_N  6 //Valid values: 0,1,2,3, 4, 5, 6 //2^N = NUM_ADCSAMPLES_PER_RESULT
	#define NUM_ADCSAMPLES_PER_CALL    4 //Must be divisible into NUM_ADCSAMPLES_PER_RESULT!
	
	static uint8_t adcSamplesTaken = 0;
	static uint16_t adcAccumulator = 0;

	for(int ii=0; ii<NUM_ADCSAMPLES_PER_CALL; ii++)
	{
		adcAccumulator += analogRead(PIN_BATTCURRENT);
		adcSamplesTaken++;
	}

	static int16_t latest_battCurrent_amps = 0;
	if(adcSamplesTaken == NUM_ADCSAMPLES_PER_RESULT)
	{
		int16_t battCurrent_counts = int16_t( (adcAccumulator >> NUM_ADCSAMPLES_2_TO_THE_N) ); //Shift must match:
		adcAccumulator = 0;
		adcSamplesTaken = 0;
		debugUSB_batteryCurrent_counts(battCurrent_counts);

		//convert current sensor result into approximate amperage for MCM & user-display
		//don't use this result for current accumulation... it's not accurate enough (FYI: SoC accumulates raw ADC result)
		latest_battCurrent_amps = ( (battCurrent_counts * 13) >> 6) - 67; //Accurate to within 3.7 amps of actual value
	}

	return latest_battCurrent_amps;

}