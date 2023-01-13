//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#include "libcm.h"

char goalSpeed[NUM_FAN_CONTROLLERS] = {'0'}; //store desired fan speeds (for OEM and PCB fans)
static char actualSpeed[NUM_FAN_CONTROLLERS] = {'0'};

uint8_t fanStates[NUM_FAN_CONTROLLERS] = {FAN_NOT_REQUESTED}; //each subsystem's fan speed request is stored in 2 bits

//JTS2doLater: Turn the fan on when the car is on and the battery temp isn't ideal (assumes cabin air temp is habitable)

////////////////////////////////////////////////////////////////////////////////////

//prevents rapid fan speed changes //Each fan (OEM and PCB) has its own controller
void fanSpeedController(uint8_t whichFan)
{
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
			if( (uint32_t)(millis() - timestamp_latestFanSpeedChange_ms[whichFan]) > FAN_HYSTERESIS_ms)
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

int8_t fan_getBatteryCoolSetpoint_C(void)
{
	int8_t coolBattAboveTemp_C = ROOM_TEMP_DEGC;

	if     (key_getSampledState()         == KEYSTATE_ON)    { coolBattAboveTemp_C = COOL_BATTERY_ABOVE_TEMP_C_KEYON; }
	else if(gpio_isGridChargerPluggedInNow() == PLUGGED_IN)  { coolBattAboveTemp_C = COOL_BATTERY_ABOVE_TEMP_C_GRIDCHARGING; }
	else if( (SoC_getBatteryStateNow_percent() > KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC) &&
		     (key_getSampledState() == KEYSTATE_OFF) )       { coolBattAboveTemp_C = COOL_BATTERY_ABOVE_TEMP_C_KEYOFF; }
	else /*KEYOFF && SoC too low*/                           { coolBattAboveTemp_C = TEMPERATURE_SENSOR_FAULT_HI; }

	return coolBattAboveTemp_C;
}

////////////////////////////////////////////////////////////////////////////////////

int8_t fan_getBatteryHeatSetpoint_C(void)
{
	int8_t heatBattBelowTemp_C = ROOM_TEMP_DEGC;

	if     (key_getSampledState() == KEYSTATE_ON)           { heatBattBelowTemp_C = HEAT_BATTERY_BELOW_TEMP_C_KEYON; }
	else if(gpio_isGridChargerPluggedInNow() == PLUGGED_IN) { heatBattBelowTemp_C = HEAT_BATTERY_BELOW_TEMP_C_GRIDCHARGING; }
	else if( (SoC_getBatteryStateNow_percent() > KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC) &&
		     (key_getSampledState() == KEYSTATE_OFF) )      { heatBattBelowTemp_C = HEAT_BATTERY_BELOW_TEMP_C_KEYOFF; }
	else /*KEYOFF && SoC too low*/                          { heatBattBelowTemp_C = TEMPERATURE_SENSOR_FAULT_LO; }

	return heatBattBelowTemp_C;
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

////////////////////////////////////////////////////////////////////////////////////

char fan_getCurrentSpeed(uint8_t whichFan) {
	return actualSpeed[whichFan];
}

////////////////////////////////////////////////////////////////////////////////////

int8_t calculateAbsoluteDelta(int8_t temperatureA, int8_t temperatureB)
{
	int8_t absoluteDelta = 0;

	if(temperatureA > temperatureB) { absoluteDelta = temperatureA - temperatureB; }
	else                            { absoluteDelta = temperatureB - temperatureA; }

	return absoluteDelta;
}

////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Add option to see who is requesting fan state
void fan_handler(void)
{
	//JTS2doLater: Rewrite this handler entirely... it's not good.  Proposed framework:
	//If pack too warm or too cold, wait a minute after keyON, then turn fan on briefly to sample cabin air temp.
	//           If cabin air temp undesirable, turn fan off and wait a few more minutes.
	//           Once cabin air temp is good, turn fans on and heat/cool pack.

	// if(getPackTemp() > (COOL_PACK_ABOVE_TEMP_DEGC + FAN_HIGH_SPEED_degC))
	// {
	// 	//pack is very hot
	// 	timeSinceFansLastActivated_ms = millis();
	// }

	// else if (getPackTemp() < HEAT_PACK_BELOW_TEMP_DEGC)
	// {
	// 	//pack is too cold
	// 	timeSinceFansLastActivated_ms = millis();
	// }

	// else
	// {
	// 	//pack temperature is "just right"

	// 	if( (millis() - timeSinceFansLastActivated_ms) > FIVE_MINUTES_IN_MILLISECONDS)
	// 	{
	// 		if(fanSpeed == FAN_SPEED_HI) { fanSpeedSet(FAN_SPEED_LOW); }
	// 		if(fanSpeed == FAN_SPEED_LO) { fanSpeedSet(FAN_SPEED_OFF); }

	// 	}
	// }

	int8_t battTemp   = temperature_battery_getLatest();
	int8_t intakeTemp = temperature_intake_getLatest();

	static int8_t   battTemp_lastFanStateUpdate = ROOM_TEMP_DEGC;
	static int8_t intakeTemp_lastFanStateUpdate = ROOM_TEMP_DEGC;

	int8_t deltaAbs_battTemp   = calculateAbsoluteDelta(battTemp,     battTemp_lastFanStateUpdate);
	int8_t deltaAbs_intakeTemp = calculateAbsoluteDelta(intakeTemp, intakeTemp_lastFanStateUpdate);

	static uint32_t timeSinceLastFanCheck_ms = 0;

	if( (deltaAbs_battTemp   >= FAN_HYSTERESIS_degC) || //battery temperature sensor value has changed more than a few degrees
		(deltaAbs_intakeTemp >= FAN_HYSTERESIS_degC) || //intake  temperature sensor value has changed more than a few degrees
		((uint32_t)(millis() - timeSinceLastFanCheck_ms) > FORCE_FAN_UPDATE_PERIOD_ms) )
	{
		//intake or battery temperature changed enough to check for possible new fan state

		//store latest temperatures (for future comparisons)
		  battTemp_lastFanStateUpdate = battTemp;
		intakeTemp_lastFanStateUpdate = intakeTemp;

		timeSinceLastFanCheck_ms = millis();

		uint8_t fanSpeed = FAN_OFF;

		//cool pack if too warm
		if(battTemp > ROOM_TEMP_DEGC)
		{
			int8_t coolBatteryAboveTemp_C = fan_getBatteryCoolSetpoint_C();

			if(battTemp >= (temperature_intake_getLatest() + AIR_TEMP_DELTA_TO_RUN_FANS) )
			{
				//battery is warmer than intake air
				if     (battTemp >= (coolBatteryAboveTemp_C + FAN_HIGH_SPEED_degC)) { fanSpeed = FAN_HIGH; }
				else if(battTemp >= (coolBatteryAboveTemp_C                      )) { fanSpeed = FAN_LOW;  }
			}
		}
		else //heat pack if too cold
		{
			int8_t heatBatteryBelowTemp_C = fan_getBatteryHeatSetpoint_C();

			if(battTemp <= (temperature_intake_getLatest() - AIR_TEMP_DELTA_TO_RUN_FANS) )
			{
				//battery is cooler than intake air
				if     (battTemp <= (heatBatteryBelowTemp_C - FAN_HIGH_SPEED_degC)) { fanSpeed = FAN_HIGH; }
				else if(battTemp <= (heatBatteryBelowTemp_C                      )) { fanSpeed = FAN_LOW;  }
			}
		}

		//request temperature based fan speed
		fan_requestSpeed(FAN_PCB, FAN_REQUESTOR_TEMPERATURE, fanSpeed);
		#ifdef OEM_FAN_INSTALLED
			//Fan positive is unpowered when keyOFF, so no need to check key state
			fan_requestSpeed(FAN_OEM, FAN_REQUESTOR_TEMPERATURE, fanSpeed);
		#endif
	}

	if(gpio_isGridChargerChargingNow() == true) { fan_requestSpeed(FAN_PCB, FAN_REQUESTOR_GRIDCHARGER, FAN_HIGH); }
	else                                        { fan_requestSpeed(FAN_PCB, FAN_REQUESTOR_GRIDCHARGER, FAN_OFF ); }

	fanSpeedController(FAN_OEM);
	fanSpeedController(FAN_PCB);
}

////////////////////////////////////////////////////////////////////////////////////
