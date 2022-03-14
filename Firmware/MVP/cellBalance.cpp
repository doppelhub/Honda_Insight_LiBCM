//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//cell balancing Functions

#include "libcm.h"

/////////////////////////////////////////////////////////////////////////////////////////////

void cellBalance_disableBalanceResistors(void)
{
  for(uint8_t ic = 0; ic < TOTAL_IC; ic++)
  { 
    LTC68042configure_setBalanceResistors( (ic + FIRST_IC_ADDR), 0 );
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void cellBalance_configureDischargeResistors(void)
{   
  uint16_t cellsToDischarge[TOTAL_IC] = {0}; //each uint16's QTY12 LSBs correspond to each LTC6804's QTY12 cells
  static uint8_t balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT;

  bool cellsAreBalanced = true;

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
        cellsToDischarge[ic] |= (1 << cell);
        cellsAreBalanced = false;
        gpio_setFanSpeed('M', RAMP_FAN_SPEED);
        balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT;
      }
    }

    debugUSB_setCellBalanceStatus(ic, cellsToDischarge[ic]);
    LTC68042configure_setBalanceResistors( (ic + FIRST_IC_ADDR), cellsToDischarge[ic] );
  }

  if(cellsAreBalanced == true)
  { 
    balanceHysteresis = CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE;
    gpio_setFanSpeed('0', RAMP_FAN_SPEED);
  }
  else { gpio_setFanSpeed('H', RAMP_FAN_SPEED); } //cool discharge resistors
}

/////////////////////////////////////////////////////////////////////////////////////////////

void cellBalance_handler(void)
{
  static uint8_t balanceState = BALANCING_DISABLED;

  #ifdef ONLY_BALANCE_CELLS_WHEN_GRID_CHARGER_PLUGGED_IN
    if( gpio_isGridChargerPluggedInNow() == PLUGGED_IN )
  #else 
    if( (gpio_isGridChargerPluggedInNow() == PLUGGED_IN) ||
        (SoC_getBatteryStateNow_percent() > CELL_BALANCE_MIN_SoC) )
  #endif
    {
      //balance cells (if needed)
      cellBalance_configureDischargeResistors();
      balanceState = BALANCING_ALLOWED;
    }

    else if(balanceState == BALANCING_ALLOWED)
    {
      //pack SoC previously high enough to balance, but isn't now
      cellBalance_disableBalanceResistors(); //only send this command once //saves power
      gpio_setFanSpeed('0', ABSOLUTE_FAN_SPEED);
      balanceState = BALANCING_DISABLED;
    }
}