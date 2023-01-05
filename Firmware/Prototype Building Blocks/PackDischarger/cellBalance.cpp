//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//cell balancing Functions

#include "libcm.h"

bool cellsAreBalanced = true;

/////////////////////////////////////////////////////////////////////////////////////////////

//JTS2doNow: Add an array that accumulates how long each cell is discharging
//pseudocode:
// uint16_t cellBalanceTimer_seconds[NUMCELLS]=0;
// uint8_t balancingComplete = false;

// if( gridChargerJustPluggedIn ) { clear cellBalanceTimer_seconds(); balancingComplete = false; } //set all array elements to zero

// if( balancingFinished ) { balancingComplete = true; } //the first time balancing finishes, we stop accumulating the cell timers

// if( hasOneSecondPassed && (balancingComplete == FALSE) ) {
//    for(cellNumber=1; cellNumber<NUMCELLS; cellNumber++) {
//       if(cellStatus[cellNumber] == BALANCING) { cellBalanceTimer_seconds[cellNumber]++; }
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

/////////////////////////////////////////////////////////////////////////////////////////////

//JTS2doNow: When balancing cells, we only need to check cell voltage every minute (to save power).
//Need to add discharge software timeout (if not already present), as LTC ICs turn off after a couple seconds if we're not sending commands

void cellBalance_configureDischargeResistors(void)
{   
  uint16_t cellsToDischarge[TOTAL_IC] = {0}; //each uint16's QTY12 LSBs correspond to each LTC6804's QTY12 cells
  static uint8_t balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT;

  cellsAreBalanced = true; //code below will set false if any cell(s) unbalanced

  uint16_t cellDischargeVoltageThreshold = 0; //cells above this value will get discharged

  //JTS2doNow: Add a similar case (without alarm) that discharges pack down to 85% SoC (to maximize battery life)
  //determine cellDischargeVoltageThreshold       
  if (LTC68042result_hiCellVoltage_get() > CELL_VMAX_REGEN )
  {
    //pack is overcharged
    cellDischargeVoltageThreshold = CELL_VMAX_REGEN;
    Serial.print(F("\nDANGER: Cells Overcharged!!"));
  } 
  else { cellDischargeVoltageThreshold = LTC68042result_loCellVoltage_get() + balanceHysteresis; } //pack isn't overcharged

  //determine which cells to balance
  for(uint8_t ic = 0; ic < TOTAL_IC; ic++)
  {
    for (uint8_t cell = 0; cell < CELLS_PER_IC; cell++)
    {
      if(LTC68042result_specificCellVoltage_get(ic, cell) > cellDischargeVoltageThreshold)
      { 
        //this cell voltage is higher than the lowest cell voltage + hysteresis
        cellsToDischarge[ic] |= (1 << cell); //this cell will be discharged
        cellsAreBalanced = false;
        balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT;
      }
    }

    debugUSB_setCellBalanceStatus(ic, cellsToDischarge[ic], cellDischargeVoltageThreshold);
    LTC68042configure_setBalanceResistors((ic + FIRST_IC_ADDR), cellsToDischarge[ic], LTC6804_DISCHARGE_TIMEOUT_02_SECONDS);
  }

  if(cellsAreBalanced == true)
  { 
    balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE;
    gpio_setFanSpeed_PCB('0');
    //JTS2doNow: disable software timer (so LTCs turn off after two seconds) //possibly already implemented?
  } 
  else { gpio_setFanSpeed_PCB('H'); }
}

/////////////////////////////////////////////////////////////////////////////////////////////

//'cellsAreBalanced' variable is updated by calling cellBalance_configureDischargeResistors()
bool cellBalance_areCellsBalanced(void) { return cellsAreBalanced; }

/////////////////////////////////////////////////////////////////////////////////////////////

void cellBalance_handler(void)
{
  static uint8_t balanceState = BALANCING_DISABLED;

  if( (gpio_isGridChargerPluggedInNow() == PLUGGED_IN) )
  {
    //balance cells (if needed)
    cellBalance_configureDischargeResistors();
    balanceState = BALANCING_ALLOWED;
  }
}