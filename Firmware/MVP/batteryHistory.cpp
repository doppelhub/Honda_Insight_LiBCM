//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//2D array stores total hours battery has spent at each temperature and SoC combination
//X axis: SoC
//Y axis: Temp
//Z axis: uint16 value is hours spent at this Temp & SoC

#include "libcm.h" //JTS2doNow: won't compile when libcm.h included

////////////////////////////////////////////////////////////////////////////////////

uint8_t calcArrayIndex_temperature(void)
{
    int8_t latestTemp = temperature_battery_getLatest();
    if(latestTemp < LO_TEMP_BIN_TOP_DEGC) { latestTemp = LO_TEMP_BIN_TOP_DEGC; }

    return (((uint8_t)(latestTemp - (LO_TEMP_BIN_TOP_DEGC + TEMP_BIN_WIDTH_DEGC - 1))) >> TEMP_BIN_WIDTH_RIGHTSHIFTS);
}

////////////////////////////////////////////////////////////////////////////////////

uint8_t calcArrayIndex_SoC(void)
{
    return ((SoC_getBatteryStateNow_percent() - (LO_SoC_BIN_TOP_PERCENT + SoC_BIN_WIDTH_PERCENT - 1)) >> SoC_BIN_WIDTH_RIGHTSHIFTS);
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
    Serial.print(F("\nTotal hours at each battery temperature and SoC combination"));
    
    int8_t binRange_temperature = LO_TEMP_BIN_TOP_DEGC;

    for(uint8_t temperatureBin=0; temperatureBin<TOTAL_TEMP_BINS; temperatureBin++)
    {
        Serial.print(F("\nt<"));
        Serial.print(binRange_temperature);
        Serial.print(F("degC:"));
        binRange_temperature += TEMP_BIN_WIDTH_DEGC;
    
        for(uint8_t stateOfChargeBin=0; stateOfChargeBin<TOTAL_SoC_BINS; stateOfChargeBin++)
        {
            uint16_t valueToPrint = eeprom_batteryHistory_getValue(temperatureBin, stateOfChargeBin);
            Serial.print(valueToPrint);
            Serial.print(',');
        }
    }

    //print X axis values //"SoC<0,4,8,12,16,20,24,28,32,36,40,44,48,51,56,60,64,68,72,76,80,84,88,92,96,100,%"
    Serial.print(F("\nSoC<"));
    for(uint8_t binRange_SoC=LO_SoC_BIN_TOP_PERCENT; binRange_SoC<=HI_SoC_BIN_TOP_PERCENT; binRange_SoC+=SoC_BIN_WIDTH_PERCENT)
    {
        Serial.print(binRange_SoC);
        Serial.print(',');
    }
    Serial.print('%');
}
