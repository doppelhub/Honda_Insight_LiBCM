//JTS2doLater: eeprom.h isn't wrapped into MVP yet

#ifndef time_h
#define time_h
	bool time_toUpdate_keyOffValues(void);

	bool time_hasKeyBeenOffLongEnough(void);

	void time_waitForLoopPeriod(void);
#endif
