//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef soc_h
	#define soc_h

	void SoC_integrateCharge_adcCounts(int16_t adcCounts);

	uint16_t SoC_getBatteryStateNow_mAh(void);
	void     SoC_setBatteryStateNow_mAh(uint16_t newPackCharge_mAh);

	uint8_t SoC_getBatteryStateNow_percent(void);
	void    SoC_setBatteryStateNow_percent(uint8_t newSoC);

	uint8_t SoC_estimateFromRestingCellVoltage_percent(void);

	void SoC_updateUsingOpenCircuitVoltage(void);

	void SoC_turnOffLiBCM_ifPackEmpty(void);

	void SoC_handler(void);

#endif