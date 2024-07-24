//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef cellbalance_h
    #define cellbalance_h

    #define BALANCING_DISABLED 0
    #define BALANCING_ALLOWED  1

	#define YES__BALANCING_ALLOWED YES__CHARGING_ALLOWED //balancing code reusing grid charging constants

    bool cellBalance_areCellsBalancing(void);

    void cellBalance_handler(void);

#endif
