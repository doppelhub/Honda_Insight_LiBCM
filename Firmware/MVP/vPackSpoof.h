#ifndef vpackspoof_h
	#define vpackspoof_h

	void vPackSpoof_updateVoltage(uint8_t actualPackVoltage, uint8_t voltageToSpoof);

	uint8_t vPackSpoof_isVpinSpoofed(void);

	uint8_t getPackVoltage_VpinIn(void);

	void vPackSpoof_handleKeyOFF(void);

	void vPackSpoof_handleKeyON(void);

#endif