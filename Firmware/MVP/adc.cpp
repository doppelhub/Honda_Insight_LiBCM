//handles all ADC calls
//JTS2do: Remove Arduino functions
//JTS2do: use interrupts, so ADC calls don't block execution (each conversion takes ~112 us)

#include "libcm.h"


uint16_t adc_VPIN_raw(void)
{
	uint16_t VPIN_result = analogRead(PIN_VPIN_IN);	
	return VPIN_result;
}

/////////////////////////////////////////////////////////////////////

int16_t adc_batteryCurrent_Amps(void)
{
	int16_t battCurrent_amps;
	uint16_t ADC_oversampledAccumulator = 0;

	//JTS2do: Remove if() statement after code only gets one ADC result per call
	if( vPackSpoof_isVpinSpoofed() == true )
	{
		//JTS2do: Convert to rolling average (only get one ADC value per run).
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

		if( ENABLE_CURRENT_HACK )
		{
			battCurrent_amps = (int16_t)(battCurrent_amps * 0.7); //140% current hack = tell MCM 70% actual
		}

		Serial.print( String(battCurrent_amps) );
		Serial.print(F(" A(MCM)"));
	} 
	else
	{
		battCurrent_amps = 0; //we don't have time to get battery current with above implementation
	}
	return battCurrent_amps;
}