//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042cell_h
	#define LTC68042cell_h
	
	#define STATE_FIRSTRUN 0
	#define STATE_GATHER  1
	#define STATE_PROCESS 2

	void LTC68042cell_nextVoltages(void);
#endif