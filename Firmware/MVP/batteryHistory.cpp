//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//2D array stores total hours battery has spent at each temperature and SoC combination
//X axis: SoC
//Y axis: Temp
//Z axis: uint16 value is hours spent at this Temp & SoC

#include "libcm.h"

////////////////////////////////////////////////////////////////////////////////////

uint8_t calcArrayIndex_temperature(void)
{
    int8_t latestTemp = temperature_battery_getLatest();

    if(latestTemp < LO_TEMP_BIN_TOP_DEGC) { latestTemp = LO_TEMP_BIN_TOP_DEGC; }

    int8_t helper = latestTemp - LO_TEMP_BIN_TOP_DEGC + (TEMP_BIN_WIDTH_DEGC - 1);

    if(helper < 0) { helper = 0; }

    uint8_t index = ((uint8_t)helper) >> TEMP_BIN_WIDTH_RIGHTSHIFTS;

    if(index > (TOTAL_TEMP_BINS - 1)) { index = TOTAL_TEMP_BINS - 1; }
    
    return index;
}

////////////////////////////////////////////////////////////////////////////////////

uint8_t calcArrayIndex_SoC(void)
{
    int8_t latestSoC = (int8_t)SoC_getBatteryStateNow_percent();

    int8_t helper = latestSoC - LO_SoC_BIN_TOP_PERCENT + (SoC_BIN_WIDTH_PERCENT - 1);

    if(helper < 0) { helper = 0; }

    uint8_t index = ((uint8_t)helper) >> SoC_BIN_WIDTH_RIGHTSHIFTS;

    if(index > (TOTAL_SoC_BINS - 1)) { index = TOTAL_SoC_BINS - 1; }

    return index;
}

////////////////////////////////////////////////////////////////////////////////////

void batteryHistory_handler(void)
{
    static uint32_t timestamp_lastUpdate_ms = 0;

    if((millis() - timestamp_lastUpdate_ms) > BATTERY_HISTORY_UPDATE_PERIOD_ms)
    { 
        timestamp_lastUpdate_ms = millis();

        eeprom_batteryHistory_incrementValue(calcArrayIndex_temperature(), calcArrayIndex_SoC());
    }
}

////////////////////////////////////////////////////////////////////////////////////

void batteryHistory_printAll(void)
{

    Serial.print(F("\nBattery Temperature and SoC History"
    "\n -Columns: Battery SoC (%)"
    "\n -Rows: Battery Temperature (C)"
    "\n -Value: Time spent at this SoC+Temp (hours)"
    "\nRecommendation: Paste this comma-separated output into a spreadsheet with color-coded conditional formatting"
    ));
    
    //print X axis values //"SoC<0,4,8,12,16,20,24,28,32,36,40,44,48,51,56,60,64,68,72,76,80,84,88,92,96,100,%"
    Serial.print(F("\nSoC<=,,,"));
    for(uint8_t binRange_SoC=LO_SoC_BIN_TOP_PERCENT; binRange_SoC<=HI_SoC_BIN_TOP_PERCENT; binRange_SoC+=SoC_BIN_WIDTH_PERCENT)
    {
        Serial.print(binRange_SoC);
        Serial.print(',');
    }
    Serial.print('%');

    Serial.print('\n');
    for(uint8_t ii=0; ii<63; ii++) { Serial.print('-'); } // "---------------------------------------------------------"

    int8_t binRange_temperature = LO_TEMP_BIN_TOP_DEGC;

    for(uint8_t temperatureBin=0; temperatureBin<TOTAL_TEMP_BINS; temperatureBin++)
    {
        Serial.print(F("\nt<=,"));
        Serial.print(binRange_temperature);
        Serial.print(F(",degC:"));
        binRange_temperature += TEMP_BIN_WIDTH_DEGC;
    
        for(uint8_t stateOfChargeBin=0; stateOfChargeBin<TOTAL_SoC_BINS; stateOfChargeBin++)
        {
            Serial.print(',');
            uint16_t valueToPrint = eeprom_batteryHistory_getValue(temperatureBin, stateOfChargeBin);
            Serial.print(valueToPrint);      
        }
    }


}

