//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef wattHours_h
    #define wattHours_h

	#define NUM_BYTES_PER_Wh_RECORD 8
	#define NUM_Wh_RECORDS          128
	#define NUM_BYTES_Wh_HISTORY    (NUM_BYTES_PER_Wh_RECORD * NUM_Wh_RECORDS)

	#define ENERGY_SOURCE_GRID_CHARGER 1
	#define ENERGY_SOURCE_ENGINE_REGEN 2 

	#define FLAG_CHARGE_IS_FROM_GRID 0x8000

	#define MIN_Wh_TO_STORE_TRIP 10

	uint16_t energy_getAssist_Wh(void);
	uint16_t energy_getRegen_Wh (void);
	
	void energy_zeroWh(void);
	void energy_storeTrip(uint8_t caller);

	void energy_handler(void);

#endif
