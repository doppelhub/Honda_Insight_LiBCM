//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef vpackspoof_h
	#define vpackspoof_h

	#define MCMe_USING_VPACK 1 //use the actual pack voltage
	#define MCMe_USER_DEFINED 2 //use a manual value (set by user)

	#define MCME_VOLTAGE_OFFSET_ADJUST 12 //JTS2doNow: This feature is no longer used //use maxPossibleVspoof instead

	#define BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS 10 //below this many amps assist, spoofed pack voltage is as high as possible
		
	//choose the current range to adjust spoofed pack voltage from 0 to 100% over
		//#define ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF  32 //2^5 = 32 //MUST be 2^n!
		//#define ADDITIONAL_AMPS__2_TO_THE_N        5 //2^5 = 32
	//OR
		#define ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF  64 //2^6 = 64 //MUST be 2^n!
		#define ADDITIONAL_AMPS__2_TO_THE_N        6 //2^6 = 64

		#define MAXIMIZE_POWER_ABOVE_CURRENT_AMPS (BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS + ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF)

		#define VSPOOF_TO_MAXIMIZE_POWER 125 //Maximum assist occurs when MCM thinks pack is at 120 volts

	void vPackSpoof_setModeMCMePWM(uint8_t newMode);

	int16_t vPackSpoof_getPWMcounts_MCMe(void);
	void    vPackSpoof_setPWMcounts_MCMe(uint8_t newCounts);

	int16_t vPackSpoof_getPWMcounts_VPINout(void);

	void vPackSpoof_setVoltage(void);

	uint8_t vPackSpoof_getMCMeOffsetVoltage(void);

	void vPackSpoof_handleKeyOFF(void);

	void vPackSpoof_handleKeyON(void);

	uint8_t vPackSpoof_getSpoofedPackVoltage(void);

	void spoofVoltageMCMe_setSpecificPWM(uint8_t valuePWM);
#endif