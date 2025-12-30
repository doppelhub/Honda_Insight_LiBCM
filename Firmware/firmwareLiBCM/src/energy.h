//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef wattHours_h
    #define wattHours_h

	#define NUM_BYTES_PER_Wh_RECORD 8
	#define NUM_Wh_RECORDS          128
	#define NUM_BYTES_Wh_HISTORY    (NUM_BYTES_PER_Wh_RECORD * NUM_Wh_RECORDS)

	#define MIN_Wh_TO_STORE_TRIP 10

	uint16_t energy_getAssist_Wh(void);
	uint16_t energy_getRegen_Wh (void);
	uint16_t energy_getGridCharger_Wh (void);
	uint16_t energy_getTripMeterAssist_Wh (void);
	uint16_t energy_getTripMeterRegen_Wh (void);
	uint16_t energy_getTripMeterGridCharge_Wh (void);

	void energy_zeroWh(void);
	void energy_storeTrip(void);

	void energy_zeroWhTripMeter(void);
	void energy_storeTripMeter(uint16_t drive_assist_wh, uint16_t drive_regen_wh);
	void energy_storeTripMeterGridCharge(uint16_t grid_charger_wh);

	void energy_handler(void);

#endif
