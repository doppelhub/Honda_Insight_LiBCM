//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef cellbalance_h
	#define cellbalance_h

	#define BALANCING_DISABLED 0
	#define BALANCING_ALLOWED  1

	void cellBalance_handler(void);

	void cellBalance_disableBalanceResistors(void);

#endif
