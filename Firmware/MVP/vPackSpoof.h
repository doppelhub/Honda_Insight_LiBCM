//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef vpackspoof_h
	#define vpackspoof_h

	void vPackSpoof_updateVoltage(uint8_t actualPackVoltage, uint8_t voltageToSpoof);

	void vPackSpoof_handleKeyOFF(void);

	void vPackSpoof_handleKeyON(void);

#endif