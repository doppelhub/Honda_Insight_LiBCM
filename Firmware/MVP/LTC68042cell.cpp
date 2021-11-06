//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LTC6804 cell voltage data processor and related functions
//"CVR" = "Cell Voltage Register" = QTY3 16b cell voltages, plus QTY1 PEC (8B total) //see datasheet

#include "libcm.h"

//all returned cell voltages are stored in this array
//Note: LTC cells are 1-indexed, whereas array data is 0-indexed
//  Example: cellVoltages_counts[0][ 1] is IC_1 cell_02
//  Example: cellVoltages_counts[3][11] is IC_4 cell_12
uint16_t cellVoltages_counts[TOTAL_IC][CELLS_PER_IC];


//---------------------------------------------------------------------------------------


//tell all LTC68042 ICs to measure all cells
void startCellConversion()
{
  uint8_t cmd[4];

  //JTS2doLater: Replace magic numbers with #define
  //Cell Voltage conversion command.
  uint8_t ADCV[2] = { ((MD_FILTERED & 0x02 ) >> 1) + 0x02,  //set bit 9 true
                      ((MD_FILTERED & 0x01 ) << 7) + 0x60 + (DCP_DISABLED<<4) + CELL_CH_ALL }; 

  //Load 'ADCV' command into cmd array
  cmd[0] = ADCV[0];
  cmd[1] = ADCV[1];

  //Calculate PEC
  uint16_t temp_pec = LTC68042configure_calcPEC15(2, ADCV);
  cmd[2] = (uint8_t)(temp_pec >> 8);
  cmd[3] = (uint8_t)(temp_pec);

  LTC68042configure_wakeupCore();

  //send 'adcv' command to all LTC6804s (broadcast command)
  digitalWrite(PIN_SPI_CS,LOW);
  LTC68042configure_spiWrite(4,cmd);
  digitalWrite(PIN_SPI_CS,HIGH); 
}


//---------------------------------------------------------------------------------------


//Read a single 8 byte CVR and store the result in *data
//This function is ONLY used by validateAndStoreNextCVR().
void serialReadCVR( uint8_t chipAddress, char cellVoltageRegister, uint8_t *data ) //data: Unparsed cellVoltage_counts
{
  uint8_t cmd[4];
  uint16_t calculated_pec;

  cmd[0] = 0x80 + (chipAddress<<3); //configure LTC address (p46:Table33)

  switch(cellVoltageRegister) //chose which "cell voltage register" to read
  {
    case 'A': cmd[1] = 0x04; break;
    case 'B': cmd[1] = 0x06; break;
    case 'C': cmd[1] = 0x08; break;
    case 'D': cmd[1] = 0x0A; break;
  }

  calculated_pec = LTC68042configure_calcPEC15(2, cmd);
  cmd[2] = (uint8_t)(calculated_pec >> 8);
  cmd[3] = (uint8_t)(calculated_pec);

  LTC68042configure_wakeupIsoSPI(); //Guarantees LTC6804 isoSPI port is awake.
  digitalWrite(PIN_SPI_CS,LOW);
  LTC68042configure_spiWriteRead(cmd,4,&data[0],8);
  digitalWrite(PIN_SPI_CS,HIGH);
}


//---------------------------------------------------------------------------------------


//Validate specified LTC6804's specified CVR
//store valid cell voltages in cellVoltages_counts[][]
void validateAndStoreNextCVR(uint8_t chipAddress, char cellVoltageRegister)
{
	const uint8_t NUM_BYTES_IN_REG  = 6; //QTY3 cells * 2B/cell
	const uint8_t NUM_RX_BYTES      = 8; //NUM_BYTES_IN_REG + 2B PEC
	const uint8_t MAX_READ_ATTEMPTS = 4; //max attempts to read back CVR without PEC error

	uint8_t attemptCounter = 0;
	uint16_t received_pec;
	uint16_t calculated_pec;
	uint16_t cellX_Voltage_counts;
	uint16_t cellY_Voltage_counts;
	uint16_t cellZ_Voltage_counts;

	do //repeats until PECs match (i.e. no data transmission errors) 
	{
		uint8_t *returnedData; //JTS2doLater: change to standard empty array
		returnedData = (uint8_t *) malloc( NUM_RX_BYTES * sizeof(uint8_t) );

		//Read single cell voltage register (QTY3 cell voltages) from specified IC
		serialReadCVR(chipAddress, cellVoltageRegister, returnedData); //result stored in returnedData

		//parse 16b cell voltages from returnedData (from LTC6804)
		cellX_Voltage_counts = returnedData[0] + (returnedData[1]<<8); //(lower byte)(upper byte)
		cellY_Voltage_counts = returnedData[2] + (returnedData[3]<<8);
		cellZ_Voltage_counts = returnedData[4] + (returnedData[5]<<8);

		received_pec   = (returnedData[6]<<8) + returnedData[7]; //PEC0 PEC1
		calculated_pec = LTC68042configure_calcPEC15(NUM_BYTES_IN_REG, &returnedData[0]);

		free(returnedData);

		attemptCounter++; //prevent while loop hang
	} while( (received_pec != calculated_pec) && (attemptCounter < MAX_READ_ATTEMPTS) ); //retry if isoSPI error

	if (attemptCounter > 1)
	{ //PEC error occurred
		LTC68042result_errorCount_increment();
	} 
	else //data transmitted correctly
	{ //Determine which LTC cell voltages were read into returnedData
		uint8_t cellX=0; //1st cell in returnedData (LTC cell 1, 4, 7, or 10)
		uint8_t cellY=0; //2nd cell in returnedData (LTC cell 2, 5, 8, or 11) 
		uint8_t cellZ=0; //3rd cell in returnedData (LTC cell 3, 6, 9, or 12)
		switch(cellVoltageRegister)  //LUT to prevent QTY3 multiplies & QTY12 adds per call
		{
			case 'A': cellX=0;  cellY=1;  cellZ=2 ; break; //LTC cells  1/ 2/ 3 (LTC 1-indexed, array 0-indexed)
			case 'B': cellX=3;  cellY=4;  cellZ=5 ; break; //LTC cells  4/ 5/ 6
			case 'C': cellX=6;  cellY=7;  cellZ=8 ; break; //LTC cells  7/ 8/ 9
			case 'D': cellX=9;  cellY=10; cellZ=11; break; //LTC cells 10/11/12
			default: Serial.print(F("\nillegal CVR index")); while(1) {;} //hang here until watchdog resets.
		}

		//store valid cell voltage results
		cellVoltages_counts[chipAddress - FIRST_IC_ADDR][cellX] = cellX_Voltage_counts;
		cellVoltages_counts[chipAddress - FIRST_IC_ADDR][cellY] = cellY_Voltage_counts;
		cellVoltages_counts[chipAddress - FIRST_IC_ADDR][cellZ] = cellZ_Voltage_counts;
	}
}


//---------------------------------------------------------------------------------------


//perform cell voltage calculations that require all cell voltages
//results stored in LTC68042results.c
void processAllCellVoltages(void)
{
	uint32_t packVoltage_RAW = 0; //Multiply by 0.0001 for volts
	uint16_t loCellVoltage = 65535;
	uint16_t hiCellVoltage = 0;

	//loop through every cell in pack
	for (int chip = 0 ; chip < TOTAL_IC; chip++) //actual LTC serial address: 'chip' + FIRST_IC_ADDR )
	{ 
		for (int cell=0; cell < CELLS_PER_IC; cell++) //actual LTC cell number: 'cell' + 1 (zero-indexed)
		{ 
			uint16_t cellUnderTest = cellVoltages_counts[chip][cell];
		
			//accumulate Vpack	
			packVoltage_RAW += cellUnderTest;
			
			//find hi/lo cells
			if( cellUnderTest < loCellVoltage ) { loCellVoltage = cellUnderTest; }
			if( cellUnderTest > hiCellVoltage ) { hiCellVoltage = cellUnderTest; }

			//check for new maxEver/minEver cells (if any)
			if (cellUnderTest > LTC68042result_maxEverCellVoltage_get() ) {LTC68042result_maxEverCellVoltage_set(cellUnderTest); }
			if (cellUnderTest < LTC68042result_minEverCellVoltage_get() ) {LTC68042result_minEverCellVoltage_set(cellUnderTest); }

			LTC68042result_specificCellVoltage_set(chip, cell, cellUnderTest);
		}
	}

	LTC68042result_packVoltage_set( (uint8_t)(packVoltage_RAW * 0.0001) );
	
	LTC68042result_loCellVoltage_set(loCellVoltage);
	LTC68042result_hiCellVoltage_set(hiCellVoltage);
}


//---------------------------------------------------------------------------------------


//either gather next QTY3 cell voltages from LTC6804, or process a complete batch of returned cell voltage data.
//raw cell voltages are stored in file-scoped "cellVoltages_counts[][]"" array
//latest results are stored in "LTC68042_result.c"
//Example with QTY48 cells:
//	-the absolute first call starts a conversion.
//	After that, the behavior is as follows:  
//	-the next sixteen calls ( (48 cells) / (3 cells per call) = 16 calls ) read back QTY48 cell voltages.
//  -The seventeenth call performs all pack voltage math and stores valid results in LTC68042_result.c  
void LTC68042cell_nextVoltages(void)
{
	LTC68042configure_wakeupCore(); //non-blocking if LTC ICs already on

	static uint8_t presentState = STATE_FIRSTRUN;

	if(presentState == STATE_GATHER)
	{ //retrieve next CVR from LTC, then validate and store in cellVoltages_counts[][] array
		//round-robin state handlers
  		static uint8_t chipAddress = FIRST_IC_ADDR;
  		static char cellVoltageRegister = 'A'; //LTC68042 contains QTY4 CVRs (A/B/C/D)

  		validateAndStoreNextCVR(chipAddress, cellVoltageRegister);

  		//determine which LTC68042 IC & CVR to read next
	    cellVoltageRegister++;
	    if(cellVoltageRegister >= 'E' )
	    { //LTC6804 only has registers A,B,C,D
	    	cellVoltageRegister = 'A'; //reset back to first CVR

	    	chipAddress++; //move to next LTC6804 IC
	    	if(chipAddress >= (FIRST_IC_ADDR + TOTAL_IC) )
	    	{ //just finished reading last IC's last CVR... all cell voltages stored in cellVoltages_counts[][]
	    		startCellConversion(); //takes a while to finish... start ASAP
	    		
	    		chipAddress = FIRST_IC_ADDR; //reset to first LTC IC
					presentState = STATE_PROCESS; //all cell voltages gathered.  Process data on next run.
	    	}
	    }
	}

	else if(presentState == STATE_PROCESS)
	{	//all cell voltages read... 
		processAllCellVoltages(); //do math and store in LTC68042_result.c

		presentState = STATE_GATHER; //only runs once each time all cells are measured
	}

	else if(presentState == STATE_FIRSTRUN)
	{
		startCellConversion(); //make sure there's ADC data to read on first run
		presentState = STATE_GATHER;
	}

	else
	{
		Serial.print(F("\nillegal LTC68042cell state"));
		while(1) {;} //hang here until watchdog resets.
	}
}