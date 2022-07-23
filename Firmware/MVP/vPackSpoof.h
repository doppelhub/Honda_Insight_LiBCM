//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef vpackspoof_h
	#define vpackspoof_h

	#define MCMe_USING_VPACK 1 //use the actual pack voltage
	#define MCMe_USER_DEFINED 2 //use a manual value (set by user)

	void vPackSpoof_setModeMCMePWM(uint8_t newMode);

	int16_t vPackSpoof_getPWMcounts_MCMe(void);
	void    vPackSpoof_setPWMcounts_MCMe(uint8_t newCounts);

	int16_t vPackSpoof_getPWMcounts_VPIN(void);

	void vPackSpoof_setVoltage(void);

	uint8_t vPackSpoof_getMCMeOffsetVoltage(void);
	void    vPackSpoof_setMCMeOffsetVoltage(uint8_t newOffset);

	void vPackSpoof_handleKeyOFF(void);

	void vPackSpoof_handleKeyON(void);

	uint8_t vPackSpoof_getSpoofedPackVoltage(void);

	void spoofVoltageMCMe_setSpecificPWM(uint8_t valuePWM);
#endif