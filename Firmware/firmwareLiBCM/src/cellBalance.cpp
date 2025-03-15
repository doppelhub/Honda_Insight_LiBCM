//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//cell balancing Functions

#include "libcm.h"

bool cellsAreBalancing = NO;

/////////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Add an array that accumulates how long each cell is discharging
//pseudocode:
// uint16_t cellBalanceTimer_seconds[NUMCELLS]=0;
// uint8_t balancingComplete = false;

// if (gridChargerJustPluggedIn) { clear cellBalanceTimer_seconds(); balancingComplete = false; } //set all array elements to zero

// if (balancingFinished) { balancingComplete = true; } //the first time balancing finishes, we stop accumulating the cell timers

// if (hasOneSecondPassed && (balancingComplete == FALSE)) {
//    for (cellNumber=1; cellNumber<NUMCELLS; cellNumber++) {
//       if (cellStatus[cellNumber] == BALANCING) { cellBalanceTimer_seconds[cellNumber]++; }
// }} 

//Allow LiBCM to print this array over USB
// -if all cells are similar, then all array elements should have similar values (ideally they would all be 0).
// -cells with less capacity will self-discharge more (at rest, at idle etc).
// -cells with more capacity will span a narrower voltage range (compared to cells with less capacity).
// -The absolute value stored for each cell in the cellBalanceTimer_seconds array isn't important, but the difference between the various cells is.
// -Outlier cells (i.e. that spend more or less time balancing) are suspect.
// -a weaker cell will tend to self-discharge more often than healthy cells.
//Since healthy cells won't lose this energy (through self-discharge), they'll spend more time balancing.
//(Theory) Therefore, weaker cells will likely tend to have lower stored values in the cellBalanceTimer_seconds array.

/////////////////////////////////////////////////////////////////////////////////////////

bool cellBalance_areCellsBalancing(void) { return cellsAreBalancing; }

/////////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Always allow discharge balancing when a cell is overcharged (for safety)
//JTS2doLater: Write keyOff test that measures each cell voltage twice: once with discharge resistor off, and again with resistor on.
//             Then verify voltage drop, which means the discharge resistor is turning off and on.  If there isn't enough resolution,
//             another method would be to wait a few hours for pack voltages to settle, then log all cell voltages an hour apart.
//JTS2doLater: Add per-cell SoC, to allow balancing at any SoC (see icn.net:post#1502833,comment#579)
//balance cells (if needed)
void configureDischargeResistors(void)
{   
    static uint8_t balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT;
    uint16_t cellDischargeVoltageThreshold = 0; //cells above this value are discharged
    uint16_t cellsToDischarge[TOTAL_IC] = {0}; //each uint16's QTY12 LSBs correspond to each LTC6804's QTY12 cells

    cellsAreBalancing = NO;

    cellDischargeVoltageThreshold = 35900;

    //determine which cells to balance
    for (uint8_t ic = 0; ic < TOTAL_IC; ic++)
    {
        for (uint8_t cell = 0; cell < CELLS_PER_IC; cell++)
        {
            if (LTC68042result_specificCellVoltage_get(ic, cell) > cellDischargeVoltageThreshold)
            { 
                //this cell voltage is higher than the lowest cell voltage + hysteresis
                cellsToDischarge[ic] |= (1 << cell); //this cell will be discharged
                cellsAreBalancing = YES;
                balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT;
            }
        }

        debugUSB_setCellBalanceStatus(ic, cellsToDischarge[ic], cellDischargeVoltageThreshold);
        LTC68042configure_setBalanceResistors((ic + FIRST_IC_ADDR), cellsToDischarge[ic], LTC6804_DISCHARGE_TIMEOUT_02_SECONDS);
    }

    if (cellsAreBalancing == NO) { balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE; } 
}

/////////////////////////////////////////////////////////////////////////////////////////

void disableDischargeResistors(void)
{
    //to save power, only called once each time balancing is disabled
    const uint16_t cellsToDischarge = 0;

    for (uint8_t ic = 0; ic < TOTAL_IC; ic++)
    {
        debugUSB_setCellBalanceStatus(ic, cellsToDischarge, CELL_VMAX_REGEN);
        LTC68042configure_setBalanceResistors((ic + FIRST_IC_ADDR), cellsToDischarge, LTC6804_DISCHARGE_TIMEOUT_02_SECONDS);
    }
    cellsAreBalancing = NO;
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t isBalancingAllowed(void)
{
    //thermal checks
    if (temperature_battery_getLatest()      > CELL_BALANCE_MAX_TEMP_C) { return NO__BATTERY_IS_HOT;          }
    
    return YES__BALANCING_ALLOWED;
}

/////////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: balance majorly imbalanced cells even when grid charger is unplugged
void cellBalance_handler(void)
{
    static uint8_t isBalancingAllowed_previous = NO__UNINITIALIZED;
           uint8_t isBalancingAllowed_now      = isBalancingAllowed();

    if (isBalancingAllowed_now == YES__BALANCING_ALLOWED)
    {
        if (time_isItTimeToPerformKeyOffTasks() == YES) { configureDischargeResistors(); }
    }
    else if (isBalancingAllowed_previous == YES__BALANCING_ALLOWED) { disableDischargeResistors(); }
    
    isBalancingAllowed_previous = isBalancingAllowed_now;
}

/////////////////////////////////////////////////////////////////////////////////////////
