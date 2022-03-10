//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
#include "libcm.h"

//updated by gridCharger_handler() //prevents mid-loop state changes from affecting loop logic
bool gridChargerState_sampled = PLUGGED_IN;

//////////////////////////////////////////////////////////////////////////////////

bool gridCharger_didStateChange(void)
{
  static bool gridChargerState_previous = UNPLUGGED;

  bool didGridChargerStateChange = NO;

  if( gridChargerState_sampled != gridChargerState_previous )
  {
    didGridChargerStateChange = YES;
    gridChargerState_previous = gridChargerState_sampled;
  }

  return didGridChargerStateChange;
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handleUnplugEvent(void)
{
  Serial.print(F("Unplugged"));
  gpio_turnGridCharger_off();
  lcd_displayOFF();
  gpio_setGridCharger_powerLevel('H'); //reduces power consumption
  gpio_turnBuzzer_off(); //if issue persists, something else will turn buzzer back on
  SoC_updateUsingOpenCircuitVoltage();
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handlePluginEvent(void)
{
  Serial.print(F("Plugged In"));
  lcd_displayOn();
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_chargePack(void)
{
  static uint8_t cellState = CELLSTATE_UNINITIALIZED;
  static uint8_t cellStatePrevious = CELLSTATE_UNINITIALIZED;

  lcd_refresh();  
  debugUSB_printLatest_data_gridCharger();

  if( LTC68042result_hiCellVoltage_get() > CELL_VREST_85_PERCENT_SoC )
  {
    //at least one cell is overcharged
    cellState = CELLSTATE_OVERCHARGED;
    gpio_turnGridCharger_off();
    gpio_setGridCharger_powerLevel('0');
    //JTS2doNow: display Warning on LCD
    gpio_turnBuzzer_on_highFreq();
  }

  else if( (LTC68042result_loCellVoltage_get() < CELL_VMIN_GRIDCHARGER) )
  {
    //at least one cell is too empty to safely grid charge
    cellState = CELLSTATE_OVERDISCHARGED;
    gpio_turnGridCharger_off();
    gpio_setGridCharger_powerLevel('0');
    //JTS2doNow: display Warning on LCD
    gpio_turnBuzzer_on_highFreq();
  }

  else if( (LTC68042result_hiCellVoltage_get() > CELL_VMAX_GRIDCHARGER) )
  {
    //at least one cell is full
    cellState = CELLSTATE_SOMECELLSFULL;
    gpio_turnGridCharger_off();
    gpio_setGridCharger_powerLevel('0');
    gpio_turnBuzzer_off();
  }

  else if( LTC68042result_hiCellVoltage_get() <= (CELL_VMAX_GRIDCHARGER - VCELL_HYSTERESIS) )
  {
    //no cells are full
    cellState = CELLSTATE_ZEROCELLSFULL;
    gpio_turnGridCharger_on();
    gpio_setGridCharger_powerLevel('H');
    gpio_turnBuzzer_off();
  }

  if (cellStatePrevious != cellState)
  {
    //charging state has changed
    Serial.print(F("\nGrid: "));
    switch (cellState)
    {
      case CELLSTATE_UNINITIALIZED:  Serial.print(F("init")          ); break;
      case CELLSTATE_OVERCHARGED:    Serial.print(F("Overcharged")   ); break;
      case CELLSTATE_OVERDISCHARGED: Serial.print(F("Overdischarged")); break;
      case CELLSTATE_SOMECELLSFULL:  Serial.print(F("Charged")       ); break;
      case CELLSTATE_ZEROCELLSFULL:  Serial.print(F("Charging")      ); break;
    }
  }

  cellStatePrevious = cellState;
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handler(void)
{
  gridChargerState_sampled = gpio_isGridChargerPluggedInNow(); //Nowhere else in this file should query actual I/O... use this variable instead

  if( gridCharger_didStateChange() == YES )
  {
    Serial.print(F("\nCharger: "));
    if (gridChargerState_sampled == PLUGGED_IN) { gridCharger_handlePluginEvent(); }
    if (gridChargerState_sampled == UNPLUGGED ) { gridCharger_handleUnplugEvent(); }
  }

  if(gridChargerState_sampled == PLUGGED_IN) { gridCharger_chargePack(); }
}