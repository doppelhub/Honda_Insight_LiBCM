//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//measures OEM temperature sensors
//LTC6804 temperature sensors are measured in LTC68042gpio.cpp

//FYI: need to enable temperature sensors

#include "libcm.h"

/*

	-Monitor Temperature
		-Read QTY4 temp pins (Temp_YEL/GRN/WHT/BLU_Pin)
		-Read QTY4 LTC6804 temps
		-If temp warm (35 degC?) && ( tempCabin < max(tempBattery) )
			-Onboard fans low (FanOnPWM_Pin)
			-OEM Fan on low (FanOEMlow_Pin)
		-If temp hot (45 degC?)
			-OEM Fan on high (FanOEMhigh_Pin)
			-Onboard fans full speed (FanOnPWM_Pin)
		-If temp overheating (50 degC?)
			-OEM Fans on high (FanOEMhigh_Pin)
			-Onboard fans full speed (FanOnPWM_Pin)
			-Send overtemp flag (METSCI@Serial2)

*/