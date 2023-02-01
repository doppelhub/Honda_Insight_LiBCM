//Copyright 2023-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles pack heater (if installed)

#include "libcm.h"

bool heaterInstalled = NO;

////////////////////////////////////////////////////////////////////////////////////

bool heater_isInstalled(void) { return heaterInstalled; }

////////////////////////////////////////////////////////////////////////////////////

//only call this function at powerup
//pin floats when called, which can cause switching FET to continuously operate in active region (bad)
void heater_init(void)
{
	pinMode(PIN_GPIO3_HEATER,INPUT_PULLUP);

	if(digitalRead(PIN_GPIO3_HEATER) == false) { heaterInstalled = YES; } //if connected, the isolated driver on the heater PCB will pull signal low
	else                                       { heaterInstalled = NO;  } //if heater PCB disconnected, the CPU pullup will pull signal high

	digitalWrite(PIN_GPIO3_HEATER,LOW); //turn heater off
	pinMode(PIN_GPIO3_HEATER,INPUT); //switch pin to input (redundant, for safety)
}

////////////////////////////////////////////////////////////////////////////////////

bool hasEnoughTimePassedToChangeState(void)
{
	static uint32_t timestamp_latestChange_ms = 0;

	bool hasEnoughTimePassed = NO;

	if( (millis() - timestamp_latestChange_ms) > HEATER_STATE_CHANGE_HYSTERESIS_ms)
	{
		hasEnoughTimePassed = YES;
		timestamp_latestChange_ms = millis();
	}
	
	return hasEnoughTimePassed;
}

////////////////////////////////////////////////////////////////////////////////////

bool heater_isPackTooHot(void)
{
	if( (temperature_battery_getLatest() > FORCE_HEATER_OFF_ABOVE_TEMP_C) ||
		(temperature_battery_getLatest() == TEMPERATURE_SENSOR_FAULT_LO)   ) //assume pack too hot if temp sensors disconnected
	     { return YES; }
	else { return NO;  }
}

////////////////////////////////////////////////////////////////////////////////////

void heater_handler(void)
{
	if( (SoC_isThermalManagementAllowed() == NO) || //not enough energy to heat pack
		(heater_isInstalled() == NO            ) || //heater not installed         
	    (heater_isPackTooHot() == YES)            ) //pack is too hot
	{ gpio_turnPackHeater_off(); } //heater not allowed

	else //heater is allowed
	{
		if(hasEnoughTimePassedToChangeState() == YES)
		{
			if( temperature_battery_getLatest() < temperature_heatBatteryBelow_C() ) { gpio_turnPackHeater_on();  Serial.print('H'); } //pack is     cold //JTS2doNow: remove debug 'H'
			else                                                                     { gpio_turnPackHeater_off(); Serial.print('h'); } //pack is not cold
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Turn all discharger resistors on when pack is heating.