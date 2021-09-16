#ifndef key_h
	#define key_h

	#define KEYOFF                 0
	#define KEYON                  1
	#define KEYSTATE_UNINITIALIZED 2
	#define KEYOFF_JUSTOCCURRED    3

	void key_stateChangeHandler(void);

	uint8_t key_getSampledState(void);

#endif