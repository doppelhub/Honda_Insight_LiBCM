//Copyright 2023-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles pack heater (if installed)

#include "libcm.h"

uint8_t heaterInstalled = NO;

////////////////////////////////////////////////////////////////////////////////////

uint8_t heater_isInstalled(void) { return heaterInstalled; }

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

uint8_t hasEnoughTimePassedToChangeState(void)
{
	static uint32_t timestamp_latestChange_ms = 0;

	static uint8_t pinState_helper = gpio_getPinState(PIN_GPIO3_HEATER);
	       uint8_t pinState_now    = gpio_getPinState(PIN_GPIO3_HEATER);

	uint8_t hasEnoughTimePassed = YES;

	if(pinState_now != pinState_helper)
	{
		//heater state changed
		if( (millis() - timestamp_latestChange_ms) < HEATER_STATE_CHANGE_HYSTERESIS_ms ) { hasEnoughTimePassed = NO; }
		else
		{
			// enough time has passed
			timestamp_latestChange_ms = millis();
			pinState_helper = pinState_now;
		}
	}

	return hasEnoughTimePassed;
}

////////////////////////////////////////////////////////////////////////////////////

uint8_t heater_isPackTooHot(void)
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
	{
		//heater not allowed
		gpio_turnPackHeater_off();
	}
	else //heater is allowed
	{
		if(hasEnoughTimePassedToChangeState() == YES)
		{
			if( temperature_battery_getLatest() < temperature_heatBatteryBelow_C() ) { gpio_turnPackHeater_on();  } //pack is     cold
			else                                                                     { gpio_turnPackHeater_off(); } //pack is not cold
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Turn all discharger resistors on when pack is heating.