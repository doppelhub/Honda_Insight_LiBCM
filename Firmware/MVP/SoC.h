//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef soc_h
	#define soc_h

	void SoC_integrateCharge_adcCounts(int16_t adcCounts);

	uint16_t SoC_getBatteryStateNow_mAh(void);
	void     SoC_setBatteryStateNow_mAh(uint16_t newPackCharge_mAh);

	uint8_t SoC_getBatteryStateNow_percent(void);
	void    SoC_setBatteryStateNow_percent(uint8_t newSoC);

	uint8_t SoC_estimateFromRestingCellVoltage_percent(void);

	void SoC_updateUsingLatestOpenCircuitVoltage(void);

	void SoC_turnOffLiBCM_ifPackEmpty(void);

	bool SoC_isThermalManagementAllowed(void);

	void SoC_handler(void);

	#ifdef BATTERY_TYPE_5AhG3
		#define CELL_VREST_85_PERCENT_SoC 40000 //for maximum life, resting cell voltage should remain below 85% SoC
		#define CELL_VREST_10_PERCENT_SoC 34200 //for maximum life, resting cell voltage should remain above 10% SoC
		#define STACK_mAh_NOM 5000 //5 Ah nominal //nominal pack size (0:100% SoC)
	#elif defined BATTERY_TYPE_47AhFoMoCo
		#define CELL_VREST_85_PERCENT_SoC 39700
		#define CELL_VREST_10_PERCENT_SoC 34000
		#define STACK_mAh_NOM 47000
	#else
		#error (Battery type not specified in config.h)
	#endif

#endif