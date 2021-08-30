//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042result_h
	#define LTC68042result_h

	uint8_t LTC68042result_errorCount_get       (void                 );
	void    LTC68042result_errorCount_set       (uint8_t newErrorCount);
	void    LTC68042result_errorCount_increment (void                 );

	void    LTC68042result_stackVoltage_set (uint8_t voltage);
	uint8_t LTC68042result_stackVoltage_get (void           );

	void     LTC68042result_minEverCellVoltage_set(uint16_t newMin_counts);
	uint16_t LTC68042result_minEverCellVoltage_get(void                  );

	void     LTC68042result_maxEverCellVoltage_set(uint16_t newMax_counts);
	uint16_t LTC68042result_maxEverCellVoltage_get(void                  );

	void     LTC68042result_loCellVoltage_set(uint16_t newLo_counts);
	uint16_t LTC68042result_loCellVoltage_get(void                 );

	void     LTC68042result_hiCellVoltage_set(uint16_t newHi_counts);
	uint16_t LTC68042result_hiCellVoltage_get(void                 );
#endif

// void printCellVoltage_all();

// void LTC6804_printCellVoltage_max_min(void);

// uint8_t LTC6804results_getStackVoltage(void);

// void LTC68042result_setStackVoltage( uint8_t stackVoltage_volts );