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
    LTC68042configure_setBalanceResistors((ic + FIRST_IC_ADDR), cellsToDischarge[ic], LTC6804_DISCHARGE_TIMEOUT_00_SECONDS);
  }

  if(cellsAreBalanced == true)
  { 
    balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE;
    //JTS2doNow: disable software timer (so LTCs turn off after two seconds) //possibly already implemented?

    //JTS2doNow: Move to grid charger handler?
    if( (temperature_battery_getLatest() > WHEN_GRID_CHARGING_COOL_PACK_ABOVE_TEMP) &&
        (gpio_isGridChargerPluggedInNow() == PLUGGED_IN) )
    {
      fan_requestSpeed(FAN_PCB, FAN_REQUESTOR_CELLBALANCE, FAN_HIGH);
      //JTS2doNow:
      //A) cool a warm pack if intake air is cooler.
      //B) warm a cool pack if intake air is warmer.
      //C) only balance cells when grid charger plugged in.
      //D) only balance cells when majorly imbalanced, unless grid charger plugged in.
    }
    else //grid charger not plugged in or battery isn't too hot
    { 
      fan_requestSpeed(FAN_PCB, FAN_REQUESTOR_CELLBALANCE, FAN_OFF);
    }
  }
  else //(cellsAreBalanced == false)
  {
    fan_requestSpeed(FAN_PCB, FAN_REQUESTOR_CELLBALANCE, FAN_LOW); //cool discharge resistors
  } 
}

/////////////////////////////////////////////////////////////////////////////////////////////

//'cellsAreBalanced' variable is updated by calling cellBalance_configureDischargeResistors()
bool cellBalance_areCellsBalanced(void) { return cellsAreBalanced; }

/////////////////////////////////////////////////////////////////////////////////////////////

void cellBalance_handler(void)
{
  static uint8_t balanceState = BALANCING_DISABLED;

  //JTS2doNow: Should we only balance cells when the pack is nearly charged?  See post#1502833, comment#579 @ ic.net
  //JTS2doNow: If cabin air sensor is too high, don't allow cell balancing (for now we're looking at battery module temperature)
  //running the fans will increase battery temp if cabin air temp is too high...
  //...but the fans need to run to remove the waste heat from the discharge resistors
  #ifdef ONLY_BALANCE_CELLS_WHEN_GRID_CHARGER_PLUGGED_IN
    if( (gpio_isGridChargerPluggedInNow() == PLUGGED_IN) &&
        (temperature_battery_getLatest() < CELL_BALANCE_MAX_TEMP_C) )
  #else 
    if( (temperature_battery_getLatest() < CELL_BALANCE_MAX_TEMP_C) && //JTS2doNow: prevents fans from turning on //need to move fan logic to fan handler.
        ( (gpio_isGridChargerPluggedInNow() == PLUGGED_IN) ||
          ((SoC_getBatteryStateNow_percent() > CELL_BALANCE_MIN_SoC) && (time_hasKeyBeenOffLongEnough_toEstimateSoC() == true))
        )
      )
  #endif
    {
      //balance cells (if needed)
      cellBalance_configureDischargeResistors();
      balanceState = BALANCING_ALLOWED;
    }

    else if(balanceState == BALANCING_ALLOWED)
    {
      //pack SoC previously high enough to balance, but isn't now
      //this code only runs once (i.e. when the above if statement state changes) to save power
      LTC68042configure_programVolatileDefaults(); //disable discharge resistors and software timer
      fan_requestSpeed(FAN_PCB, FAN_REQUESTOR_CELLBALANCE, FAN_OFF);
      balanceState = BALANCING_DISABLED;
    }
}