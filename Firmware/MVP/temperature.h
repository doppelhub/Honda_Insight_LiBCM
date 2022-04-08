//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef temperature_h
	#define temperature_h

	int8_t temperature_battery_getLatest(void);
	int8_t temperature_airIntake(void);
	int8_t temperature_airExhaust(void);
	int8_t temperature_gridCharger(void);
	int8_t temperature_ambient(void);

	int8_t temperature_measureOneSensor_degC(uint8_t thermistorPin);
	
	void temperature_handler(void);

	#define TEMPERATURE_SENSOR_FAULT 127
	#define ROOM_TEMP_DEGC 23
	#define NUM_BATTERY_TEMP_SENSORS 3
#endif