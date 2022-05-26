//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//cell balancing Functions

#include "libcm.h"

bool cellsAreBalanced = true;

/////////////////////////////////////////////////////////////////////////////////////////////

void cellBalance_configureDischargeResistors(void)
{   
  uint16_t cellsToDischarge[TOTAL_IC] = {0}; //each uint16's QTY12 LSBs correspond to each LTC6804's QTY12 cells
  static uint8_t balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT;

  cellsAreBalanced = true; //code below will set false if cells unbalanced

  uint16_t cellDischargeVoltageThreshold = 0; //cells above this value will get discharged

  //JTS2doNow: Add a similar case (without alarm) that discharges pack down to 85% SoC (to maximize battery life)
  //determine cellDischargeVoltageThreshold       
  if (LTC68042result_hiCellVoltage_get() > CELL_VMAX_REGEN )
  {
    //pack is overcharged
    cellDischargeVoltageThreshold = CELL_VMAX_REGEN;
    Serial.print(F("\nDANGER: Cells Overcharged!!"));
  } 
  else { cellDischargeVoltageThreshold = LTC68042result_loCellVoltage_get(); } //pack isn't overcharged //balance to minimum cell voltage

  //determine which cells to balance
  for(uint8_t ic = 0; ic < TOTAL_IC; ic++)
  {
    for (uint8_t cell = 0; cell < CELLS_PER_IC; cell++)
    {
      if(LTC68042result_specificCellVoltage_get(ic, cell) > (cellDischargeVoltageThreshold + balanceHysteresis) )
      { 
        //this cell voltage is higher than the lowest cell voltage
        cellsToDischarge[ic] |= (1 << cell); //this cell will be discharged
        cellsAreBalanced = false;
        balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT;
      }
    }

    debugUSB_setCellBalanceStatus(ic, cellsToDischarge[ic], cellDischargeVoltageThreshold + balanceHysteresis);
    LTC68042configure_setBalanceResistors((ic + FIRST_IC_ADDR), cellsToDischarge[ic], LTC6804_DISCHARGE_TIMEOUT_00_SECONDS);
  }

  if(cellsAreBalanced == true)
  { 
    balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE;
    //JTS2doNow: disable software timer (so LTCs turn off after two seconds)

    if(temperature_battery_getLatest() > WHEN_GRID_CHARGING_COOL_PACK_ABOVE_TEMP) { gpio_setFanSpeed('H', RAMP_FAN_SPEED); }
    else                                                                          { gpio_setFanSpeed('0', RAMP_FAN_SPEED); }
    //JTS2doNow: Need to add a bitmask so multiple sources can turn fan on/off... fan on unless all sources have cleared their respective bit.
  }
  else { gpio_setFanSpeed('H', IMMEDIATE_FAN_SPEED); } //cool discharge resistors
}

/////////////////////////////////////////////////////////////////////////////////////////////

//'cellsAreBalanced' variable is updated by calling cellBalance_configureDischargeResistors()
bool cellBalance_wereCellsBalanced(void) { return cellsAreBalanced; }

/////////////////////////////////////////////////////////////////////////////////////////////

void cellBalance_handler(void)
{
  static uint8_t balanceState = BALANCING_DISABLED;

  //JTS2doNow: If cabin air sensor is too high, don't allow cell balancing (for now we're looking at battery module temperature)
  //running the fans will increase battery temp if cabin air temp is too high...
  //...but the fans need to run to remove the waste heat from the discharge resistors
  #ifdef ONLY_BALANCE_CELLS_WHEN_GRID_CHARGER_PLUGGED_IN
    if( (gpio_isGridChargerPluggedInNow() == PLUGGED_IN) &&
        (temperature_battery_getLatest() < CELL_BALANCE_MAX_TEMP_C) )
  #else 
    if(     (temperature_battery_getLatest() < CELL_BALANCE_MAX_TEMP_C) &&
        (   (gpio_isGridChargerPluggedInNow() == PLUGGED_IN) ||
          ( (SoC_getBatteryStateNow_percent() > CELL_BALANCE_MIN_SoC) && (time_hasKeyBeenOffLongEnough() == true)
          )
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
      gpio_setFanSpeed('0', IMMEDIATE_FAN_SPEED);
      balanceState = BALANCING_DISABLED;
    }
}