#ifndef gpio_h
	#define gpio_h

	void gpio_begin(void);

	bool gpio_keyStateNow(void);

	void gpio_setFanSpeed_OEM(char speed);

	void gpio_turnCurrentSensor_on(void);

	void gpio_turnCurrentSensor_off(void);

	bool gpio_isGridChargerPluggedIn(void);

#endif