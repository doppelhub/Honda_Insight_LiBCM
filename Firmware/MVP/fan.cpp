//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#include "libcm.h"

char goalSpeed[NUM_FAN_CONTROLLERS] = {'0'}; //store desired fan speeds (for OEM and PCB fans)

uint8_t fanStates[NUM_FAN_CONTROLLERS] = {FAN_NOT_REQUESTED}; //each subsystem's fan speed request is stored in 2 bits

////////////////////////////////////////////////////////////////////////////////////

//maintains the state of each fan speed controller (OEM & PCB fans)
void fanSpeedController(uint8_t whichFan)
{
	static char actualSpeed[NUM_FAN_CONTROLLERS] = {'0'};
	static uint32_t timestamp_latestFanSpeedChange_ms[NUM_FAN_CONTROLLERS] = {0};
	
	if(actualSpeed[whichFan] != goalSpeed[whichFan])
	{
		uint8_t changeFanSpeedNow = NO;

		if( ((actualSpeed[whichFan] == '0')                                ) || //speed changing from off to either low or high
			((actualSpeed[whichFan] == 'L') && (goalSpeed[whichFan] == 'H'))  ) //speed changing from low to high
		{
			//fan speed is increasing //change speed immediately
			changeFanSpeedNow = YES;
		}
		else //(fan speed is decreasing)
		{
			if( (millis() - timestamp_latestFanSpeedChange_ms[whichFan]) > FAN_HYSTERESIS_ms)
			{
				//hysteresis delay period has passed
				changeFanSpeedNow = YES;
			}
		}

		if(changeFanSpeedNow == YES)
		{
			if(whichFan == FAN_PCB) { gpio_setFanSpeed_PCB(goalSpeed[whichFan]); }
			if(whichFan == FAN_OEM) { gpio_setFanSpeed_OEM(goalSpeed[whichFan]); }
			actualSpeed[whichFan] = goalSpeed[whichFan];
			timestamp_latestFanSpeedChange_ms[whichFan] = millis();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
void fan_handler(void)
{
	fanSpeedController(FAN_OEM);
	fanSpeedController(FAN_PCB);
}

////////////////////////////////////////////////////////////////////////////////////

void fan_requestSpeed(uint8_t whichFan, uint8_t requestor, char newFanSpeed)
{
	uint8_t fanSpeedOtherSubsystems = fanStates[whichFan] & ~(0b11 << requestor); //mask out requestor's previous state
	fanStates[whichFan] = (newFanSpeed<<requestor) | fanSpeedOtherSubsystems;

	if     (fanStates[whichFan] & FAN_HI_MASK) { goalSpeed[whichFan] = 'H'; } //at least one subsystem is requesting high speed
	else if(fanStates[whichFan] & FAN_LO_MASK) { goalSpeed[whichFan] = 'L'; } //at least one subsystem is requesting low speed
	else                                       { goalSpeed[whichFan] = '0'; } //nobody is requesting fan
}