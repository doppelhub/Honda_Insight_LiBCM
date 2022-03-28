//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef cellbalance_h
	#define cellbalance_h

	#define BALANCING_DISABLED 0
	#define BALANCING_ALLOWED  1

	bool cellBalance_wereCellsBalanced(void);

	void cellBalance_handler(void);

#endif