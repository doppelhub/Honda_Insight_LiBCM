//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042cell_h
	#define LTC68042cell_h
	
	#define STATE_FIRSTRUN 0
	#define STATE_GATHER  1
	#define STATE_PROCESS 2

	#define GATHERED_LTC6804_DATA  0
	#define PROCESSED_LTC6804_DATA 1

	bool LTC68042cell_nextVoltages(void);
	void LTC68042cell_sampleGatherAndProcessAllCellVoltages(void);
#endif