#ifndef key_h
	#define key_h

	#define KEYON  true
	#define KEYOFF false

	void key_stateChangeHandler(void);

	bool key_getSampledState(void);

#endif