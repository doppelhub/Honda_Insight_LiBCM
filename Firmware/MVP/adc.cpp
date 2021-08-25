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
	int16_t battCurrent_amps;
	uint16_t ADC_oversampledAccumulator = 0;

	//JTS2doLater: Convert to rolling average (only get one ADC value per run).
	for(int ii=0; ii<64; ii++)  //takes 7.2 ms to get QTY64 conversions
	{
		ADC_oversampledAccumulator += analogRead(PIN_BATTCURRENT);
	}

	int16_t ADC_oversampledResult = int16_t( (ADC_oversampledAccumulator >> 6) );
	Serial.print(F("\nADC:"));
	Serial.print( String(ADC_oversampledResult) );

	//convert current sensor result into approximate amperage for MCM & user-display
	//don't use this result for current accumulation... it's not accurate enough (FYI: SoC accumulates raw ADC result)
	battCurrent_amps = ( (ADC_oversampledResult * 13) >> 6) - 67; //Accurate to within 3.7 amps of actual value

	Serial.print(F(", "));
	Serial.print( String(battCurrent_amps) );
	Serial.print(F(" A(raw), "));
	
	return battCurrent_amps;
}