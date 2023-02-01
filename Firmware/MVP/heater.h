#ifndef heater_h
	#define heater_h

	#define HEATER_STATE_CHANGE_HYSTERESIS_ms (2 * 1000) //period required before heater can turn on or off

	#define FORCE_HEATER_OFF_ABOVE_TEMP_C 25 //prevent code changes from overheating pack

	void heater_init(void);

	bool heater_isInstalled(void);
	bool heater_isPackTooHot(void);

	void heater_handler(void);

#endif