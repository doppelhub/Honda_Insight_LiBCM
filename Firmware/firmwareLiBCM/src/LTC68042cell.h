//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042cell_h
    #define LTC68042cell_h
    
    #define LTC_STATE_FIRSTRUN 0
    #define LTC_STATE_GATHER   1
    #define LTC_STATE_PROCESS  2

    #define GATHERING_CELL_DATA 0
    #define CELL_DATA_PROCESSED 1

	#define LTC6804_MAX_CONVERSION_TIME_ms 5 //4.43 ms in '2kHz' sampling mode

    bool LTC68042cell_nextVoltages(void);
    void LTC68042cell_acquireAllCellVoltages(void);

#endif
