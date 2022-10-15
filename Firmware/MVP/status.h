//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef runtimeStatus_h
	#define runtimeStatus_h

	void status_printState(void);

	void status_setValue(uint8_t parameterToSet, uint8_t value);

	#define STATUS_IGNITION 1
	#define STATUS_REBOOTED 2

#endif