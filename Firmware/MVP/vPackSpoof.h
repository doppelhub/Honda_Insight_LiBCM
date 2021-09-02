//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef vpackspoof_h
	#define vpackspoof_h

	int16_t vPackSpoof_getPWMcounts_MCMe(void);

	int16_t vPackSpoof_getPWMcounts_VPIN(void);

	void vPackSpoof_setVoltage(uint8_t newSpoofedVoltage);

	void vPackSpoof_handleKeyOFF(void);

	void vPackSpoof_handleKeyON(void);

	uint8_t vPackSpoof_getSpoofedPackVoltage(void);
#endif