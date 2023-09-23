//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef batteryHistory_h
	#define batteryHistory_h

	#define BATTERY_HISTORY_UPDATE_PERIOD_ms MILLISECONDS_PER_HOUR

	#define TEMP_BIN_WIDTH_DEGC          4 //must be 2^n //e.g. -26 to -23, -22 d to -19, etc
	#define TEMP_BIN_WIDTH_RIGHTSHIFTS   2 //must match above (DEGC = 2^RIGHTSHIFTS)
	#define LO_TEMP_BIN_TOP_DEGC      (-27)// lowest bin's high count (e.g. temps up to -27 degC)
	#define HI_TEMP_BIN_TOP_DEGC        69 //highest bin's high count (e.g. temps from 65 to 69)
	#define TOTAL_TEMP_BINS (((HI_TEMP_BIN_TOP_DEGC - LO_TEMP_BIN_TOP_DEGC) >> TEMP_BIN_WIDTH_RIGHTSHIFTS) + 2)
	//bin#         0,  1,  2,  3,  4, 5, 6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
	//degC:up to -27,-23,-19,-15,-11,-7,-3,1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,65,69,70+

	#define SoC_BIN_WIDTH_PERCENT        4 //must be 2^n //e.g. 0 to 3%, 4 to 7%, etc.
	#define SoC_BIN_WIDTH_RIGHTSHIFTS    2 //must match above (PERCENT = 2^RIGHTSHIFTS)
	#define LO_SoC_BIN_TOP_PERCENT       0 // lowest bin's high count //bin0 is 0:0% SoC //bin1 is 1:4% SoC
	#define HI_SoC_BIN_TOP_PERCENT     100 //highest bin's high count (e.g. SoC up to 100%)
	#define TOTAL_SoC_BINS (((HI_SoC_BIN_TOP_PERCENT - LO_SoC_BIN_TOP_PERCENT) >> SoC_BIN_WIDTH_RIGHTSHIFTS) + 1)
	//bin#    0,  1,  2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24, 25
	//up to % 0,1:4,5:8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100

	#define NUM_BYTES_PER_BIN 2 //65535 hours (~7.5 years) storable in each X+Y bin
	#define NUM_BYTES_BATTERY_HISTORY (TOTAL_SoC_BINS * TOTAL_TEMP_BINS * NUM_BYTES_PER_BIN)

	void batteryHistory_printAll(void);

	void batteryHistory_handler(void);
	
#endif