//Copyright 2023-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles pack heater (if installed)

#include "libcm.h"

uint8_t heaterLocation = HEATER_NOT_CONNECTED;

//JTS2doLater: Add indicator to 4x20 when heater is off/on

////////////////////////////////////////////////////////////////////////////////////

uint8_t heater_isConnected(void) { return heaterLocation; }

////////////////////////////////////////////////////////////////////////////////////

bool isHeaterConnectedtoPin(int16_t pinToTest)
{
	uint8_t initialPinMode = gpio_getPinMode(pinToTest);
	bool isHeaterConnectedToThisPin = NO;

	pinMode(pinToTest,INPUT_PULLUP);

	if(digitalRead(pinToTest) == false)
	{
		isHeaterConnectedToThisPin = YES; //if connected, the isolated driver on the heater PCB will pull signal low
		digitalWrite(pinToTest,LOW); //turn heater off
		pinMode(pinToTest,INPUT); //switch pin to input (redundant, for safety)
	} 
	else { pinMode(pinToTest,initialPinMode); } //heater not connected to this pin

	return isHeaterConnectedToThisPin;
}

////////////////////////////////////////////////////////////////////////////////////

//determine if heater installed, and if so, which GPIO pin it's connected to
void heater_begin(void)
{
	if(isHeaterConnectedtoPin(PIN_GPIO1) == true) { heaterLocation = HEATER_CONNECTED_DAUGHTERBOARD;   }
	if(isHeaterConnectedtoPin(PIN_GPIO3) == true) { heaterLocation = HEATER_CONNECTED_DIRECT_TO_LICBM; }
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
	if( (SoC_isThermalManagementAllowed() == NO)       || //not enough energy to heat pack
		(heater_isConnected() == HEATER_NOT_CONNECTED) || //heater not installed         
	    (heater_isPackTooHot() == YES)                  ) //pack is too hot
	{ gpio_turnPackHeater_off(); } //heater not allowed

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