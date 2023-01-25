#ifndef heater_h
	#define heater_h

	#define heater_OFF 0
	#define heater_ON  1

	#define HEATER_STATE_CHANGE_HYSTERESIS_ms (1 * 1000) //period required before heater can turn on or off

	#define FORCE_HEATER_OFF_ABOVE_TEMP_C 30 //prevent code changes from overheating pack

	void heater_init(void);

	uint8_t heater_isInstalled(void);
	uint8_t heater_isPackTooHot(void);

	void heater_handler(void);

#endif