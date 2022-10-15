//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//Stores runtime execution information //used to debug LiBCM
//To print status at runtime, type '$DISP=DBG'

#include "libcm.h"

/////////////////////////////////////////////////////////////////////////////////////////////

	//Message format:
	//\n k

	//Where:
	//r: since last keyOFF, has LiBCM booted while key ON?
	//k: keyState


	//JTS2doNow: Finish implementing this feature
	void status_printState(void)
	{
		static uint8_t numPeriodsPrinted = 0;
		
		Serial.print('.');

		if(numPeriodsPrinted++ == 100)
		{ 
			Serial.print('\n');
			numPeriodsPrinted = 0;
		}

	}

/////////////////////////////////////////////////////////////////////////////////////////////

	void status_setValue(uint8_t parameterToSet, uint8_t value)
	{
		switch(parameterToSet)
		{
			case STATUS_IGNITION: ; break;
			case STATUS_REBOOTED: ; break;
		}
	}