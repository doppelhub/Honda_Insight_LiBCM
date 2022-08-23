#ifndef key_h
	#define key_h

	#define KEYSTATE_OFF              0
	#define KEYSTATE_ON               1
	#define KEYSTATE_UNINITIALIZED    2
	#define KEYSTATE_OFF_JUSTOCCURRED 3

	void key_stateChangeHandler(void);

	uint8_t key_getSampledState(void);
	uint32_t key_latestTurnOnTime_ms_get(void);
	uint32_t key_latestTurnOffTime_ms_get(void);
#endif
