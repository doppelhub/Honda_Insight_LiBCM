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

//JTS2doNow: Think this through to make sure it won't over-discharge cells
//only call when:
//grid charger is plugged in
//minimum cell voltage above safe minimum 
//no cell is severely overcharged
void gridCharger_balanceCells(void)
{
  //JTS2doNow: Implement balancing
   
  uint16_t cellsToDischarge[TOTAL_IC] = {0};

  bool isAtLeastOneCellUnbalanced = false;

  if(LTC68042result_loCellVoltage_get() > 37500) //JTS2doLater: Figure out final minimum voltage
  { //all cells above minimum balancing voltage
    for(uint8_t ic = 0; ic < TOTAL_IC; ic++)
    {
      for (uint8_t cell = 0; cell < CELLS_PER_IC; cell++)
      {
        if(LTC68042result_specificCellVoltage_get(ic, cell) > (LTC68042result_loCellVoltage_get() + ONE_MILLIVOLT_COUNTS) )
        { //this particular cell's voltage is more than one mV above the lowest cell
          cellsToDischarge[ic] |= (1 << cell);
          isAtLeastOneCellUnbalanced = true;
        }
      }

    debugUSB_setCellBalanceStatus(ic, cellsToDischarge[ic]);
    LTC68042configure_cellBalancing_setCells( (ic + FIRST_IC_ADDR), cellsToDischarge[ic] );
    }
  }

  if(isAtLeastOneCellUnbalanced == true) { gpio_setFanSpeed('H'); }
  else                                   { gpio_setFanSpeed('0'); }

  
}

//////////////////////////////////////////////////////////////////////////////////

//JTS2doNow: Think this through
void gridCharger_chargePack(void)
{
  lcd_refresh();
  LTC68042cell_nextVoltages(); 
  debugUSB_printLatest_data_gridCharger();

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
    gpio_setFanSpeed('0'); //set inside balanceCells()
    gpio_setGridCharger_powerLevel('0');
  }

  //grid charger plugged in and all cells less than full
  else if( LTC68042result_hiCellVoltage_get() <= (GRID_CHARGER_CELL_VMAX - VCELL_HYSTERESIS) )
  {
    gpio_turnGridCharger_on();
    gpio_setFanSpeed('M');
    gpio_setGridCharger_powerLevel('H');
  }

  //grid charger plugged in and all cells almost full
  else if( (LTC68042result_hiCellVoltage_get() <= GRID_CHARGER_CELL_VMAX)
         && LTC68042result_loCellVoltage_get() >= MIN_DISCHARGE_VOLTAGE_COUNTS )
  {
    //gpio_setFanSpeed('0'); //set inside balanceCells()
    gridCharger_balanceCells();
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