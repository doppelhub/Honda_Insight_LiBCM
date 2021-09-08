//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
#include "libcm.h"

//updated by gridCharger_handler() (prevents mid-loop state changes from affecting loop logic)
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
  gpio_setGridCharger_powerLevel('H'); //reduces power consumption
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

//called when (at least one cell is full) && (no cell is severely overcharged)
void gridCharger_balanceCells(void)
{
  ;
  //JTS2doNow: Implement balancing
  //if min cell voltage greater than 3.8 volts, check each voltage.
  //If a particular cell voltage is more than 1 mV higher than minimum voltage:
  //turn fans on high
  //enable discharge resistor

}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_chargePack(void)
{
  lcd_refresh();
  LTC68042cell_nextVoltages(); 
  debugUSB_printLatest_data();

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
    gridCharger_balanceCells();
  }

  //grid charger plugged in and all cells less than full
  else if( LTC68042result_hiCellVoltage_get() <= (GRID_CHARGER_CELL_VMAX - VCELL_HYSTERESIS) )
  {
    gpio_turnGridCharger_on();
    gpio_setFanSpeed('M');
    gpio_setGridCharger_powerLevel('H');
  }
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

  if(gridCharger_getSampledState() == PLUGGED_IN) { gridCharger_chargePack(); }
}