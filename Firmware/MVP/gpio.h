#ifndef gpio_h
	#define gpio_h

	void gpio_begin(void);

	bool gpio_keyStateNow(void);

	void gpio_setFanSpeed_OEM(char speed);
	void gpio_setFanSpeed(uint8_t speed, bool isSpeedRamped);

	void gpio_turnPowerSensors_on( void);
	void gpio_turnPowerSensors_off(void);

	bool gpio_isGridChargerPluggedInNow(void);
	bool gpio_isGridChargerChargingNow(void);

	void gpio_turnGridCharger_on( void);
	void gpio_turnGridCharger_off(void);

	void gpio_setGridCharger_powerLevel(char powerLevel);

	void gpio_turnBuzzer_on_highFreq(void);
	void gpio_turnBuzzer_on_lowFreq(void);
	void gpio_turnBuzzer_off(void);
	void gpio_playSound_firmwareUpdated(void);

	bool gpio_isCoverInstalled(void);
	void gpio_safetyCoverCheck(void);

	void gpio_turnHMI_on(void);
	void gpio_turnHMI_off(void);

	void gpio_turnTemperatureSensors_on(void);
	void gpio_turnTemperatureSensors_off(void);

	void gpio_turnLiBCM_off(void);

	#define RAMP_FAN_SPEED 1
	#define ABSOLUTE_FAN_SPEED 0

#endif