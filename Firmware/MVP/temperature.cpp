//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//measures OEM temperature sensors
//LTC6804 temperature sensors are measured in LTC68042gpio.cpp

//FYI: need to enable temperature sensors

#include "libcm.h"

int8_t battTemp = ROOM_TEMP_DEGC;

////////////////////////////////////////////////////////////////////////////////////

int8_t temperature_battery_getLatest() { return battTemp; }

////////////////////////////////////////////////////////////////////////////////////

int8_t temperature_airIntake(void)  { return temperature_measureOneSensor_degC(PIN_TEMP_GRN); }
int8_t temperature_airExhaust(void) { return temperature_measureOneSensor_degC(PIN_TEMP_YEL); }
int8_t temperature_gridCharger(void){ return temperature_measureOneSensor_degC(PIN_TEMP_BLU); }
int8_t temperature_ambient(void)	{ return temperature_measureOneSensor_degC(PIN_TEMP_WHT); }

////////////////////////////////////////////////////////////////////////////////////

//stores the most extreme battery temperature in battTemp
//LiBCM has QTY3 battery temperature sensors
void temperature_battery_measure(void)
{
	uint8_t numTempSensorFaults = 0;
	int8_t batteryTemps[NUM_BATTERY_TEMP_SENSORS + 1] = {0}; //1-indexed ([1] = bay1 temp)

	batteryTemps[1] = temperature_measureOneSensor_degC(PIN_TEMP_BAY1);
	batteryTemps[2] = temperature_measureOneSensor_degC(PIN_TEMP_BAY2);
	batteryTemps[3] = temperature_measureOneSensor_degC(PIN_TEMP_BAY3);
	int8_t tempHi = -127; //degC
	int8_t tempLo =  127; //degC

	for(uint8_t ii = 1; ii <= NUM_BATTERY_TEMP_SENSORS; ii++)
	{
		if(batteryTemps[ii] == TEMPERATURE_SENSOR_FAULT) //verify temperatures are in range
		{
			batteryTemps[ii] =  ROOM_TEMP_DEGC; //ignore missing sensor
			numTempSensorFaults++;
		}

		if(batteryTemps[ii] > tempHi) { tempHi = batteryTemps[ii]; } //find hi temp
		if(batteryTemps[ii] < tempLo) { tempLo = batteryTemps[ii]; } //find lo temp
	}

	//alert user if all battery temp sensors are faulted
	if(numTempSensorFaults == NUM_BATTERY_TEMP_SENSORS)
	{
		Serial.print(F("\nConnect Batt Temp Sensors!"));
		//JTS2doNow: Beep?  Fans full blast?  How to alert user?
		battTemp = TEMPERATURE_SENSOR_FAULT;
	}
	else //at least one battery temperature sensor is working
	{
		//find hi & lo magnitudes from ROOM_TEMP_DEGC
		int8_t tempHiDelta = 0;
		int8_t tempLoDelta = 0;

		if(tempHi > ROOM_TEMP_DEGC) { tempHiDelta = tempHi - ROOM_TEMP_DEGC; }
		else                        { tempHiDelta = ROOM_TEMP_DEGC - tempHi; }

		if(tempLo > ROOM_TEMP_DEGC) { tempLoDelta = tempLo - ROOM_TEMP_DEGC; }
		else                        { tempLoDelta = ROOM_TEMP_DEGC - tempLo; }

		//figure out which magnitude is further from ROOM_TEMP_DEGC 
		if(tempHiDelta > tempLoDelta) { battTemp = tempHi; }
		else                          { battTemp = tempLo; }
	}
}

////////////////////////////////////////////////////////////////////////////////////

void temperature_handler(void)
{
	#define TEMP_SENSORS_OFF     0
	#define TEMP_SENSORS_ON      1
	#define TEMP_SENSORS_POWERUP 2
	#define TEMP_MEASURE_NOW     3
	static uint8_t tempSensorState = TEMP_SENSORS_OFF; //state machine
		
	static uint8_t keyStatePrevious = KEYOFF;
	uint8_t keyState_Now = key_getSampledState();

	if(keyState_Now != keyStatePrevious) { tempSensorState = TEMP_SENSORS_OFF; } //key state just changed (keyON->OFF or keyOFF->ON)

	keyStatePrevious = keyState_Now;

	//determine how often temperature sensors are measured
	#define TEMP_UPDATE_PERIOD_MILLIS_KEYON  500 
	#define TEMP_UPDATE_PERIOD_MILLIS_KEYOFF 60000 // 60k = 1 minute //careful: uint16_t!
	uint16_t temperatureUpdateInterval = 0;
	if(keyState_Now == KEYON) {temperatureUpdateInterval = TEMP_UPDATE_PERIOD_MILLIS_KEYON;  }
	else                      {temperatureUpdateInterval = TEMP_UPDATE_PERIOD_MILLIS_KEYOFF; }
	
	static uint32_t millis_previous = 0;

	if( (millis() - millis_previous) > temperatureUpdateInterval )
	{
		//time to measure temperature sensors
		millis_previous = millis();

		if     (tempSensorState == TEMP_SENSORS_ON)  { tempSensorState = TEMP_MEASURE_NOW;                                       }
		else if(tempSensorState == TEMP_SENSORS_OFF) { tempSensorState = TEMP_SENSORS_POWERUP; gpio_turnTemperatureSensors_on(); }
	}

	else if(tempSensorState == TEMP_MEASURE_NOW)
	{
		tempSensorState = TEMP_SENSORS_ON;
		temperature_battery_measure();
		//JTS2doNow: Add more temp sensor logic for fans/etc
		
		if(keyState_Now == KEYOFF)
		{
			tempSensorState = TEMP_SENSORS_OFF;
			gpio_turnTemperatureSensors_off(); 
			Serial.print(F("\ntemp:"));
			Serial.print(String(battTemp));
		}
	}	

	else if (tempSensorState == TEMP_SENSORS_POWERUP)
	{
		//wait for the temp sensor LPFs to stabilize
		#define TEMP_STABILIZATION_TIME_mS 100
		#define TEMP_NUM_CYCLES_TO_STABILIZE (TEMP_STABILIZATION_TIME_mS / LOOP_RATE_MILLISECONDS) //preprocessor handles this division
		
		static uint8_t numCyclesWaited = 0;
		
		if(numCyclesWaited < TEMP_NUM_CYCLES_TO_STABILIZE) { numCyclesWaited++; } //keep waiting
		else        /* temp sensors stabilized */          { numCyclesWaited = 0; tempSensorState = TEMP_MEASURE_NOW; }
	}

}

////////////////////////////////////////////////////////////////////////////////////

int8_t temperature_measureOneSensor_degC(uint8_t thermistorPin)
{			
	uint16_t countsADC = analogRead(thermistorPin); //measure ADC counts

	//This commented out section quite math intensive:
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
	//Rather than do the above math, LiBCM uses a lookup table (determined by the above equations):

	int8_t tempMeasured_celsius = 0;
	if     (countsADC > 1000) { tempMeasured_celsius = TEMPERATURE_SENSOR_FAULT; } //sensor unplugged
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
	else if(countsADC >  172) { tempMeasured_celsius =  63; } //JTS2doNow: It's possible MCM can only receive values up to 63 degC (63*2 = 126 (max int8_t))
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
	else                      { tempMeasured_celsius =  TEMPERATURE_SENSOR_FAULT; } //sensor shorted

	return tempMeasured_celsius;
}

////////////////////////////////////////////////////////////////////////////////////

//JTS2doNow:
/*
	-Monitor Temperature
		-Read QTY4 temp pins (Temp_YEL/GRN/WHT/BLU_Pin)
		-Read QTY4 LTC6804 temps
		-If temp warm (35 degC?) && ( tempCabin < max(tempBattery) )
			-Onboard fans low (FanOnPWM_Pin)
			-OEM Fan on low (FanOEMlow_Pin)
		-If temp hot (45 degC?)
			-OEM Fan on high (FanOEMhigh_Pin)
			-Onboard fans full speed (FanOnPWM_Pin)
		-If temp overheating (50 degC?)
			-OEM Fans on high (FanOEMhigh_Pin)
			-Onboard fans full speed (FanOnPWM_Pin)
			-Send overtemp flag (METSCI@Serial2)

*/