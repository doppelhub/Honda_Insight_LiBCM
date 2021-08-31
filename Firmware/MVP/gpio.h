#ifndef gpio_h
	#define gpio_h

	void gpio_begin(void);

	bool gpio_keyStateNow(void);

	void gpio_setFanSpeed_OEM(char speed);
	void gpio_setFanSpeed(char speed);

	void gpio_turnCurrentSensor_on( void);
	void gpio_turnCurrentSensor_off(void);

	bool gpio_isGridChargerPluggedInNow(void);

	void gpio_turnGridCharger_on( void);
	void gpio_turnGridCharger_off(void);

	void gpio_setGridCharger_powerLevel(char powerLevel);

#endif