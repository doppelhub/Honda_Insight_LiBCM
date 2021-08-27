#ifndef debugUSB_h
	#define debugUSB_h

	void debugUSB_batteryCurrentActual_amps(uint8_t current_amps);

	void debugUSB_batteryCurrentSpoofed_amps(uint8_t current_amps);

	void debugUSB_batteryCurrent_counts(uint16_t current_counts);

	void debugUSB_cellHI_counts(uint16_t voltage_counts);

	void debugUSB_cellLO_counts(uint16_t voltage_counts);

	void debugUSB_VpackActual_volts(uint8_t voltage_volts);

	void debugUSB_VpackSpoofed_volts(uint8_t voltage_volts);

	void debugUSB_printLatest_data(void);
#endif