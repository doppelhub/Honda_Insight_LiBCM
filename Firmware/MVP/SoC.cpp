//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//maintains battery state of charge

#include "libcm.h"

uint16_t stackFull_Calculated_mAh  = (STACK_mAh * 0.01) * STACK_SoC_MAX;
uint16_t stackEmpty_Calculated_mAh = (STACK_mAh * 0.01) * STACK_SoC_MIN;
uint16_t packCharge_Now_mAh = 3000;
uint8_t  packCharge_Now_percent = (packCharge_Now_mAh * 100) / stackFull_Calculated_mAh;

//Estimate SoC based on resting cell voltage.
//EHW5 cells settle to final 'resting' voltage in ten minutes, but are fairly close to that value after just one minute

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

//SoC is calculated faster when the unit is mAh
uint16_t SoC_getBatteryStateNow_mAh(void) { return packCharge_Now_mAh; }
void     SoC_setBatteryStateNow_mAh(uint16_t newPackCharge_mAh) { packCharge_Now_mAh = newPackCharge_mAh; }

//SoC is calculated slower when the unit is % 
uint8_t SoC_getBatteryStateNow_percent(void) { return packCharge_Now_percent; }
void    SoC_setBatteryStateNow_percent(uint8_t newSoC_percent) { packCharge_Now_mAh = newSoC_percent * 0.01 * stackFull_Calculated_mAh; } //JTS2doNow: Is this cast correctly?

/////////////////////////////////////////////////////////////////////

void SoC_updateUsingOpenCircuitVoltage(void)
{
	LTC68042cell_sampleGatherAndProcessAllCellVoltages(); //get latest cell data

	uint8_t batterySoC_percent = SoC_estimateFromRestingCellVoltage_percent(); //determine resting SoC

	Serial.print(F("\nOld SoC: "));
	Serial.print( String(SoC_getBatteryStateNow_percent()) );
	Serial.print(F("%, New SoC:"));
	Serial.print(String(batterySoC_percent));
	Serial.print('%');

	SoC_setBatteryStateNow_percent(batterySoC_percent); //update SoC
}

/////////////////////////////////////////////////////////////////////

//JTS2doNow: Don't update SoC until the key has been off for at least ten minutes.  This prevents recent current from influencing resting battery voltage 
//only call this function when the key is off (or you'll get a check engine light)
void SoC_openCircuitVoltage_handler(void)
{
	#define KEY_OFF_SoC_UPDATE_PERIOD_MINUTES 10 //JTS2doNow: How long to wait?
	#define KEY_OFF_SoC_UPDATE_PERIOD_MILLISECONDS (KEY_OFF_SoC_UPDATE_PERIOD_MINUTES * 60000)

	static uint32_t SoC_latestUpdate_milliseconds = 0;

	if( (millis() - SoC_latestUpdate_milliseconds ) >= KEY_OFF_SoC_UPDATE_PERIOD_MILLISECONDS)
	{
		SoC_latestUpdate_milliseconds = millis();
		
		debugUSB_displayUptime_seconds();

		SoC_updateUsingOpenCircuitVoltage();

		//turn LiBCM off if pack SoC is low
		//LiBCM will power back on when the key is turned on
		if( LTC68042result_loCellVoltage_get() < CELL_VMIN_KEYOFF)
		{
			gpio_turnBuzzer_on_highFreq();
			delay(100);
			gpio_turnLiBCM_off(); //game over, thanks for playing 
			while(1) { ; } //LiBCM takes a bit to turn off... wait here until that happens
		}
	}
}

/////////////////////////////////////////////////////////////////////

//Don't call this function when current is flowing through the current sensor
//Wait at least ten minutes after keyOff for most accurate results
uint8_t SoC_estimateFromRestingCellVoltage_percent(void)
{
	uint16_t restingCellVoltage = LTC68042result_loCellVoltage_get(); //JTS2doNow: need an algorithm to look at hi cell, too.
	uint8_t estimatedSoC = 0;

	//JTS2doLater: piecewise linearize this massive conditional into fewer cases
	// if     (restingVoltage>4.0 volts) { SoC = (restingVoltage-4.0)*PIECEWISE_SLOPE_ABOVE_4000_mV + 4.000; }
	// else if(restingVoltage>3.8 volts) { SoC = (restingVoltage-3.8)*PIECEWISE_SLOPE_ABOVE_3800_mV + 3.800; }
	// else if(restingVoltage>3.5 volts) { SoC = (restingVoltage-3.5)*PIECEWISE_SLOPE_ABOVE_3500_mV + 3.500; }
	//Note: I sure wish lithium cell voltages were linear!
	if     (restingCellVoltage >= 42000) { estimatedSoC = 100; }
	else if(restingCellVoltage >= 41750) { estimatedSoC =  99; }
	else if(restingCellVoltage >= 41500) { estimatedSoC =  98; }
	else if(restingCellVoltage >= 41350) { estimatedSoC =  97; }
	else if(restingCellVoltage >= 41200) { estimatedSoC =  96; }
	else if(restingCellVoltage >= 41100) { estimatedSoC =  95; }
	else if(restingCellVoltage >= 41000) { estimatedSoC =  94; }
	else if(restingCellVoltage >= 40900) { estimatedSoC =  93; }
	else if(restingCellVoltage >= 40800) { estimatedSoC =  92; }
	else if(restingCellVoltage >= 40700) { estimatedSoC =  91; }
	else if(restingCellVoltage >= 40600) { estimatedSoC =  90; }
	else if(restingCellVoltage >= 40500) { estimatedSoC =  89; }
	else if(restingCellVoltage >= 40400) { estimatedSoC =  88; }
	else if(restingCellVoltage >= 40300) { estimatedSoC =  87; }
	else if(restingCellVoltage >= 40200) { estimatedSoC =  86; }
	else if(restingCellVoltage >= 40100) { estimatedSoC =  85; }
	else if(restingCellVoltage >= 40000) { estimatedSoC =  84; }
	else if(restingCellVoltage >= 39900) { estimatedSoC =  83; }
	else if(restingCellVoltage >= 39800) { estimatedSoC =  82; }
	else if(restingCellVoltage >= 39700) { estimatedSoC =  81; }
	else if(restingCellVoltage >= 39600) { estimatedSoC =  80; }
	else if(restingCellVoltage >= 39500) { estimatedSoC =  79; }
	else if(restingCellVoltage >= 39375) { estimatedSoC =  78; }
	else if(restingCellVoltage >= 39250) { estimatedSoC =  77; }
	else if(restingCellVoltage >= 39125) { estimatedSoC =  76; }
	else if(restingCellVoltage >= 39000) { estimatedSoC =  75; }
	else if(restingCellVoltage >= 38900) { estimatedSoC =  74; }
	else if(restingCellVoltage >= 38800) { estimatedSoC =  73; }
	else if(restingCellVoltage >= 38700) { estimatedSoC =  72; }
	else if(restingCellVoltage >= 38600) { estimatedSoC =  71; }
	else if(restingCellVoltage >= 38500) { estimatedSoC =  70; }
	else if(restingCellVoltage >= 38400) { estimatedSoC =  69; }
	else if(restingCellVoltage >= 38300) { estimatedSoC =  68; }
	else if(restingCellVoltage >= 38225) { estimatedSoC =  67; }
	else if(restingCellVoltage >= 38150) { estimatedSoC =  66; }
	else if(restingCellVoltage >= 38075) { estimatedSoC =  65; }
	else if(restingCellVoltage >= 38000) { estimatedSoC =  64; }
	else if(restingCellVoltage >= 37925) { estimatedSoC =  63; }
	else if(restingCellVoltage >= 37850) { estimatedSoC =  62; }
	else if(restingCellVoltage >= 37800) { estimatedSoC =  61; }
	else if(restingCellVoltage >= 37750) { estimatedSoC =  60; }
	else if(restingCellVoltage >= 37700) { estimatedSoC =  59; }
	else if(restingCellVoltage >= 37650) { estimatedSoC =  58; }
	else if(restingCellVoltage >= 37575) { estimatedSoC =  57; }
	else if(restingCellVoltage >= 37500) { estimatedSoC =  56; }
	else if(restingCellVoltage >= 37450) { estimatedSoC =  55; }
	else if(restingCellVoltage >= 37400) { estimatedSoC =  54; }
	else if(restingCellVoltage >= 37350) { estimatedSoC =  53; }
	else if(restingCellVoltage >= 37275) { estimatedSoC =  52; }
	else if(restingCellVoltage >= 37200) { estimatedSoC =  51; }
	else if(restingCellVoltage >= 37150) { estimatedSoC =  50; }
	else if(restingCellVoltage >= 37100) { estimatedSoC =  49; }
	else if(restingCellVoltage >= 37050) { estimatedSoC =  48; }
	else if(restingCellVoltage >= 37000) { estimatedSoC =  47; }
	else if(restingCellVoltage >= 36950) { estimatedSoC =  46; }
	else if(restingCellVoltage >= 36900) { estimatedSoC =  45; }
	else if(restingCellVoltage >= 36850) { estimatedSoC =  44; }
	else if(restingCellVoltage >= 36800) { estimatedSoC =  43; }
	else if(restingCellVoltage >= 36750) { estimatedSoC =  42; }
	else if(restingCellVoltage >= 36700) { estimatedSoC =  41; }
	else if(restingCellVoltage >= 36650) { estimatedSoC =  40; }
	else if(restingCellVoltage >= 36600) { estimatedSoC =  39; }
	else if(restingCellVoltage >= 36550) { estimatedSoC =  38; }
	else if(restingCellVoltage >= 36475) { estimatedSoC =  37; }
	else if(restingCellVoltage >= 36400) { estimatedSoC =  36; }
	else if(restingCellVoltage >= 36350) { estimatedSoC =  35; }
	else if(restingCellVoltage >= 36300) { estimatedSoC =  34; }
	else if(restingCellVoltage >= 36250) { estimatedSoC =  33; }
	else if(restingCellVoltage >= 36200) { estimatedSoC =  32; }
	else if(restingCellVoltage >= 36125) { estimatedSoC =  31; }
	else if(restingCellVoltage >= 36050) { estimatedSoC =  30; }
	else if(restingCellVoltage >= 36000) { estimatedSoC =  29; }
	else if(restingCellVoltage >= 35925) { estimatedSoC =  28; }
	else if(restingCellVoltage >= 35850) { estimatedSoC =  27; }
	else if(restingCellVoltage >= 35775) { estimatedSoC =  26; }
	else if(restingCellVoltage >= 35700) { estimatedSoC =  25; }
	else if(restingCellVoltage >= 35600) { estimatedSoC =  24; }
	else if(restingCellVoltage >= 35500) { estimatedSoC =  23; }
	else if(restingCellVoltage >= 35400) { estimatedSoC =  22; }
	else if(restingCellVoltage >= 35300) { estimatedSoC =  21; }
	else if(restingCellVoltage >= 35200) { estimatedSoC =  20; }
	else if(restingCellVoltage >= 35150) { estimatedSoC =  19; }
	else if(restingCellVoltage >= 35100) { estimatedSoC =  18; }
	else if(restingCellVoltage >= 35050) { estimatedSoC =  17; }
	else if(restingCellVoltage >= 35000) { estimatedSoC =  16; }
	else if(restingCellVoltage >= 34950) { estimatedSoC =  15; }
	else if(restingCellVoltage >= 34900) { estimatedSoC =  14; }
	else if(restingCellVoltage >= 34850) { estimatedSoC =  13; }
	else if(restingCellVoltage >= 34800) { estimatedSoC =  12; }
	else if(restingCellVoltage >= 34750) { estimatedSoC =  11; }
	else if(restingCellVoltage >= 34700) { estimatedSoC =  10; }
	else if(restingCellVoltage >= 34400) { estimatedSoC =   9; }
	else if(restingCellVoltage >= 34100) { estimatedSoC =   8; }
	else if(restingCellVoltage >= 33700) { estimatedSoC =   7; }
	else if(restingCellVoltage >= 33200) { estimatedSoC =   6; }
	else if(restingCellVoltage >= 32700) { estimatedSoC =   5; }
	else if(restingCellVoltage >= 32200) { estimatedSoC =   4; }
	else if(restingCellVoltage >= 31500) { estimatedSoC =   3; }
	else if(restingCellVoltage >= 30800) { estimatedSoC =   2; }
	else if(restingCellVoltage >= 30100) { estimatedSoC =   1; }
	else                                 { estimatedSoC =   0; }

	return estimatedSoC;
}

/////////////////////////////////////////////////////////////////////

void SoC_handler(void)
{
	packCharge_Now_percent = (uint8_t)((uint32_t)packCharge_Now_mAh * 100) / stackFull_Calculated_mAh;
}

/////////////////////////////////////////////////////////////////////

/*

-Save SoC(RAM) to EEPROM
-If value in RAM is more than 10% different from EEPROM value, update EEPROM.
-Also store if key recently turned off (keyState_previous == on && keyState_now == off)
-EEPROM writes require 3.3. ms blocking time (interrupts must be disabled)
	-Example EEPROM code p24 MEGA2560 manual

*/