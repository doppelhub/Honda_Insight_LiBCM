//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//maintains battery state of charge

#include "libcm.h"

uint16_t packCharge_Now_mAh = 3E3; //JTS2doLater: will uint16 work with Leaf battery?
uint16_t packCharge_Full_mAh = 5E3; 

void SoC_integrateCharge_adcCounts(int16_t adcCounts)
{
	//determine how much 'charge' (in ADC counts) went into the battery
	int16_t deltaCharge_counts = adcCounts - ADC_NOMINAL_0A_COUNTS; //positive during assist, negative during regen
	
	//Time for some dimensional analysis!
	//Note: items in brackets are units (e.g. "123 [amps]", "456 [volts]")
	//We know that: 1 [ A] = 1 [ Coulomb] / 1 [ s] 
	//      and so: 1 [mA] = 1 [uCoulomb] / 1 [ms] 
	// If we then divide both sides by 1 mA, we get:
	// 1 [] = (1 [uCoulomb/ms] / 1 [mA])... the equation below includes this version of '1' (to help us cancel units)		
	//then:
	// deltaCharge_uCoulomb [uCoulomb] = (deltaCharge_counts [delta_Counts] / 1) * (ADC_MILLIAMPS_PER_COUNT [mA] / 1 [delta_Counts]) * (1 [uCoulomb/ms] / 1 [mA]) * (LOOP_RATE_MILLISECONDS [ms] / 1)
	//cancel units:
	// deltaCharge_uCoulomb [uCoulomb] = (deltaCharge_counts [            ] / 1) * (ADC_MILLIAMPS_PER_COUNT [  ] / 1 [            ]) * (1 [uCoulomb/  ] / 1 [  ]) * (LOOP_RATE_MILLISECONDS [  ] / 1)
	//therefore:
	// deltaCharge_uCoulomb            =  deltaCharge_counts                     *  ADC_MILLIAMPS_PER_COUNT                                                       *  LOOP_RATE_MILLISECONDS
	//int32_t deltaCharge_uCoulomb = (int32_t)deltaCharge_counts * (ADC_MILLIAMPS_PER_COUNT * LOOP_RATE_MILLISECONDS);
	//One final change: The battery current sensor isn't updated every time through the loop... so multiply by the number of loops per result
	int32_t deltaCharge_uCoulomb = (int32_t)deltaCharge_counts * (ADC_MILLIAMPS_PER_COUNT * LOOP_RATE_MILLISECONDS * ADC_NUMLOOPS_PER_RESULT);

	//Notes:
	//5 Ah is 18.0E9 uCoulombs, whereas 2^32 is ~4.3E9 counts...
	//so uint32_t isn't large enough to store the entire battery's charge...
	//so instead we'll store the total battery charge in microAmpHours (1 uAh = 3600 uC)...
	//and we'll decriment an intermediate uCoulomb buffer to zero
	static int32_t intermediateChargeBuffer_uCoulomb = 0;

	intermediateChargeBuffer_uCoulomb += deltaCharge_uCoulomb; //gets more positive during assist //gets more negative during regen

	#define ONE_MILLIAMPHOUR_IN_MICROCOULOMBS 3600000

	while(intermediateChargeBuffer_uCoulomb > ONE_MILLIAMPHOUR_IN_MICROCOULOMBS )
	{	//assist
		intermediateChargeBuffer_uCoulomb -= ONE_MILLIAMPHOUR_IN_MICROCOULOMBS;  //remove 1 mAh from buffer
		if(packCharge_Now_mAh >     0) { packCharge_Now_mAh -= 1; } //pack discharged 1 mAh (assist)
	}

	while(intermediateChargeBuffer_uCoulomb < -ONE_MILLIAMPHOUR_IN_MICROCOULOMBS)
	{	//regen
		intermediateChargeBuffer_uCoulomb += ONE_MILLIAMPHOUR_IN_MICROCOULOMBS; //add 1 mAh to buffer
		if(packCharge_Now_mAh < 65535) { packCharge_Now_mAh += 1; } //pack charged 1 mAh (regen)
	}

}

/////////////////////////////////////////////////////////////////////

uint16_t SoC_packCharge_Now_mAh_get(void) { return packCharge_Now_mAh; }

/////////////////////////////////////////////////////////////////////

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