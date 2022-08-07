//JTS2doLater: eeprom.h isn't wrapped into MVP yet

#ifndef time_h
#define time_h
	bool time_toUpdate_keyOffValues(void);

	bool time_hasKeyBeenOffLongEnough_toEstimateSoC(void);
	bool time_hasKeyBeenOffLongEnough_toTurnOffLiBCM(void);

	void time_waitForLoopPeriod(void);

	void time_stopwatch(bool timerAction);

	uint16_t  time_hertz_to_milliseconds(uint8_t hertz);

	void time_loopPeriod_ms_set(uint8_t period_ms);
	uint8_t time_loopPeriod_ms_get(void);

	#define START_TIMER true
	#define STOP_TIMER false

#endif
