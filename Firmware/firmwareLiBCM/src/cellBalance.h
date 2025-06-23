//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef cellbalance_h
    #define cellbalance_h

    #define BALANCING_DISABLED 0
    #define BALANCING_ALLOWED  1

	#define CELL_MAJOR_IMBALANCE_DELTA          1000  // '1000' = 100 mV
	#define CELL_IMAX_MAJOR_IMBALANCE_DECIAMPS    90  //   '90' = 9.0 A 
	#define CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE   32  //   '32' = 3.2 mV //CANNOT exceed 255 counts (25.5 mV)
    #define CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT   22  //   '22' = 2.2 mV //ADC uncertainty: 2.2 mV //MUST be less than CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE

	//balancing constants reuse grid charging constants
	#define YES__BALANCING_ALLOWED       YES__CHARGING_ALLOWED
	#define  NO__BALANCING_NOT_REQUESTED  NO__CHARGING_NOT_REQUESTED

    bool cellBalance_areCellsBalancing(void);

    void cellBalance_handler(void);

#endif
