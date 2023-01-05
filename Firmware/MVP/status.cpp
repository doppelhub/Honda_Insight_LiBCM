//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//Stores runtime execution information //used to debug LiBCM
//To print status at runtime, type '$DISP=DBG'

#include "libcm.h"

uint8_t  parameters_8b [STATUS_NUM_PARAMETERS_8b]  = { 0 };
uint16_t parameters_16b[STATUS_NUM_PARAMETERS_16b] = { 0 };

/////////////////////////////////////////////////////////////////////////////////////////////

//Message format:
//\n k TBD

//JTS2doNow: implement
void status_printState(void)
{	

	//this is just throughput test code.  Looks like we can easily sent 6200 ascii characters per second... should be enough
	static uint8_t numTimesPrinted = 0;

	for( uint8_t ii=0; ii<62; ii++) { Serial.print('.'); }

	numTimesPrinted++;
	if(numTimesPrinted == 99) { Serial.print('\n'); numTimesPrinted = 0; }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void status_setValue_8b(uint8_t parameterToSet, uint8_t value) { parameters_8b[parameterToSet] = value; }

