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
  SoC_updateUsingOpenCircuitVoltage();
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

//JTS2doNow: Separate grid charging from balancing... balancing should occur independently from grid charging
//JTS2doNow: Think this through to make sure it won't over-discharge cells
//only call when:
//grid charger is plugged in
//minimum cell voltage above safe minimum 
//no cell is severely overcharged
void gridCharger_balanceCells(void)
{   
  uint16_t cellsToDischarge[TOTAL_IC] = {0}; //each uint16's QTY12 LSBs correspond to each cell 

  bool isAtLeastOneCellUnbalanced = false;

  if(LTC68042result_loCellVoltage_get() > 32000) //32000 = 3.2000 volts //prevent lowest cell overdischarge (in a severely unbalanced pack)
  { //all cells above minimum balancing voltage
    for(uint8_t ic = 0; ic < TOTAL_IC; ic++)
    {
      for (uint8_t cell = 0; cell < CELLS_PER_IC; cell++)
      {
        if(LTC68042result_specificCellVoltage_get(ic, cell) > (LTC68042result_loCellVoltage_get() + BALANCE_TO_WITHIN_COUNTS) )
        { //this particular cell's voltage is higher than the lowest cell's voltage
          cellsToDischarge[ic] |= (1 << cell);
          isAtLeastOneCellUnbalanced = true;
        }
      }

    debugUSB_setCellBalanceStatus(ic, cellsToDischarge[ic]);
    LTC68042configure_cellBalancing_setCells( (ic + FIRST_IC_ADDR), cellsToDischarge[ic] );
    }
  }
  else
  {
    Serial.println(F("Cells too unbalanced to safely grid charge."));
  }

  if(isAtLeastOneCellUnbalanced == true) { gpio_setFanSpeed('H'); } //full blast to cool discharge resistors
  else                                   { gpio_setFanSpeed('0'); } //pack is balanced
}

//////////////////////////////////////////////////////////////////////////////////

//JTS2doNow: Separate balancing routine entirely from charging routine
void gridCharger_chargePack(void)
{
  static uint8_t cellState = CELLSTATE_UNINITIALIZED;
  static uint8_t cellStatePrevious = CELLSTATE_UNINITIALIZED;
  cellStatePrevious = cellState;

  lcd_refresh();
  LTC68042cell_nextVoltages();
  SoC_setBatteryStateNow_percent( SoC_estimateFromRestingCellVoltage_percent() ); //LiBCM's low grid charge current hardly changes cell voltage
  debugUSB_printLatest_data_gridCharger();

  //JTS2doNow: Prevent grid charging if any cell is too discharged (CELL_VMIN_GRIDCHARGER)

  //at least one cell is severely overcharged
  if( LTC68042result_hiCellVoltage_get() > (CELL_VMAX_GRIDCHARGER + VCELL_HYSTERESIS) )
  {
    cellState = CELLSTATE_OVERCHARGED;
    gpio_turnGridCharger_off();
    gpio_setFanSpeed('H'); //cool pack and discharge
    gpio_setGridCharger_powerLevel('0');
    //JTS2doLater: display Warning on LCD
    gpio_turnBuzzer_on_highFreq();
  }

  //at least one cell is full
  else if( (LTC68042result_hiCellVoltage_get() > CELL_VMAX_GRIDCHARGER) )
  {
    cellState = CELLSTATE_ONECELLFULL;
    gpio_turnGridCharger_off();
    gpio_setFanSpeed('0'); //set inside balanceCells()
    gpio_setGridCharger_powerLevel('0');
    gpio_turnBuzzer_off();
  }

  //grid charger plugged in and all cells less than full
  else if( LTC68042result_hiCellVoltage_get() <= (CELL_VMAX_GRIDCHARGER - VCELL_HYSTERESIS) )
  {
    cellState = CELLSTATE_NOCELLSFULL;
    gpio_turnGridCharger_on();
    gpio_setFanSpeed('M');
    gpio_setGridCharger_powerLevel('H');
    gpio_turnBuzzer_off();
  }

  //grid charger plugged in and all cells almost full
  else if( (LTC68042result_hiCellVoltage_get() <= CELL_VMAX_GRIDCHARGER) )
  {
    cellState = CELLSTATE_BALANCING;
    //gpio_setFanSpeed('0'); //set inside balanceCells()
    gridCharger_balanceCells();
    gpio_turnBuzzer_off();
  }

  if (cellStatePrevious != cellState)
  {
    //state has changed
    Serial.print(F(" grid:"));
    switch (cellState)
    {
      case CELLSTATE_UNINITIALIZED: Serial.print(F("init")       ); break;
      case CELLSTATE_BALANCING:     Serial.print(F("balancing")  ); break;
      case CELLSTATE_OVERCHARGED:   Serial.print(F("overcharged")); break;
      case CELLSTATE_ONECELLFULL:   Serial.print(F("oneCellFull")); break;
      case CELLSTATE_NOCELLSFULL:   Serial.print(F("charging")   ); break;
    }
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