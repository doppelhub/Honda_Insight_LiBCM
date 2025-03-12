//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LTC6804 cell voltage data processor and related functions
//"CVR" = "Cell Voltage Register" = QTY3 16b cell voltages, plus QTY1 PEC (8B total) //see datasheet

#include "libcm.h"

//all returned cell voltages are stored in this array
//Note: LTC cells are 1-indexed, whereas array data is 0-indexed
//  Example: cellVoltages_counts[0][ 1] is IC_1 cell_02
//  Example: cellVoltages_counts[3][11] is IC_4 cell_12
uint16_t cellVoltages_counts[TOTAL_IC][CELLS_PER_IC];
uint32_t conversionStart_ms = 0;

//JTS2doLater: Add cell voltage test that sets user alert if a cell voltage suddenly changes from 'balanced' to 'majorly imbalanced'

/////////////////////////////////////////////////////////////////////////////////////////

//tell all LTC68042 ICs to measure all cells
void startCellConversion(void)
{
    uint8_t cmd[4];

    //JTS2doLater: Replace magic numbers with #define
    //Cell Voltage conversion command
    uint8_t ADCV[2] = { ((MD_FILTERED & 0x02 ) >> 1) + 0x02,  //set bit 9 true
                        ((MD_FILTERED & 0x01 ) << 7) + 0x60 + (IS_DISCHARGE_ALLOWED_DURING_CONVERSION<<4) + CELL_CH_ALL };

    //Load 'ADCV' command into cmd array
    cmd[0] = ADCV[0];
    cmd[1] = ADCV[1];

    //Calculate PEC
    uint16_t temp_pec = LTC68042configure_calcPEC15(2, ADCV);
    cmd[2] = (uint8_t)(temp_pec >> 8);
    cmd[3] = (uint8_t)(temp_pec);

    LTC68042configure_spiWrite(4,cmd); //send 'adcv' command to all LTC6804s (broadcast command)
    conversionStart_ms = millis();
}

/////////////////////////////////////////////////////////////////////////////////////////

//Read a single 8 byte CVR and store the result in *data
//This function is ONLY used by validateAndStoreNextCVR().
void serialReadCVR( uint8_t chipAddress, char cellVoltageRegister, uint8_t *data ) //data: Unparsed cellVoltage_counts
{
    uint8_t cmd[4];

    cmd[0] = 0x80 + (chipAddress<<3); //configure LTC address (p46:Table33)

    switch (cellVoltageRegister) //choose which "cell voltage register" to read
    {
        case 'A': cmd[1] = 0x04; break;
        case 'B': cmd[1] = 0x06; break;
        case 'C': cmd[1] = 0x08; break;
        case 'D': cmd[1] = 0x0A; break;
    }

    uint16_t calculated_pec = LTC68042configure_calcPEC15(2, cmd);
    cmd[2] = (uint8_t)(calculated_pec >> 8);
    cmd[3] = (uint8_t)(calculated_pec);

    LTC68042configure_spiWriteRead(cmd,4,&data[0],8);
}

/////////////////////////////////////////////////////////////////////////////////////////

//Validate specified LTC6804's specified CVR
//store valid cell voltages in cellVoltages_counts[][]
void validateAndStoreNextCVR(uint8_t chipAddress, char cellVoltageRegister)
{
    const uint8_t NUM_BYTES_IN_REG  = 6; //QTY3 cells * 2B/cell
    const uint8_t NUM_RX_BYTES      = 8; //NUM_BYTES_IN_REG + 2B PEC
    const uint8_t MAX_READ_ATTEMPTS = 3; //max attempts to read back CVR without PEC error

    uint8_t attemptCounter = 0;
    uint16_t received_pec;
    uint16_t calculated_pec;
    uint16_t cellX_Voltage_counts;
    uint16_t cellY_Voltage_counts;
    uint16_t cellZ_Voltage_counts;

    do //repeats until PECs match (i.e. no data transmission errors)
    {
        uint8_t returnedData[NUM_RX_BYTES];

        //Read single cell voltage register (QTY3 cell voltages) from specified IC
        serialReadCVR(chipAddress, cellVoltageRegister, returnedData); //result stored in returnedData

        //parse 16b cell voltages from returnedData (from LTC6804)
        cellX_Voltage_counts = returnedData[0] + (returnedData[1]<<8); //(lower byte)(upper byte)
        cellY_Voltage_counts = returnedData[2] + (returnedData[3]<<8);
        cellZ_Voltage_counts = returnedData[4] + (returnedData[5]<<8);

        received_pec   = (returnedData[6]<<8) + returnedData[7]; //PEC0 PEC1
        calculated_pec = LTC68042configure_calcPEC15(NUM_BYTES_IN_REG, &returnedData[0]);

        attemptCounter++; //prevent while loop hang

        if (attemptCounter > 1) { LTC68042result_errorCount_increment(); } //log each PEC error

    } while ((received_pec != calculated_pec) && (attemptCounter < MAX_READ_ATTEMPTS)); //retry if isoSPI error

    if (attemptCounter >= MAX_READ_ATTEMPTS)
    {
        //too many errors occurred
        cellX_Voltage_counts = 0;
        cellY_Voltage_counts = 0;
        cellZ_Voltage_counts = 0;
    }

    //Determine which LTC cell voltages were read into returnedData
    uint8_t cellX=0; //1st cell in returnedData (LTC cell 1, 4, 7, or 10)
    uint8_t cellY=0; //2nd cell in returnedData (LTC cell 2, 5, 8, or 11)
    uint8_t cellZ=0; //3rd cell in returnedData (LTC cell 3, 6, 9, or 12)
    switch (cellVoltageRegister)  //LUT to prevent QTY3 multiplies & QTY12 adds per call
    {
        case 'A': cellX=0;  cellY=1;  cellZ=2 ; break; //LTC cells  1/ 2/ 3 (LTC 1-indexed, array 0-indexed)
        case 'B': cellX=3;  cellY=4;  cellZ=5 ; break; //LTC cells  4/ 5/ 6
        case 'C': cellX=6;  cellY=7;  cellZ=8 ; break; //LTC cells  7/ 8/ 9
        case 'D': cellX=9;  cellY=10; cellZ=11; break; //LTC cells 10/11/12
        default: Serial.print(F("\nillegal CVR index")); while (1) {;} //hang here until watchdog resets.
    }

    //store cell voltage results
    cellVoltages_counts[chipAddress - FIRST_IC_ADDR][cellX] = cellX_Voltage_counts;
    cellVoltages_counts[chipAddress - FIRST_IC_ADDR][cellY] = cellY_Voltage_counts;
    cellVoltages_counts[chipAddress - FIRST_IC_ADDR][cellZ] = cellZ_Voltage_counts;
}

/////////////////////////////////////////////////////////////////////////////////////////

//results stored in LTC68042results.c
void processAllCellVoltages(void)
{
    uint32_t packVoltage_RAW = 0; //Multiply by 0.0001 for volts
    uint16_t loCellVoltage = 65535;
    uint16_t hiCellVoltage = 0;

    #ifdef BATTERY_TYPE_5AhG3
        //On LiBCM, QTY3 LTC6804 ICs measure QTY2 18S EHW5 modules:
        // -LTC6804 'A' measures the first QTY12 cells in the 1st 18S module (stack cells 01:12).  No problems here.
        // -LTC6804 'C' measures the  last QTY12 cells in the 2nd 18S module (stack cells 25:36).  No problems here.
        // -LTC6804 'B' measures the remaining QTY6 cells in both modules (stack cells 13:18 in module 'A', as well as stack cells 19:24 in module 'C').
        //
        //Since LTC6804 'B' straddles two 18S modules, reading cell 19's voltage includes
        //the voltage drop across the several-foot-long current cable connecting between the 18S modules.
        //This causes the measured cell 19 voltage to differ from the actual voltage at the cell terminals, proportional to the current sourced/sunk into the battery.
        //Note that the additional voltage error is solely a function of the resistance in the cabling between the modules, and has nothing to do with the cell ESR.
        //However, due to the high frequency chopping that occurs on the IGBT driver, the current might not actually be flowing the moment the LTC6804 samples the cells.
        //If no current is flowing at the precise moment the LTC6804 ADC samples the cells, the current-proportional voltage correction (on cell 19) will actually
        //introduce its own voltage error (rather than correcting the voltage), due to how the OEM Battery Current Sensor measures the true average current (without chopping).
        //Therefore, to pick the 'correct' voltage, LiBCM calculates the voltage correction, and then picks whichever voltage is closest to the other cells.
        //
        //Note that the ultimate root cause for this behavior is that the OEM EHW5 BMS connectors don't allow us to separately sense cell 18+ from cell 19-.
        //FYI: R359 guarantees that only cell 19 (and not cell 18) will have the above-described behavior.
        //The ideal solution would be to use the LTC6813 - which measures QTY18 cells - on both 18S EHW5 modules.
        //However, that IC is backordered for years, hence the above hardware decision and this workaround.
        //It's not ideal, but it's what we've got.  STFP!

        //cell 19 is the seventh cell on the second IC
        #define CELL19_CHIP_NUMBER 1 //array is zero-indexed // '1' is the 2nd IC
        #define CELL19_CELL_NUMBER 6 //array is zero-indexed // '6' is seventh cell (i.e. stack cell 19)

        #define VOLTAGECORRECTION_mV_PER_AMP 1 //1 mV/A error measured on RevC hardware //only corrects cell 19 for this specific issue
        #define LTC6804_COUNTS_PER_mV 10 //LSB is 100 uV
        #define LTC6804_COUNT_ADJUSTMENT_PER_AMP (VOLTAGECORRECTION_mV_PER_AMP * LTC6804_COUNTS_PER_mV) //preprocessor handles this multiply

        uint16_t cell19Voltage_measured = cellVoltages_counts[CELL19_CHIP_NUMBER][CELL19_CELL_NUMBER]; //store cell 19 voltage for later
        uint16_t cell19Voltage_adjusted = cell19Voltage_measured + adc_getLatestBatteryCurrent_amps() * LTC6804_COUNT_ADJUSTMENT_PER_AMP;

        //temporarily replace cell 19's voltage with cell 18's, to prevent cell 19's (possibly incorrect) voltage from being either the highest or lowest voltage
        cellVoltages_counts[CELL19_CHIP_NUMBER][CELL19_CELL_NUMBER] = cellVoltages_counts[CELL19_CHIP_NUMBER][CELL19_CELL_NUMBER - 1];
        //we'll restore cell 19's voltage after we determine pack hi/lo (in the for loops below)
    #endif

    //loop through every cell in pack
    for (int chip = 0 ; chip < TOTAL_IC; chip++) //actual LTC serial address: 'chip' + FIRST_IC_ADDR )
    {
        for (int cell=0; cell < CELLS_PER_IC; cell++) //actual LTC cell number: 'cell' + 1 (zero-indexed)
        {
            uint16_t cellVoltageUnderTest = cellVoltages_counts[chip][cell];

            //accumulate Vpack
            packVoltage_RAW += cellVoltageUnderTest;

            //find hi/lo cells
            if (cellVoltageUnderTest < loCellVoltage) { loCellVoltage = cellVoltageUnderTest; }
            if (cellVoltageUnderTest > hiCellVoltage) { hiCellVoltage = cellVoltageUnderTest; }

            //check for new maxEver/minEver cells (if any)
            //If BATTERY_TYPE_5AhG3 is defined, cell 19 voltage cannot become maxEver or minEver right now, but we'll check again down below
            if (cellVoltageUnderTest > LTC68042result_maxEverCellVoltage_get()) {LTC68042result_maxEverCellVoltage_set(cellVoltageUnderTest); }
            if (cellVoltageUnderTest < LTC68042result_minEverCellVoltage_get()) {LTC68042result_minEverCellVoltage_set(cellVoltageUnderTest); }

            LTC68042result_specificCellVoltage_set(chip, cell, cellVoltageUnderTest);
        }
    }

    LTC68042result_packVoltage_set( (uint8_t)(packVoltage_RAW * 0.0001) );

    LTC68042result_loCellVoltage_set(loCellVoltage);
    LTC68042result_hiCellVoltage_set(hiCellVoltage);

    #ifdef BATTERY_TYPE_5AhG3
        //Now we need to determine which cell 19 voltage is correct (the actual measured value, or the current-adjusted one)
        //We do this by determining which voltage has the smallest magnitude from the max/min cell voltages (determined above).

        uint16_t midpointVoltage = ((hiCellVoltage - loCellVoltage) >> 1) + loCellVoltage;
        uint16_t cell19deltaMagnitude_measured = 0;
        uint16_t cell19deltaMagnitude_adjusted = 0;

        //find measured voltage magnitude from midpoint
        if (cell19Voltage_measured > midpointVoltage) { cell19deltaMagnitude_measured = cell19Voltage_measured - midpointVoltage; }
        else                                          { cell19deltaMagnitude_measured = midpointVoltage - cell19Voltage_measured; }

        //find adjusted voltage magnitude from midpoint
        if (cell19Voltage_adjusted > midpointVoltage) { cell19deltaMagnitude_adjusted = cell19Voltage_adjusted - midpointVoltage; }
        else                                          { cell19deltaMagnitude_adjusted = midpointVoltage - cell19Voltage_adjusted; }

        uint16_t cell19Voltage_final = 0;

        if (cell19deltaMagnitude_measured > cell19deltaMagnitude_adjusted) { cell19Voltage_final = cell19Voltage_adjusted; } //adjusted value is closer to midpoint
        else                                                               { cell19Voltage_final = cell19Voltage_measured; } //measured value is closer to midpoint

        //store whichever cell 19 voltage is closest to the other cells
        LTC68042result_specificCellVoltage_set(CELL19_CHIP_NUMBER, CELL19_CELL_NUMBER, cell19Voltage_final);

        //finally, we need to check if cell 19 is either the highest or lowest voltage
        if (cell19Voltage_final > hiCellVoltage) { LTC68042result_hiCellVoltage_set(cell19Voltage_final); }
        if (cell19Voltage_final < loCellVoltage) { LTC68042result_loCellVoltage_set(cell19Voltage_final); }
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

//either gather next QTY3 cell voltages from LTC6804, or process a complete batch of returned cell voltage data.
//raw cell voltages are stored in file-scoped "cellVoltages_counts[][]"" array
//latest validated results are stored in a different, globally-accessible array inside "LTC68042_result.c"
//Example with QTY48 cells:
//  -the absolute first call starts a conversion.
//  After that, the behavior is as follows:
//  -the next sixteen calls ( (48 cells) / (3 cells per call) = 16 calls ) read back QTY48 cell voltages.
//  -The seventeenth call performs all pack voltage math and stores valid results in LTC68042_result.c
//
//returns false while gathering data, true each time all data is processed
bool LTC68042cell_nextVoltages(void)
{
    static uint8_t presentState = LTC_STATE_FIRSTRUN;
    static bool conversionInProcess = false;
    bool cellVoltageDataStatus = GATHERING_CELL_DATA;

    if (LTC68042configure_wakeup() == LTC6804_CORE_JUST_WOKE_UP) { presentState = LTC_STATE_FIRSTRUN; }

    if (presentState == LTC_STATE_GATHER)
    { //retrieve next CVR from LTC, then validate and store in cellVoltages_counts[][] array

        //round-robin state handlers
        static uint8_t chipAddress = FIRST_IC_ADDR;
        static char cellVoltageRegister = 'A'; //LTC68042 contains QTY4 CVRs (A/B/C/D)

        //wait for current conversion to complete (should not usually be necessary)
        if (conversionInProcess)
        {
            while (LTC6804_MAX_CONVERSION_TIME_ms > (millis() - conversionStart_ms)) { ; }
            conversionInProcess = false;
        }

        validateAndStoreNextCVR(chipAddress, cellVoltageRegister);

        //determine which LTC68042 IC & CVR to read next
        cellVoltageRegister++;
        if (cellVoltageRegister >= 'E')
        {
            //LTC6804 only has registers A,B,C,D
            cellVoltageRegister = 'A'; //reset back to first CVR

            if (++chipAddress >= (FIRST_IC_ADDR + TOTAL_IC))
            {
                //just finished reading last IC's last CVR... all cell voltages stored in cellVoltages_counts[][]
                startCellConversion(); //start the next cell conversion //takes a while to finish
                conversionInProcess = true;

                chipAddress = FIRST_IC_ADDR; //reset to first LTC IC
                presentState = LTC_STATE_PROCESS; //all cell voltages gathered.  Process data on next run.
            }
        }
    }

    else if (presentState == LTC_STATE_PROCESS)
    {
        //all cell voltages read...
        processAllCellVoltages(); //do math and store in LTC68042_result.c
        cellVoltageDataStatus = CELL_DATA_PROCESSED;
        presentState = LTC_STATE_GATHER; //gather data on next run
    }

    else if (presentState == LTC_STATE_FIRSTRUN)
    {
        //LTC6804 ICs were previously off
        LTC68042configure_programVolatileDefaults();
        startCellConversion();
        conversionInProcess = true;
        presentState = LTC_STATE_GATHER;
    }

    else
    {
        Serial.print(F("\nillegal LTC68042cell state"));
        while (1) {;} //hang here until watchdog resets.
    }

    return cellVoltageDataStatus;
}

/////////////////////////////////////////////////////////////////////////////////////////

//Only call when keyOFF //takes too long to execute when keyON (causes check engine light)
//Results are stored in "LTC68042_results.c"
//JTS2doNext: rewrite to remove double call hack
void LTC68042cell_acquireAllCellVoltages(void)
{
    while (LTC68042cell_nextVoltages() != CELL_DATA_PROCESSED) { ; } //clear old data (if any)
    while (LTC68042cell_nextVoltages() != CELL_DATA_PROCESSED) { ; } //gather new data
}

/////////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Write Test
void LTC68042cell_openShortTest(void)
{
    // 1a: Turn off all sense resistors
    void LTC68042configure_setBalanceResistors(uint8_t icAddress, uint16_t cellBitmap, uint8_t softwareTimeout);

    // 1b: Measure all cell voltages
    LTC68042cell_acquireAllCellVoltages();

    // 1c: Verify all cells between 2.0:2.1 volts

    // 2a: Turn on all odd sense resistors


    // 2b: Measure all cell voltages


    // 2c: Verify all odd cells less than 1 volt


    // 2d: Verify all even cells greater than 4 volts


    // 3a: Turn on all even sense resistors


    // 3b: Measure all cell voltages


    // 3c: Verify all odd cells greater than 4 volts


    // 3d: Verify all even cells less than 1 volt


    // cleanup: Turn off all sense resistors
}

/////////////////////////////////////////////////////////////////////////////////////////
