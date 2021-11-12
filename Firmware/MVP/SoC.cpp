//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//maintains battery state of charge

#include "libcm.h"

uint32_t packCharge_Now_uAh = 3E6;
uint32_t packCharge_Full_uAh = 5E6; 

void SoC_integrateCharge_adcCounts(int16_t adcCounts)
{
	static uint32_t previousMillis = 0;

	//determine how much 'charge' went into the battery (unit: 'counts')
	int16_t deltaCharge_counts = adcCounts - ADC_NOMINAL_0A_COUNTS; //positive during assist, negative during regen
	
	//Time for some dimensional analysis!
	//Note: items in brackets are units (e.g. "123 [amps]", "456 [volts]")
	//We know that: 1 [ A] = 1 [ Coulomb] / 1 [ s] 
	//      and so: 1 [mA] = 1 [uCoulomb] / 1 [ms] 
	// If we divide both sides by 1 mA, we get:
	// 1 [] = (1 [uCoulomb/ms] / 1 [mA])... the equation below includes this version of '1' (to help us cancel units)		
	//then:
	// deltaCharge_uCoulomb [uCoulomb] = (deltaCharge_counts [delta_Counts] / 1) * (ADC_MILLIAMPS_PER_COUNT [mA] / 1 [delta_Counts]) * (1 [uCoulomb/ms] / 1 [mA]) * (LOOP_RATE_MILLISECONDS [ms] / 1)
	//cancel units:
	// deltaCharge_uCoulomb [uCoulomb] = (deltaCharge_counts [            ] / 1) * (ADC_MILLIAMPS_PER_COUNT [  ] / 1 [            ]) * (1 [uCoulomb/  ] / 1 [  ]) * (LOOP_RATE_MILLISECONDS [  ] / 1)
	//therefore:
	// deltaCharge_uCoulomb            =  deltaCharge_counts                     *  ADC_MILLIAMPS_PER_COUNT                                                       *  LOOP_RATE_MILLISECONDS
	int32_t deltaCharge_uCoulomb = (uint32_t)deltaCharge_counts * (ADC_MILLIAMPS_PER_COUNT * LOOP_RATE_MILLISECONDS);
	//JTS2doLater: Replace LOOPRATE with [ millis() - millisPrevious() ] //increases SoC accuracy

	//Notes:
	//5 Ah is 18.0E9 uCoulombs, whereas 2^32 is ~4.3E9 counts... so uint32_t isn't large enough for our battery charge accumulator...
	//...but is large enough for our instantaneous deltaCharge_uCoulomb variable:
	//deltaCharge_uCoulomb (max) = (2^10 - ADC_NOMINCAL_0A_COUNT) * ADC_MILLIAMPS_PER_COUNT * LOOPRATE
	//deltaCharge_uCoulomb (max) = (1023 -                   330) *                     215 *       10
	//deltaCharge_uCoulomb (max) = 1,489,950
	//...which fits into our uint32_t several thousand times over.

	//JTS2doNow: Every time we get 1 coulomb(?) of charge, we need to convert it to uAh

	//Convert to uAh (micro ampere hours)
	//We know that: 1  Ah = 3600  Coulombs
	//      and so: 1 uAh = 3600 uCoulombs
	//Therefore:
	//if we divide deltaCharge_uCoulomb by 3600, we'll get the number of uAh:
	//      deltaCharge_microAmpHours =   deltaCharge_uCoulomb / 3600          // 1/3600 ~= 73/(2^18)
	//      deltaCharge_microAmpHours =   deltaCharge_uCoulomb * 73 / (2^18)
	int16_t deltaCharge_microAmpHours = ((deltaCharge_uCoulomb * 73) >> 18); 

	//Notes:
	//5 Ah is 5E6 uAh, so we need to store packCharge_Now_uAh in uint32_t...
	//...but we can store deltaCharge_microAmpHours as int16_t:
	//deltaCharge_microAmpHours (max) = deltaCharge_uCoulomb / 3600
	//deltaCharge_microAmpHours (max) = 414
	//...which easily fits in our int16_t.

	//Update battery charge
	packCharge_Now_uAh += deltaCharge_microAmpHours;

	if( (millis() - previousMillis) >= 500 )
	{
		Serial.print(F("\ncurrent_counts: "));
		Serial.print(String(adcCounts));
		Serial.print(F(", delta uC: "));
		Serial.print(String(deltaCharge_uCoulomb));
		Serial.print(F(", delta uAh: "));
		Serial.print(String(deltaCharge_microAmpHours));
		Serial.print(F(", pack uAh: "));
		Serial.print(String(packCharge_Now_uAh));


		previousMillis = millis();
	}

}

void stateOfCharge_handler(void)
{
	
	;
	//JTS2doLater: turn LiBCM off if any cell drops below 2.75 volts
}

/*

-coulombCount
	-Runs once per second
	Subtract "zero amp" ADC constant
	Accumulate with latest sample.
	32b allows full assist for 18 hours without overflowing when sampled once per second.

-Save SoC(RAM) to EEPROM
-If value in RAM is more than 10% different from EEPROM value, update EEPROM.
-Also store if key recently turned off (keyState_previous == on && keyState_now == off)
-EEPROM writes require 3.3. ms blocking time (interrupts must be disabled)
	-Example EEPROM code p24 MEGA2560 manual

*/

//JTS2doLater: Disable regen & grid charger if any cell drops below some minimum 'safe' voltage