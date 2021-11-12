//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef soc_h
	#define soc_h

	void SoC_integrateCharge_adcCounts(int16_t adcCounts);

	void stateOfCharge_handler(void);

#endif