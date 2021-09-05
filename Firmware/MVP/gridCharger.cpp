//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
#include "libcm.h"

//updates when gridCharger_handler() called (prevents mid-loop state changes from affecting loop logic)
bool gridChargerState_sampled = PLUGGED_IN;

//////////////////////////////////////////////////////////////////////////////////

bool gridCharger_getSampledState(void) { return gridChargerState_sampled; }

//////////////////////////////////////////////////////////////////////////////////

bool gridCharger_didStateChange(void)
{
  gridChargerState_sampled = gpio_isGridChargerPluggedInNow(); //this is the only time LiBCM samples if plugged in

  static bool gridChargerState_previous = UNPLUGGED;

  bool didGridChargerStateChange = NO;

  if( gridCharger_getSampledState() != gridChargerState_previous )
  {
    didGridChargerStateChange = YES;
    gridChargerState_previous = gridCharger_getSampledState();
  }

  return didGridChargerStateChange;
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handleUnplugEvent(void)
{
  Serial.print(F("Unplugged"));
  gpio_setFanSpeed('0');
  gpio_turnGridCharger_off();
  lcd_displayOFF();
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handlePluginEvent(void)
{
  Serial.print(F("Plugged In"));
  lcd_displayON();
  gpio_turnGridCharger_on(); //set charger initial condition //gridCharger_balanceCells() will immediately disable if full.
  gpio_setFanSpeed('M'); 
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handler(void)
{
  if( gridCharger_didStateChange() == YES )
  {
    Serial.print(F("\nGrid Charger: "));
    if (gridCharger_getSampledState() == PLUGGED_IN) { gridCharger_handlePluginEvent(); }
    if (gridCharger_getSampledState() == UNPLUGGED ) { gridCharger_handleUnplugEvent(); }
  }

  lcd_refresh();

  gridCharger_balanceCells();
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_balanceCells(void)
{
  if(gridCharger_getSampledState() == PLUGGED_IN) 
  {
    LTC68042cell_nextVoltages(); 

    //at least one cell is severely overcharged
    if( LTC68042result_hiCellVoltage_get() > (GRID_CHARGER_CELL_VMAX + VCELL_HYSTERESIS) )
    {
      gpio_turnGridCharger_off();
      gpio_setFanSpeed('H'); //cool pack and discharge
      gpio_setGridCharger_powerLevel('0');
      //JTS2doLater: display Warning on LCD
    }

    //at least one cell is full
    else if( (LTC68042result_hiCellVoltage_get() > GRID_CHARGER_CELL_VMAX) )
    {
      gpio_turnGridCharger_off();
      gpio_setFanSpeed('0');
      gpio_setGridCharger_powerLevel('0');
    }
  
    //grid charger plugged in and all cells less than full
    //note: when first plugged in, grid charger won't turn on if cells almost full
    else if( LTC68042result_hiCellVoltage_get() <= (GRID_CHARGER_CELL_VMAX - VCELL_HYSTERESIS) )
    {
      //JTS2doLater: add balancing
      gpio_turnGridCharger_on();
      gpio_setFanSpeed('M');
      gpio_setGridCharger_powerLevel('H');
    }
  }

  //grid charger not plugged in
  else
  {
    gpio_turnGridCharger_off();
    gpio_setGridCharger_powerLevel('H'); //reduces power consumption
    gpio_setFanSpeed('0');
  }
}