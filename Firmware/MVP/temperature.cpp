//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//measures OEM temperature sensors
//LTC6804 temperature sensors are measured in LTC68042gpio.cpp

#include "libcm.h"

int8_t tempBattery = ROOM_TEMP_DEGC;
int8_t tempIntake  = ROOM_TEMP_DEGC;
int8_t tempExhaust = ROOM_TEMP_DEGC;
int8_t tempCharger = ROOM_TEMP_DEGC;
int8_t tempAmbient = ROOM_TEMP_DEGC;

////////////////////////////////////////////////////////////////////////////////////

int8_t temperature_battery_getLatest(void)    { return tempBattery; }
int8_t temperature_intake_getLatest(void)     { return tempIntake;  } //GRN OEM temp sensor
int8_t temperature_exhaust_getLatest(void)    { return tempExhaust; } //YEL OEM temp sensor
int8_t temperature_gridCharger_getLatest(void){ return tempCharger; } //BLU OEM temp sensor
int8_t temperature_ambient_getLatest(void)	  { return tempAmbient; } //WHT OEM temp sensor

////////////////////////////////////////////////////////////////////////////////////

//only call inside handler (to ensure sensors powered)
void temperature_measureOEM(void)
{
	#ifdef BATTERY_TYPE_5AhG3
		tempIntake  = temperature_measureOneSensor_degC(PIN_TEMP_GRN);
		tempExhaust = temperature_measureOneSensor_degC(PIN_TEMP_YEL);
		tempCharger = temperature_measureOneSensor_degC(PIN_TEMP_BLU);
		tempAmbient = temperature_measureOneSensor_degC(PIN_TEMP_WHT);
	#elif defined BATTERY_TYPE_47AhFoMoCo
		tempIntake  = temperature_measureOneSensor_degC(PIN_TEMP_WHT);
		tempExhaust = temperature_measureOneSensor_degC(PIN_TEMP_GRN); //JTS2doNow: Actually top rear battery module
		tempCharger = temperature_measureOneSensor_degC(PIN_TEMP_BLU);
		tempAmbient = temperature_measureOneSensor_degC(PIN_TEMP_YEL); //JTS2doNow: Acutually top middle battery module
	#endif
}

////////////////////////////////////////////////////////////////////////////////////

//only call inside handler (to ensure sensors powered)
//stores the most extreme battery temperature (from room temp) in tempBattery
//LiBCM has QTY3 battery temperature sensors
//JTS2doNow: Add FoMoCo hardware configuration
void temperature_measureBattery(void)
{
	int8_t batteryTemps[NUM_BATTERY_TEMP_SENSORS + 1] = {0}; //1-indexed ([1] = bay1 temp)

	batteryTemps[1] = temperature_measureOneSensor_degC(PIN_TEMP_BAY1);
	batteryTemps[2] = temperature_measureOneSensor_degC(PIN_TEMP_BAY2);
	batteryTemps[3] = temperature_measureOneSensor_degC(PIN_TEMP_BAY3);

	//stores hottest and coldest temp sensor value
	int8_t tempHi = TEMPERATURE_SENSOR_FAULT_LO; //highest measured temp is initially set to the  lowest possible temp
	int8_t tempLo = TEMPERATURE_SENSOR_FAULT_HI; // lowest measured temp is initially set to the highest possible temp

	for(uint8_t ii = 1; ii <= NUM_BATTERY_TEMP_SENSORS; ii++)
	{
		if( (batteryTemps[ii] == TEMPERATURE_SENSOR_FAULT_HI) ||
			(batteryTemps[ii] == TEMPERATURE_SENSOR_FAULT_LO)  )
		{
			Serial.print(F("\nCheck Batt Temp Sensor! Bay: "));
			Serial.print(String(ii,DEC));
		}
		else
		{
			//this sensor returned valid data
			//only valid sensors are considered
			if(batteryTemps[ii] > tempHi) { tempHi = batteryTemps[ii]; } //find hi temp
			if(batteryTemps[ii] < tempLo) { tempLo = batteryTemps[ii]; } //find lo temp
		}
	}

	//find hi & lo magnitudes from ROOM_TEMP_DEGC
	int8_t tempHiDelta = 0;
	int8_t tempLoDelta = 0;

	if(tempHi > ROOM_TEMP_DEGC) { tempHiDelta = tempHi - ROOM_TEMP_DEGC; }
	else                        { tempHiDelta = ROOM_TEMP_DEGC - tempHi; }

	if(tempLo > ROOM_TEMP_DEGC) { tempLoDelta = tempLo - ROOM_TEMP_DEGC; }
	else                        { tempLoDelta = ROOM_TEMP_DEGC - tempLo; }

	//figure out which magnitude is further from ROOM_TEMP_DEGC 
	if(tempHiDelta > tempLoDelta) { tempBattery = tempHi; }
	else                          { tempBattery = tempLo; }
}

////////////////////////////////////////////////////////////////////////////////////

void temperature_printAll_latest(void)
{
	Serial.print(F("\nTemperatures(C):"));
	Serial.print(F("\nGrid: "));
	Serial.print(temperature_gridCharger_getLatest());
	Serial.print(F("\nIn: "));
	Serial.print(temperature_intake_getLatest());
	Serial.print(F("\nAmb: "));
	Serial.print(temperature_ambient_getLatest());
	Serial.print(F("\nOut: "));
	Serial.print(temperature_exhaust_getLatest());
	Serial.print(F("\nBatt: "));
	Serial.print(temperature_battery_getLatest());
}

////////////////////////////////////////////////////////////////////////////////////

//JTS2doNow: add separate case for FoMoCo
void temperature_measureAndPrintAll(void)
{
	if(gpio_getPinState(PIN_TEMP_EN) == PIN_OUTPUT_HIGH)
	{		
		Serial.print(F("\nTemperatures(C):"));
		Serial.print(F("\nBLU: "));
		Serial.print(temperature_measureOneSensor_degC(PIN_TEMP_BLU));
		Serial.print(F("\nGRN: "));
		Serial.print(temperature_measureOneSensor_degC(PIN_TEMP_GRN));
		Serial.print(F("\nWHT: "));
		Serial.print(temperature_measureOneSensor_degC(PIN_TEMP_WHT));
		Serial.print(F("\nYEL: "));
		Serial.print(temperature_measureOneSensor_degC(PIN_TEMP_YEL));
		Serial.print(F("\nBAY1: "));
		Serial.print(temperature_measureOneSensor_degC(PIN_TEMP_BAY1));
		Serial.print(F("\nBAY2: "));
		Serial.print(temperature_measureOneSensor_degC(PIN_TEMP_BAY2));
		Serial.print(F("\nBAY3: "));
		Serial.print(temperature_measureOneSensor_degC(PIN_TEMP_BAY3));	
	}
	else
	{
		gpio_turnTemperatureSensors_on();
		Serial.print(F("\nturned sensors on. Repeat command to display temp."));
	}
}

////////////////////////////////////////////////////////////////////////////////////

int8_t temperature_coolBatteryAbove_C(void)
{
	int8_t coolBattAboveTemp_C = ROOM_TEMP_DEGC;

	if     (key_getSampledState() == KEYSTATE_ON)            { coolBattAboveTemp_C = COOL_BATTERY_ABOVE_TEMP_C_KEYON;        }
	else if(gpio_isGridChargerPluggedInNow() == YES)         { coolBattAboveTemp_C = COOL_BATTERY_ABOVE_TEMP_C_GRIDCHARGING; }
	else if( (SoC_getBatteryStateNow_percent() > KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC) &&
		     (key_getSampledState() == KEYSTATE_OFF) )       { coolBattAboveTemp_C = COOL_BATTERY_ABOVE_TEMP_C_KEYOFF;       }
	else /*KEYOFF && SoC too low*/                           { coolBattAboveTemp_C = TEMPERATURE_SENSOR_FAULT_HI;            } //don't request fan if SoC low

	if     (fan_getSpeed_now() == FAN_HIGH) { coolBattAboveTemp_C -= FAN_SPEED_HYSTERESIS_HIGH_degC; }
	else if(fan_getSpeed_now() == FAN_LOW ) { coolBattAboveTemp_C -= FAN_SPEED_HYSTERESIS_LOW_degC;  }
	else if(fan_getSpeed_now() == FAN_OFF ) { coolBattAboveTemp_C -= FAN_SPEED_HYSTERESIS_OFF_degC;  }

	return coolBattAboveTemp_C;
}

////////////////////////////////////////////////////////////////////////////////////

int8_t temperature_heatBatteryBelow_C(void)
{
	int8_t heatBattBelowTemp_C = ROOM_TEMP_DEGC;

	if     (key_getSampledState() == KEYSTATE_ON)           { heatBattBelowTemp_C = HEAT_BATTERY_BELOW_TEMP_C_KEYON;        }
	else if(gpio_isGridChargerPluggedInNow() == YES)        { heatBattBelowTemp_C = HEAT_BATTERY_BELOW_TEMP_C_GRIDCHARGING; }
	else if( (SoC_getBatteryStateNow_percent() > KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC) &&
		     (key_getSampledState() == KEYSTATE_OFF) )      { heatBattBelowTemp_C = HEAT_BATTERY_BELOW_TEMP_C_KEYOFF;       }
	else /*KEYOFF && SoC too low*/                          { heatBattBelowTemp_C = TEMPERATURE_SENSOR_FAULT_LO;            } //don't request fan if SoC low

	if     (fan_getSpeed_now() == FAN_HIGH) { heatBattBelowTemp_C += FAN_SPEED_HYSTERESIS_HIGH_degC; }
	else if(fan_getSpeed_now() == FAN_LOW ) { heatBattBelowTemp_C += FAN_SPEED_HYSTERESIS_LOW_degC;  }
	else if(fan_getSpeed_now() == FAN_OFF ) { heatBattBelowTemp_C += FAN_SPEED_HYSTERESIS_OFF_degC;  }

	return heatBattBelowTemp_C;
}

////////////////////////////////////////////////////////////////////////////////////

uint8_t turnSensorsOff_whenKeyStateChanges(uint8_t keyState)
{
	static uint8_t keyState_previous = KEYSTATE_OFF;

	if(keyState_previous != keyState) { keyState_previous = keyState; return TEMPSENSORSTATE_TURNOFF; } //key state changed (either KeyON->OFF or KeyOFF->ON)
	else                              {                               return keyState;                } //key state didn't change //return input state
}

////////////////////////////////////////////////////////////////////////////////////

uint32_t tempSensorUpdateInterval_ms(void)
{
	uint32_t updatePeriod_ms = 0;

	if     (key_getSampledState()            == KEYSTATE_ON) { updatePeriod_ms = TEMP_UPDATE_PERIOD_KEYON_ms;        }
	else if(gpio_isGridChargerPluggedInNow() == YES        ) { updatePeriod_ms = TEMP_UPDATE_PERIOD_GRIDCHARGING_ms; }
	else                                                     { updatePeriod_ms = TEMP_UPDATE_PERIOD_KEYOFF_ms;       }

	return updatePeriod_ms;
}

////////////////////////////////////////////////////////////////////////////////////

void temperature_handler(void)
{
	static uint8_t tempSensorState = TEMPSENSORSTATE_TURNON; //state machine

	uint8_t keyState_Now = key_getSampledState(); //prevent mid-loop key state change

	keyState_Now = turnSensorsOff_whenKeyStateChanges(keyState_Now);
	
	static uint32_t latestTempMeasurement_ms = 0;
	static uint32_t latestSensorTurnon_ms = 0;

	if(tempSensorState == TEMPSENSORSTATE_TURNON)
	{
		gpio_turnTemperatureSensors_on(); 
		latestSensorTurnon_ms = millis();
		tempSensorState = TEMPSENSORSTATE_POWERUP;
	}

	else if(tempSensorState == TEMPSENSORSTATE_POWERUP)       
	{
		uint32_t timeSinceSensorTurnedOn = (millis() - latestSensorTurnon_ms);

		if(timeSinceSensorTurnedOn > TEMP_POWERUP_DELAY_ms) { tempSensorState = TEMPSENSORSTATE_MEASURE; }
		//else { ; } //do nothing //we're still waiting for sensors to powerup
	}

	else if(tempSensorState == TEMPSENSORSTATE_MEASURE)
	{
		temperature_measureBattery();
		temperature_measureOEM();
		latestTempMeasurement_ms = millis();

		if( (keyState_Now == KEYSTATE_ON) || (gpio_isGridChargerPluggedInNow() == YES) ) { tempSensorState = TEMPSENSORSTATE_STAYON;  }
		else                                                                             { tempSensorState = TEMPSENSORSTATE_TURNOFF; }
	}	

	else if(tempSensorState == TEMPSENSORSTATE_TURNOFF)
	{
		//sensors only turn off when key is off and grid charger is unplugged
		Serial.print(F("\nTemp(C): ")); //print temp when key is off
		Serial.print(String(tempBattery));
		gpio_turnTemperatureSensors_off(); 
		tempSensorState = TEMPSENSORSTATE_OFF;
	}

	//else if(tempSensorState == TEMPSENSORSTATE_STAYON) { ; } //handled (eventually) inside next state

	else if( (millis() - latestTempMeasurement_ms) > tempSensorUpdateInterval_ms() )
	{
		//it's time to measure temperature sensors
		if     (tempSensorState == TEMPSENSORSTATE_STAYON)  { tempSensorState = TEMPSENSORSTATE_MEASURE; } //sensors stay on when keyON (see logic above)
		else                                                { tempSensorState = TEMPSENSORSTATE_TURNON;  } //sensors turn off when keyOFF //need to turn them on
	}

	// else if(tempSensorState == TEMPSENSORSTATE_OFF) { ; } //nothing to do
	// else                                            { ; } //nothing to do
}

////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Need to differentiate between TEMPERATURE_SENSOR_FAULT_LO and actually being below -30 degC
int8_t temperature_measureOneSensor_degC(uint8_t thermistorPin)
{			
	uint16_t countsADC = analogRead(thermistorPin); //measure ADC counts

	//This commented out section is quite math intensive:
	// -QTY3 floating point divisions
	// -QTY1 natural log (+5kB to load library)
	// -QTY3 multiplies
	// -QTY3 add/subtract 
		// #define TEMP_BALANCE_RESISTANCE_OHMS 10000
		// if(countsADC > 887) { countsADC = 887; } //prevent overflowing uint16_t in the next equation

		// //ADC measures voltage divider with 10k high leg & R_NTC bottom leg.
		// //voltage divider equation is:
		// // voltageADC = (voltageVCC * resistanceRTD) / (resistanceRTD + TEMP_BALANCE_RESISTANCE_OHMS)
		// //Reorganizing these terms for resistanceRTD:
		// // resistanceRTD = (voltageADC * TEMP_BALANCE_RESISTANCE_OHMS) / (voltageVCC - voltageADC)
		// //Change to counts (ratiometric, so allowed):
		// #define ADC_COUNTS_AT_VCC 1023
		// // resistanceThermistor_ohms = ( countsADC * TEMP_BALANCE_RESISTANCE_OHMS) / (counts_VCC - countsADC )
		// uint16_t resistanceThermistor_ohms = ( countsADC * TEMP_BALANCE_RESISTANCE_OHMS) / (ADC_COUNTS_AT_VCC - countsADC); //if countsADC exceeds 887 this will overflow
		
		// //Steinhart-Hart Beta Equation:
		// #define THERMISTOR_BETA 3982 //from datasheet
		// #define THERMISTOR_RESISTANCE_23DEGC 10000 // half ADC range since both resistors are 10kOhm at room temperature
		// #define TEMP_23DEGC_KELVIN 296
		// // 1/T_sensed_kelvin = 1/T_room + (1/beta) * ln(R_measured / R_room)
		// //Re-arrange terms:
		// uint16_t tempMeasured_kelvin = (THERMISTOR_BETA * THERMISTOR_RESISTANCE_23DEGC) / (THERMISTOR_BETA + (THERMISTOR_RESISTANCE_23DEGC * log(resistanceThermistor_ohms / TEMP_23DEGC_KELVIN) ) );
		// int8_t tempMeasured_celsius = T_sensed_kelvin - 273
	//
	//Rather than do the above math, LiBCM uses a lookup table (derived from the above equations):

	int16_t tempMeasured_celsius = 0;
	if     (countsADC > 1000) { tempMeasured_celsius = TEMPERATURE_SENSOR_FAULT_LO; } //sensor unplugged //or VERY cold
	else if(countsADC >  971) { tempMeasured_celsius = -30; } //MCM expecting uint8_t, where T_MCM = T_actual + 30 //So MCM can only receive down to -30 degC
	else if(countsADC >  968) { tempMeasured_celsius = -29; }
	else if(countsADC >  964) { tempMeasured_celsius = -28; }
	else if(countsADC >  961) { tempMeasured_celsius = -27; }
	else if(countsADC >  957) { tempMeasured_celsius = -26; }
	else if(countsADC >  953) { tempMeasured_celsius = -25; }
	else if(countsADC >  948) { tempMeasured_celsius = -24; }
	else if(countsADC >  944) { tempMeasured_celsius = -23; }
	else if(countsADC >  939) { tempMeasured_celsius = -22; }
	else if(countsADC >  934) { tempMeasured_celsius = -21; }

	else if(countsADC >  929) { tempMeasured_celsius = -20; }
	else if(countsADC >  923) { tempMeasured_celsius = -19; }
	else if(countsADC >  917) { tempMeasured_celsius = -18; }
	else if(countsADC >  911) { tempMeasured_celsius = -17; }
	else if(countsADC >  905) { tempMeasured_celsius = -16; }
	else if(countsADC >  899) { tempMeasured_celsius = -15; }
	else if(countsADC >  892) { tempMeasured_celsius = -14; }
	else if(countsADC >  885) { tempMeasured_celsius = -13; }
	else if(countsADC >  878) { tempMeasured_celsius = -12; }
	else if(countsADC >  871) { tempMeasured_celsius = -11; }

	else if(countsADC >  863) { tempMeasured_celsius = -10; }
	else if(countsADC >  855) { tempMeasured_celsius =  -9; }
	else if(countsADC >  847) { tempMeasured_celsius =  -8; }
	else if(countsADC >  839) { tempMeasured_celsius =  -7; }
	else if(countsADC >  830) { tempMeasured_celsius =  -6; }
	else if(countsADC >  821) { tempMeasured_celsius =  -5; }
	else if(countsADC >  812) { tempMeasured_celsius =  -4; }
	else if(countsADC >  803) { tempMeasured_celsius =  -3; }
	else if(countsADC >  794) { tempMeasured_celsius =  -2; }
	else if(countsADC >  784) { tempMeasured_celsius =  -1; }

	else if(countsADC >  774) { tempMeasured_celsius =   0; }
	else if(countsADC >  764) { tempMeasured_celsius =   1; }
	else if(countsADC >  753) { tempMeasured_celsius =   2; }
	else if(countsADC >  743) { tempMeasured_celsius =   3; }
	else if(countsADC >  732) { tempMeasured_celsius =   4; }
	else if(countsADC >  721) { tempMeasured_celsius =   5; }
	else if(countsADC >  710) { tempMeasured_celsius =   6; }
	else if(countsADC >  699) { tempMeasured_celsius =   7; }
	else if(countsADC >  688) { tempMeasured_celsius =   8; }
	else if(countsADC >  676) { tempMeasured_celsius =   9; }

	else if(countsADC >  665) { tempMeasured_celsius =  10; }
	else if(countsADC >  653) { tempMeasured_celsius =  11; }
	else if(countsADC >  641) { tempMeasured_celsius =  12; }
	else if(countsADC >  630) { tempMeasured_celsius =  13; }
	else if(countsADC >  618) { tempMeasured_celsius =  14; }
	else if(countsADC >  606) { tempMeasured_celsius =  15; }
	else if(countsADC >  594) { tempMeasured_celsius =  16; }
	else if(countsADC >  582) { tempMeasured_celsius =  17; }
	else if(countsADC >  570) { tempMeasured_celsius =  18; }
	else if(countsADC >  559) { tempMeasured_celsius =  19; }

	else if(countsADC >  547) { tempMeasured_celsius =  20; }
	else if(countsADC >  535) { tempMeasured_celsius =  21; }
	else if(countsADC >  523) { tempMeasured_celsius =  22; }
	else if(countsADC >  512) { tempMeasured_celsius =  23; }
	else if(countsADC >  500) { tempMeasured_celsius =  24; }
	else if(countsADC >  488) { tempMeasured_celsius =  25; }
	else if(countsADC >  477) { tempMeasured_celsius =  26; }
	else if(countsADC >  466) { tempMeasured_celsius =  27; }
	else if(countsADC >  455) { tempMeasured_celsius =  28; }
	else if(countsADC >  444) { tempMeasured_celsius =  29; }

	else if(countsADC >  433) { tempMeasured_celsius =  30; }
	else if(countsADC >  422) { tempMeasured_celsius =  31; }
	else if(countsADC >  411) { tempMeasured_celsius =  32; }
	else if(countsADC >  401) { tempMeasured_celsius =  33; }
	else if(countsADC >  391) { tempMeasured_celsius =  34; }
	else if(countsADC >  380) { tempMeasured_celsius =  35; }
	else if(countsADC >  371) { tempMeasured_celsius =  36; }
	else if(countsADC >  361) { tempMeasured_celsius =  37; }
	else if(countsADC >  351) { tempMeasured_celsius =  38; }
	else if(countsADC >  342) { tempMeasured_celsius =  39; }

	else if(countsADC >  333) { tempMeasured_celsius =  40; }
	else if(countsADC >  324) { tempMeasured_celsius =  41; }
	else if(countsADC >  315) { tempMeasured_celsius =  42; }
	else if(countsADC >  305) { tempMeasured_celsius =  43; }
	else if(countsADC >  298) { tempMeasured_celsius =  44; }
	else if(countsADC >  290) { tempMeasured_celsius =  45; }
	else if(countsADC >  281) { tempMeasured_celsius =  46; }
	else if(countsADC >  273) { tempMeasured_celsius =  47; }
	else if(countsADC >  265) { tempMeasured_celsius =  48; }
	else if(countsADC >  258) { tempMeasured_celsius =  49; }

	else if(countsADC >  251) { tempMeasured_celsius =  50; }
	else if(countsADC >  244) { tempMeasured_celsius =  51; }
	else if(countsADC >  237) { tempMeasured_celsius =  52; }
	else if(countsADC >  230) { tempMeasured_celsius =  53; }
	else if(countsADC >  223) { tempMeasured_celsius =  54; }
	else if(countsADC >  217) { tempMeasured_celsius =  55; }
	else if(countsADC >  211) { tempMeasured_celsius =  56; }
	else if(countsADC >  205) { tempMeasured_celsius =  57; }
	else if(countsADC >  199) { tempMeasured_celsius =  58; }
	else if(countsADC >  193) { tempMeasured_celsius =  59; }

	else if(countsADC >  188) { tempMeasured_celsius =  60; }
	else if(countsADC >  182) { tempMeasured_celsius =  61; }
	else if(countsADC >  177) { tempMeasured_celsius =  62; }
	else if(countsADC >  172) { tempMeasured_celsius =  63; }
	else if(countsADC >  167) { tempMeasured_celsius =  64; }
	else if(countsADC >  162) { tempMeasured_celsius =  65; }
	else if(countsADC >  157) { tempMeasured_celsius =  66; }
	else if(countsADC >  152) { tempMeasured_celsius =  67; }
	else if(countsADC >  148) { tempMeasured_celsius =  68; }
	else if(countsADC >  144) { tempMeasured_celsius =  69; }

	else if(countsADC >  140) { tempMeasured_celsius =  70; }
	else if(countsADC >  136) { tempMeasured_celsius =  71; }
	else if(countsADC >  132) { tempMeasured_celsius =  72; }
	else if(countsADC >  128) { tempMeasured_celsius =  73; }
	else if(countsADC >  124) { tempMeasured_celsius =  74; }
	else if(countsADC >  121) { tempMeasured_celsius =  75; }
	else                      { tempMeasured_celsius =  TEMPERATURE_SENSOR_FAULT_HI; } //sensor shorted

	//correct for OEM temperature sensor's (unknown) k-coefficients
	//empirical data: ~/GitHub/Honda_Insight_LiBCM/Firmware/MVP/Calculations/OEM thermistor scaling.ods
	if( ((tempMeasured_celsius > TEMPERATURE_SENSOR_FAULT_LO ) && (tempMeasured_celsius < TEMPERATURE_SENSOR_FAULT_HI )) &&
		((thermistorPin == PIN_TEMP_GRN) || (thermistorPin == PIN_TEMP_BLU) || (thermistorPin == PIN_TEMP_YEL) || (thermistorPin == PIN_TEMP_WHT)) ) 
	{
		tempMeasured_celsius = ((tempMeasured_celsius * 5) >> 2) - 5; //actual: countsADC = countsADC * 1.225 - 4;
	}

	return (uint8_t)tempMeasured_celsius;
}

////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater:
//Read QTY5 onboard LTC6804 temps