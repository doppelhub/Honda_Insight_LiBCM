//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//maintains battery state of charge

#include "libcm.h"

//These variables should only be used until the next "////////////////////" comment (after that, use these functions instead)
uint16_t stackFull_Calculated_mAh  = STACK_mAh_NOM; //JTS2doLater: LiBCM needs to adjust value after it figures out actual pack mAh
uint16_t packCharge_Now_mAh = 3000; //immediately overwritten if keyOFF when LiBCM turns on
uint8_t  packCharge_Now_percent = (packCharge_Now_mAh * 100) / stackFull_Calculated_mAh;

//SoC calculation is faster when the unit is mAh
uint16_t SoC_getBatteryStateNow_mAh(void) { return packCharge_Now_mAh; }
void     SoC_setBatteryStateNow_mAh(uint16_t newPackCharge_mAh) { packCharge_Now_mAh = newPackCharge_mAh; }

//SoC calculation is slower when the unit is % 
uint8_t SoC_getBatteryStateNow_percent(void) { return packCharge_Now_percent; }
void    SoC_setBatteryStateNow_percent(uint8_t newSoC_percent) { packCharge_Now_mAh = (stackFull_Calculated_mAh * 0.01) * newSoC_percent; } //JTS2doNow: Is this cast correctly?

//uses SoC percentage to update mAh value (LiBCM stores battery SoC in mAh, not %)
void SoC_calculateBatteryStateNow_percent(void) { packCharge_Now_percent = (uint8_t)(((uint32_t)packCharge_Now_mAh * 100) / stackFull_Calculated_mAh); }

/////////////////////////////////////////////////////////////////////

//integrate current over time (coulomb counting)
//LiBCM uses this function to determine SoC while keyON
void SoC_integrateCharge_adcCounts(int16_t adcCounts)
{
	//determine how much 'charge' (in ADC counts) went into the battery
	int16_t deltaCharge_counts = adcCounts - ADC_NOMINAL_0A_COUNTS; //positive during assist, negative during regen //adcCounts already calibrated to zero crossing
	
	//Time for some dimensional analysis!
	//Note: items in brackets are units (e.g. "123 [amps]", "456 [volts]")
	//We know that: 1 [ A] = 1 [ Coulomb] / 1 [ s] 
	//      and so: 1 [mA] = 1 [uCoulomb] / 1 [ms] 
	// If we then divide both sides by 1 mA, we get:
	// 1 [] = (1 [uCoulomb/ms] / 1 [mA])... the equation below includes this version of '1' (to help us cancel units)		
	//then:
	// deltaCharge_uCoulomb [uCoulomb] = (deltaCharge_counts [delta_Counts] / 1) * (ADC_MILLIAMPS_PER_COUNT [mA] / 1 [delta_Counts]) * (1 [uCoulomb/ms] / 1 [mA]) * (time_loopPeriod_ms_get() [ms] / 1)
	//cancel units:
	// deltaCharge_uCoulomb [uCoulomb] = (deltaCharge_counts [            ] / 1) * (ADC_MILLIAMPS_PER_COUNT [  ] / 1 [            ]) * (1 [uCoulomb/  ] / 1 [  ]) * (time_loopPeriod_ms_get() [  ] / 1)
	//therefore:
	// deltaCharge_uCoulomb            =  deltaCharge_counts                     *  ADC_MILLIAMPS_PER_COUNT                                                       *  time_loopPeriod_ms_get()
	//int32_t deltaCharge_uCoulomb = (int32_t)deltaCharge_counts * (ADC_MILLIAMPS_PER_COUNT * time_loopPeriod_ms_get());
	//One final change: The battery current sensor isn't updated every time through the loop... so multiply by the number of loops per result
	int32_t deltaCharge_uCoulomb = (int32_t)deltaCharge_counts * time_loopPeriod_ms_get() * (ADC_MILLIAMPS_PER_COUNT * ADC_NUMLOOPS_PER_RESULT);

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
		if(SoC_getBatteryStateNow_mAh() >     0) { SoC_setBatteryStateNow_mAh( SoC_getBatteryStateNow_mAh() - 1 ); } //pack discharged 1 mAh (assist)
	}

	while(intermediateChargeBuffer_uCoulomb < -ONE_MILLIAMPHOUR_IN_MICROCOULOMBS)
	{	//regen
		intermediateChargeBuffer_uCoulomb += ONE_MILLIAMPHOUR_IN_MICROCOULOMBS; //add 1 mAh to buffer
		if(SoC_getBatteryStateNow_mAh() < 65535) { SoC_setBatteryStateNow_mAh( SoC_getBatteryStateNow_mAh() + 1 ); } //pack charged 1 mAh (regen)
	}
}

/////////////////////////////////////////////////////////////////////

//Estimate SoC based on resting cell voltage.
//LiBCM uses this function to determine SoC while keyOFF
//EHW5 cells settle to final 'resting' voltage in ten minutes, but are fairly close to that value after just one minute
void SoC_updateUsingLatestOpenCircuitVoltage(void)
{
	if(time_toUpdate_keyOffValues() == true)
	{
		uint8_t batterySoC_percent = SoC_estimateFromRestingCellVoltage_percent(); //determine resting SoC

		Serial.print(F("\nOld SoC: "));
		Serial.print( String(SoC_getBatteryStateNow_percent()) );
		Serial.print(F("%, New SoC:"));
		Serial.print(String(batterySoC_percent));
		Serial.print('%');

		SoC_setBatteryStateNow_percent(batterySoC_percent); //update SoC
	}
}

/////////////////////////////////////////////////////////////////////

//Calling this function when battery is sourcing/sinking current will cause estimation error
//Wait at least ten minutes after keyOff for most accurate results
#ifdef BATTERY_TYPE_5AhG3
	uint8_t SoC_estimateFromRestingCellVoltage_percent(void)
	{
		uint16_t restingCellVoltage = LTC68042result_loCellVoltage_get(); //JTS2doNow: need an algorithm to look at hi cell, too.
		uint8_t estimatedSoC = 0;

		if     (restingCellVoltage >= 42000) { estimatedSoC = 100; }
		else if(restingCellVoltage >= 41820) { estimatedSoC =  99; }
		else if(restingCellVoltage >= 41640) { estimatedSoC =  98; }
		else if(restingCellVoltage >= 41460) { estimatedSoC =  97; }
		else if(restingCellVoltage >= 41280) { estimatedSoC =  96; }
		else if(restingCellVoltage >= 41100) { estimatedSoC =  95; }
		else if(restingCellVoltage >= 40980) { estimatedSoC =  94; }
		else if(restingCellVoltage >= 40860) { estimatedSoC =  93; }
		else if(restingCellVoltage >= 40740) { estimatedSoC =  92; }
		else if(restingCellVoltage >= 40620) { estimatedSoC =  91; }
		else if(restingCellVoltage >= 40500) { estimatedSoC =  90; }
		else if(restingCellVoltage >= 40400) { estimatedSoC =  89; }
		else if(restingCellVoltage >= 40300) { estimatedSoC =  88; }
		else if(restingCellVoltage >= 40200) { estimatedSoC =  87; }
		else if(restingCellVoltage >= 40100) { estimatedSoC =  86; }
		else if(restingCellVoltage >= CELL_VREST_85_PERCENT_SoC) { estimatedSoC =  85; } //max cell voltage for long lifetime
		else if(restingCellVoltage >= 39880) { estimatedSoC =  84; }
		else if(restingCellVoltage >= 39760) { estimatedSoC =  83; }
		else if(restingCellVoltage >= 39640) { estimatedSoC =  82; }
		else if(restingCellVoltage >= 39520) { estimatedSoC =  81; }
		else if(restingCellVoltage >= 39400) { estimatedSoC =  80; }
		else if(restingCellVoltage >= 39260) { estimatedSoC =  79; }
		else if(restingCellVoltage >= 39120) { estimatedSoC =  78; }
		else if(restingCellVoltage >= 38980) { estimatedSoC =  77; }
		else if(restingCellVoltage >= 38840) { estimatedSoC =  76; }
		else if(restingCellVoltage >= 38700) { estimatedSoC =  75; }
		else if(restingCellVoltage >= 38620) { estimatedSoC =  74; }
		else if(restingCellVoltage >= 38540) { estimatedSoC =  73; }
		else if(restingCellVoltage >= 38460) { estimatedSoC =  72; }
		else if(restingCellVoltage >= 38380) { estimatedSoC =  71; }
		else if(restingCellVoltage >= 38300) { estimatedSoC =  70; }
		else if(restingCellVoltage >= 38230) { estimatedSoC =  69; }
		else if(restingCellVoltage >= 38160) { estimatedSoC =  68; }
		else if(restingCellVoltage >= 38090) { estimatedSoC =  67; }
		else if(restingCellVoltage >= 38020) { estimatedSoC =  66; }
		else if(restingCellVoltage >= 37950) { estimatedSoC =  65; }
		else if(restingCellVoltage >= 37890) { estimatedSoC =  64; }
		else if(restingCellVoltage >= 37830) { estimatedSoC =  63; }
		else if(restingCellVoltage >= 37770) { estimatedSoC =  62; }
		else if(restingCellVoltage >= 37710) { estimatedSoC =  61; }
		else if(restingCellVoltage >= 37650) { estimatedSoC =  60; }
		else if(restingCellVoltage >= 37580) { estimatedSoC =  59; }
		else if(restingCellVoltage >= 37510) { estimatedSoC =  58; }
		else if(restingCellVoltage >= 37440) { estimatedSoC =  57; }
		else if(restingCellVoltage >= 37370) { estimatedSoC =  56; }
		else if(restingCellVoltage >= 37300) { estimatedSoC =  55; }
		else if(restingCellVoltage >= 37250) { estimatedSoC =  54; }
		else if(restingCellVoltage >= 37200) { estimatedSoC =  53; }
		else if(restingCellVoltage >= 37150) { estimatedSoC =  52; }
		else if(restingCellVoltage >= 37100) { estimatedSoC =  51; }
		else if(restingCellVoltage >= 37050) { estimatedSoC =  50; }
		else if(restingCellVoltage >= 37000) { estimatedSoC =  49; }
		else if(restingCellVoltage >= 36950) { estimatedSoC =  48; }
		else if(restingCellVoltage >= 36900) { estimatedSoC =  47; }
		else if(restingCellVoltage >= 36850) { estimatedSoC =  46; }
		else if(restingCellVoltage >= 36800) { estimatedSoC =  45; }
		else if(restingCellVoltage >= 36750) { estimatedSoC =  44; }
		else if(restingCellVoltage >= 36700) { estimatedSoC =  43; }
		else if(restingCellVoltage >= 36650) { estimatedSoC =  42; }
		else if(restingCellVoltage >= 36600) { estimatedSoC =  41; }
		else if(restingCellVoltage >= 36550) { estimatedSoC =  40; }
		else if(restingCellVoltage >= 36480) { estimatedSoC =  39; }
		else if(restingCellVoltage >= 36410) { estimatedSoC =  38; }
		else if(restingCellVoltage >= 36340) { estimatedSoC =  37; }
		else if(restingCellVoltage >= 36270) { estimatedSoC =  36; }
		else if(restingCellVoltage >= 36200) { estimatedSoC =  35; }
		else if(restingCellVoltage >= 36140) { estimatedSoC =  34; }
		else if(restingCellVoltage >= 36080) { estimatedSoC =  33; }
		else if(restingCellVoltage >= 36020) { estimatedSoC =  32; }
		else if(restingCellVoltage >= 35960) { estimatedSoC =  31; }
		else if(restingCellVoltage >= 35900) { estimatedSoC =  30; }
		else if(restingCellVoltage >= 35800) { estimatedSoC =  29; }
		else if(restingCellVoltage >= 35700) { estimatedSoC =  28; }
		else if(restingCellVoltage >= 35600) { estimatedSoC =  27; }
		else if(restingCellVoltage >= 35500) { estimatedSoC =  26; }
		else if(restingCellVoltage >= 35400) { estimatedSoC =  25; }
		else if(restingCellVoltage >= 35360) { estimatedSoC =  24; }
		else if(restingCellVoltage >= 35320) { estimatedSoC =  23; }
		else if(restingCellVoltage >= 35280) { estimatedSoC =  22; }
		else if(restingCellVoltage >= 35240) { estimatedSoC =  21; }
		else if(restingCellVoltage >= 35200) { estimatedSoC =  20; }
		else if(restingCellVoltage >= 35160) { estimatedSoC =  19; }
		else if(restingCellVoltage >= 35120) { estimatedSoC =  18; }
		else if(restingCellVoltage >= 35080) { estimatedSoC =  17; }
		else if(restingCellVoltage >= 35040) { estimatedSoC =  16; }
		else if(restingCellVoltage >= 35000) { estimatedSoC =  15; }
		else if(restingCellVoltage >= 34900) { estimatedSoC =  14; }
		else if(restingCellVoltage >= 34800) { estimatedSoC =  13; }
		else if(restingCellVoltage >= 34700) { estimatedSoC =  12; }
		else if(restingCellVoltage >= 34540) { estimatedSoC =  11; }
		else if(restingCellVoltage >= CELL_VREST_10_PERCENT_SoC) { estimatedSoC =  10; } //min cell voltage for long lifetime
		else if(restingCellVoltage >= 33900) { estimatedSoC =   9; }
		else if(restingCellVoltage >= 33600) { estimatedSoC =   8; }
		else if(restingCellVoltage >= 33200) { estimatedSoC =   7; }
		else if(restingCellVoltage >= 32800) { estimatedSoC =   6; }
		else if(restingCellVoltage >= 32200) { estimatedSoC =   5; }
		else if(restingCellVoltage >= 31700) { estimatedSoC =   4; }
		else if(restingCellVoltage >= 31000) { estimatedSoC =   3; }
		else if(restingCellVoltage >= 30500) { estimatedSoC =   2; }
		else if(restingCellVoltage >= 29500) { estimatedSoC =   1; }
		else                                 { estimatedSoC =   0; }

		return estimatedSoC;
	}

#elif defined BATTERY_TYPE_47AhFoMoCo

	uint8_t SoC_estimateFromRestingCellVoltage_percent(void)
		{
			uint16_t restingCellVoltage = LTC68042result_loCellVoltage_get(); //JTS2doNow: need an algorithm to look at hi cell, too.
			uint8_t estimatedSoC = 0;

			//~/Honda_Insight_LiBCM/Electronics/Lithium Batteries/47 Ah FoMoCo Modules/Resting SoC Discharge Curve
			if     (restingCellVoltage >= 42000) { estimatedSoC = 100; }
			else if(restingCellVoltage >= 41800) { estimatedSoC =  99; }
			else if(restingCellVoltage >= 41600) { estimatedSoC =  98; }
			else if(restingCellVoltage >= 41400) { estimatedSoC =  97; }
			else if(restingCellVoltage >= 41200) { estimatedSoC =  96; }
			else if(restingCellVoltage >= 41000) { estimatedSoC =  95; }
			else if(restingCellVoltage >= 40880) { estimatedSoC =  94; }
			else if(restingCellVoltage >= 40760) { estimatedSoC =  93; }
			else if(restingCellVoltage >= 40640) { estimatedSoC =  92; }
			else if(restingCellVoltage >= 40520) { estimatedSoC =  91; }
			else if(restingCellVoltage >= 40400) { estimatedSoC =  90; }
			else if(restingCellVoltage >= 40260) { estimatedSoC =  89; }
			else if(restingCellVoltage >= 40120) { estimatedSoC =  88; }
			else if(restingCellVoltage >= 39980) { estimatedSoC =  87; }
			else if(restingCellVoltage >= 39840) { estimatedSoC =  86; }
			else if(restingCellVoltage >= CELL_VREST_85_PERCENT_SoC) { estimatedSoC =  85; } //max cell voltage for long lifetime
			else if(restingCellVoltage >= 39600) { estimatedSoC =  84; }
			else if(restingCellVoltage >= 39500) { estimatedSoC =  83; }
			else if(restingCellVoltage >= 39400) { estimatedSoC =  82; }
			else if(restingCellVoltage >= 39300) { estimatedSoC =  81; }
			else if(restingCellVoltage >= 39200) { estimatedSoC =  80; }
			else if(restingCellVoltage >= 39100) { estimatedSoC =  79; }
			else if(restingCellVoltage >= 39000) { estimatedSoC =  78; }
			else if(restingCellVoltage >= 38900) { estimatedSoC =  77; }
			else if(restingCellVoltage >= 38800) { estimatedSoC =  76; }
			else if(restingCellVoltage >= 38700) { estimatedSoC =  75; }
			else if(restingCellVoltage >= 38600) { estimatedSoC =  74; }
			else if(restingCellVoltage >= 38500) { estimatedSoC =  73; }
			else if(restingCellVoltage >= 38400) { estimatedSoC =  72; }
			else if(restingCellVoltage >= 38300) { estimatedSoC =  71; }
			else if(restingCellVoltage >= 38200) { estimatedSoC =  70; }
			else if(restingCellVoltage >= 38100) { estimatedSoC =  69; }
			else if(restingCellVoltage >= 38000) { estimatedSoC =  68; }
			else if(restingCellVoltage >= 37900) { estimatedSoC =  67; }
			else if(restingCellVoltage >= 37800) { estimatedSoC =  66; }
			else if(restingCellVoltage >= 37700) { estimatedSoC =  65; }
			else if(restingCellVoltage >= 37580) { estimatedSoC =  64; }
			else if(restingCellVoltage >= 37460) { estimatedSoC =  63; }
			else if(restingCellVoltage >= 37340) { estimatedSoC =  62; }
			else if(restingCellVoltage >= 37220) { estimatedSoC =  61; }
			else if(restingCellVoltage >= 37100) { estimatedSoC =  60; }
			else if(restingCellVoltage >= 37020) { estimatedSoC =  59; }
			else if(restingCellVoltage >= 36940) { estimatedSoC =  58; }
			else if(restingCellVoltage >= 36860) { estimatedSoC =  57; }
			else if(restingCellVoltage >= 36780) { estimatedSoC =  56; }
			else if(restingCellVoltage >= 36770) { estimatedSoC =  55; }
			else if(restingCellVoltage >= 36640) { estimatedSoC =  54; }
			else if(restingCellVoltage >= 36580) { estimatedSoC =  53; }
			else if(restingCellVoltage >= 36520) { estimatedSoC =  52; }
			else if(restingCellVoltage >= 36460) { estimatedSoC =  51; }
			else if(restingCellVoltage >= 36400) { estimatedSoC =  50; }
			else if(restingCellVoltage >= 36360) { estimatedSoC =  49; }
			else if(restingCellVoltage >= 36320) { estimatedSoC =  48; }
			else if(restingCellVoltage >= 36280) { estimatedSoC =  47; }
			else if(restingCellVoltage >= 36240) { estimatedSoC =  46; }
			else if(restingCellVoltage >= 36200) { estimatedSoC =  45; }
			else if(restingCellVoltage >= 36160) { estimatedSoC =  44; }
			else if(restingCellVoltage >= 36120) { estimatedSoC =  43; }
			else if(restingCellVoltage >= 36080) { estimatedSoC =  42; }
			else if(restingCellVoltage >= 36040) { estimatedSoC =  41; }
			else if(restingCellVoltage >= 36000) { estimatedSoC =  40; }
			else if(restingCellVoltage >= 35960) { estimatedSoC =  39; }
			else if(restingCellVoltage >= 35920) { estimatedSoC =  38; }
			else if(restingCellVoltage >= 35880) { estimatedSoC =  37; }
			else if(restingCellVoltage >= 35840) { estimatedSoC =  36; }
			else if(restingCellVoltage >= 35800) { estimatedSoC =  35; }
			else if(restingCellVoltage >= 35760) { estimatedSoC =  34; }
			else if(restingCellVoltage >= 35720) { estimatedSoC =  33; }
			else if(restingCellVoltage >= 35680) { estimatedSoC =  32; }
			else if(restingCellVoltage >= 35640) { estimatedSoC =  31; }
			else if(restingCellVoltage >= 35600) { estimatedSoC =  30; }
			else if(restingCellVoltage >= 35540) { estimatedSoC =  29; }
			else if(restingCellVoltage >= 35480) { estimatedSoC =  28; }
			else if(restingCellVoltage >= 35420) { estimatedSoC =  27; }
			else if(restingCellVoltage >= 35360) { estimatedSoC =  26; }
			else if(restingCellVoltage >= 35300) { estimatedSoC =  25; }
			else if(restingCellVoltage >= 35240) { estimatedSoC =  24; }
			else if(restingCellVoltage >= 35180) { estimatedSoC =  23; }
			else if(restingCellVoltage >= 35120) { estimatedSoC =  22; }
			else if(restingCellVoltage >= 35060) { estimatedSoC =  21; }
			else if(restingCellVoltage >= 35000) { estimatedSoC =  20; }
			else if(restingCellVoltage >= 34880) { estimatedSoC =  19; }
			else if(restingCellVoltage >= 34760) { estimatedSoC =  18; }
			else if(restingCellVoltage >= 34640) { estimatedSoC =  17; }
			else if(restingCellVoltage >= 34520) { estimatedSoC =  16; }
			else if(restingCellVoltage >= 34400) { estimatedSoC =  15; }
			else if(restingCellVoltage >= 34320) { estimatedSoC =  14; }
			else if(restingCellVoltage >= 34240) { estimatedSoC =  13; }
			else if(restingCellVoltage >= 34160) { estimatedSoC =  12; }
			else if(restingCellVoltage >= 34080) { estimatedSoC =  11; }
			else if(restingCellVoltage >= CELL_VREST_10_PERCENT_SoC) { estimatedSoC =  10; } //min cell voltage for long lifetime
			else if(restingCellVoltage >= 33900) { estimatedSoC =   9; }
			else if(restingCellVoltage >= 33800) { estimatedSoC =   8; }
			else if(restingCellVoltage >= 33700) { estimatedSoC =   7; }
			else if(restingCellVoltage >= 33600) { estimatedSoC =   6; }
			else if(restingCellVoltage >= 33200) { estimatedSoC =   5; }
			else if(restingCellVoltage >= 32600) { estimatedSoC =   4; }
			else if(restingCellVoltage >= 32000) { estimatedSoC =   3; }
			else if(restingCellVoltage >= 31000) { estimatedSoC =   2; }
			else if(restingCellVoltage >= 30000) { estimatedSoC =   1; }
			else                                 { estimatedSoC =   0; }

			return estimatedSoC;
		}
#endif	

/////////////////////////////////////////////////////////////////////

void SoC_handler(void)
{
	SoC_calculateBatteryStateNow_percent();
}

/////////////////////////////////////////////////////////////////////

/*

JTS2doLater:
-Save SoC(RAM) to EEPROM
-If value in RAM is more than 10% different from EEPROM value, update EEPROM.
-Also store if key recently turned off (keyState_previous == on && keyState_now == off)

*/