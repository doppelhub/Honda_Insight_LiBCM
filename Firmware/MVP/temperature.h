//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef temperature_h
	#define temperature_h

	int8_t temperature_battery_getLatest(void);
	int8_t temperature_intake_getLatest(void);
	int8_t temperature_exhaust_getLatest(void);
	int8_t temperature_gridCharger_getLatest(void);
	int8_t temperature_ambient_getLatest(void); //IMA bay temperature

	int8_t temperature_measureOneSensor_degC(uint8_t thermistorPin);
	
	void temperature_handler(void);

	#define TEMPERATURE_SENSOR_FAULT_HI 127
	#define TEMPERATURE_SENSOR_FAULT_LO -31
	#define ROOM_TEMP_DEGC 23
	#define NUM_BATTERY_TEMP_SENSORS 3

	#define TEMP_STABILIZATION_TIME_ms 100

	#define TEMP_UPDATE_PERIOD_MILLIS_KEYON  500 
	#define TEMP_UPDATE_PERIOD_MILLIS_KEYOFF 60000 // 60k = 1 minute //careful: uint16_t!
#endif