//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042result_h
	#define LTC68042result_h

	uint8_t LTC68042result_errorCount_get       (void                 );
	void    LTC68042result_errorCount_set       (uint8_t newErrorCount);
	void    LTC68042result_errorCount_increment (void                 );

	void    LTC68042result_packVoltage_set (uint8_t voltage);
	uint8_t LTC68042result_packVoltage_get (void           );

	void     LTC68042result_minEverCellVoltage_set(uint16_t newMin_counts);
	uint16_t LTC68042result_minEverCellVoltage_get(void                  );

	void     LTC68042result_maxEverCellVoltage_set(uint16_t newMax_counts);
	uint16_t LTC68042result_maxEverCellVoltage_get(void                  );

	void     LTC68042result_loCellVoltage_set(uint16_t newLo_counts);
	uint16_t LTC68042result_loCellVoltage_get(void                 );

	void     LTC68042result_hiCellVoltage_set(uint16_t newHi_counts);
	uint16_t LTC68042result_hiCellVoltage_get(void                 );
#endif

// void printCellVoltage_all(); //JTS2doLater: Add back