//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//maintains battery state of charge

#include "libcm.h"

void stateOfCharge_handler(void)
{
	;
	//JTS2doLater: add accumulator.
	//JTS2doLater: turn off LiBCM if voltage too low
	//JTS2doLater: discharge pack if too full (e.g. run fans)

	//JTS2doNow: turn LiBCM off if any cell drops below 2.75 volts
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