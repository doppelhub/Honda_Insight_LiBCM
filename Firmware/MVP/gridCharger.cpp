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
  analogWrite(PIN_FAN_PWM,0);     //turn onboard fans off
  digitalWrite(PIN_GRID_EN,0);    //turn grid charger off
  Serial.print(F("\nGrid Charger Disabled"));
  lcd_displayOFF();
  LED(4,LOW);
  //JTS2doNow: Turn 
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handlePluginEvent(void)
{
  Serial.print(F("Plugged In"));
  lcd_displayON();
  LED(4,HIGH);
  //JTS2doNow: Turn LiBCM fans on
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_handler(void)
{
  gridChargerState_sampled = gpio_isGridChargerPluggedInNow(); //this is the only time LiBCM samples if plugged in

  if( gridCharger_didStateChange() == YES )
  {
    Serial.print(F("\nGrid Charger: "));
    if (gridCharger_getSampledState() == PLUGGED_IN)
    {
      gridCharger_handlePluginEvent();
      gpio_turnGridCharger_on(); //force charger on if(GRID_CHARGER_CELL_VMAX - VCELL_HYSTERESIS) < VCELL < GRID_CHARGER_CELL_VMAX)
      gpio_setFanSpeed('M'); 
    }
    if (gridCharger_getSampledState() == UNPLUGGED ) { gridCharger_handleUnplugEvent(); }
  }

  gridCharger_balanceCells();
}

//////////////////////////////////////////////////////////////////////////////////

//To prevent overchanging, do not call outside gridCharger_handler()!
void gridCharger_balanceCells(void)
{
  if(gridCharger_getSampledState() == PLUGGED_IN) 
  {
    //at least one cell is full
    if( (LTC68042result_hiCellVoltage_get() > GRID_CHARGER_CELL_VMAX) )
    {
      gpio_turnGridCharger_off();
      gpio_setFanSpeed('L');
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