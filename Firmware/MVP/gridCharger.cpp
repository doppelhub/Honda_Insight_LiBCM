//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
#include "libcm.h"

//updates when gridCharger_stateChangeHandler() called (prevents mid-loop state changes from affecting loop logic)
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

void gridCharger_stateChangeHandler(void)
{
  gridChargerState_sampled = gpio_isGridChargerPluggedIn(); //this is the only time LiBCM samples if plugged in

  if( gridCharger_didStateChange() == YES )
  {
    Serial.print(F("\nGrid Charger: "));
    if (gridCharger_getSampledState() == PLUGGED_IN) { gridCharger_handlePluginEvent(); }
    if (gridCharger_getSampledState() == UNPLUGGED ) { gridCharger_handleUnplugEvent(); }
  }
}

//////////////////////////////////////////////////////////////////////////////////

void gridCharger_balanceCells(void)
{
  //JTS2doNow: make this work
  //JTS2doLater: add balancing (right now charger turns off when first cell is full)
  //grid charger handling
  if( (LTC68042result_hiCellVoltage_get() <= 39000) && !(digitalRead(PIN_GRID_SENSE)) ) //Battery not full and grid charger is plugged in
  {
    if( LTC68042result_hiCellVoltage_get() <= 38950) //hysteresis to prevent rapid cycling
    {
      digitalWrite(PIN_GRID_EN,1); //enable grid charger
      analogWrite(PIN_FAN_PWM,125); //enable fan
    }
  }
  else
  {  //battery is full or grid charger isn't plugged in
    digitalWrite(PIN_GRID_EN,0); //disable grid charger
    analogWrite(PIN_FAN_PWM,0); //disable fan
  }
}