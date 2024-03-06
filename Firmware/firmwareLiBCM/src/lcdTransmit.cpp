//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles all communication with 4x20 lcd display

//The 4x20 LCD I2C bus is super slow... therefore, only one screen variable is updated per superloop iteration.
//This is handled by token 'lcd_whichFunctionUpdates', which is used for a round-robin scheduler
//Avoid calling 'lcd2.___' at all costs; it's ALWAYS faster to do math to see if you need to send data to screen.

#include "libcm.h"

#include "lcd_I2C.h" //ensure these funcitons are only called within this file

lcd_I2C_jts lcd2(0x27);

//JTS2doLater: Add indicator to 4x20 when fan is off/low/high

//These variables are reset during key or grid charger state change
uint8_t  cycleFrameNumber = CYCLEFRAME_INIT;
uint16_t timeValue_onScreen        = 0xFFFF;
uint8_t  packVoltageActual_onScreen   =   0;
uint8_t  packVoltageSpoofed_onScreen  =   0;
uint16_t maxEverCellVoltage_onScreen  =   0;
uint16_t minEverCellVoltage_onScreen  =   0;
uint8_t  SoC_onScreen                 =   0;
uint16_t hiCellVoltage_onScreen       =   0;
uint16_t loCellVoltage_onScreen       =   0;
int8_t   battTemp_onScreen            =  99;
uint8_t  cellBalanceFlag_onScreen     = 'b';
uint8_t  isoSPI_errorFlag_onScreen    = 'e';
uint8_t  fanSpeed_onScreen            = 'f';
uint8_t  gridChargerState_onScreen    = 'g';
uint8_t  heaterState_onScreen         = 'h';
bool     isBacklightFlashingRequested =  NO;

bool     areAllStaticValuesDisplayed  =  NO;

/////////////////////////////////////////////////////////////////////////////////////////

void lcdTransmit_begin(void) { lcd2.begin(20,4); }
void lcdTransmit_end(void)   { Wire.end();       }

/////////////////////////////////////////////////////////////////////////////////////////

//some screen elements cycle through various values over time
//this function determines which cycle frame to display
//stores 'CYCLEFRAME_A', 'CYCLEFRAME_B', etc in cycleFrameNumber
void whichCycleFrameToDisplay(void)
{
    static uint32_t lastTimeFrameChanged_ms = 0;

    uint32_t timeKeyOn_ms = time_sinceLatestKeyOn_ms();

    if (timeKeyOn_ms < 500)
    {
        cycleFrameNumber = CYCLEFRAME_A;
        lastTimeFrameChanged_ms = 0;  
    }

    uint16_t frameDisplayPeriod_ms = 0;
    
    if      (cycleFrameNumber == CYCLEFRAME_A)  { frameDisplayPeriod_ms = CYCLEFRAME_A_PERIOD_ms; }
    else if (cycleFrameNumber == CYCLEFRAME_B)  { frameDisplayPeriod_ms = CYCLEFRAME_B_PERIOD_ms; }

    if(timeKeyOn_ms - lastTimeFrameChanged_ms >= frameDisplayPeriod_ms)
    {
        lastTimeFrameChanged_ms = timeKeyOn_ms;
        if(++cycleFrameNumber > CYCLEFRAME_MAX_VALUE) { cycleFrameNumber = CYCLEFRAME_A; }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

//flash backlight if requested
bool lcd_flashBacklight(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    static bool isBacklightOn = YES;
    static uint32_t lastBacklightStateChange_ms = 0;

    if (isBacklightFlashingRequested == NO)
    {
        if (isBacklightOn == NO)
        {
            lcd2.backlight();
            didscreenUpdateOccur = SCREEN_UPDATED;
        }
    }
    else if ((millis() - lastBacklightStateChange_ms) > (BACKLIGHT_FLASHING_PERIOD_ms>>1))
    {
        if (isBacklightOn == YES) { lcd2.noBacklight(); isBacklightOn = NO;  }
        else                      { lcd2.backlight();   isBacklightOn = YES; }

        lastBacklightStateChange_ms = millis();
        didscreenUpdateOccur = SCREEN_UPDATED;      
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

//alternates between:
    //time since last keyON, and; //"tuuuuu" in seconds
    //time until firmware expires //"FWuuuu" in minutes
bool lcd_printTime_unitless(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    static uint8_t cycleFrameNumber_previous = CYCLEFRAME_INIT;

    if (time_sinceLatestKeyOn_ms() < 500) { cycleFrameNumber_previous = CYCLEFRAME_INIT; }

    if (cycleFrameNumber_previous != cycleFrameNumber)
    {
        lcd2.setCursor(0,3);
        if      (cycleFrameNumber == CYCLEFRAME_A) { lcd2.print(F("FW")); }
        else if (cycleFrameNumber == CYCLEFRAME_B) { lcd2.print(F("t" )); }
        
        cycleFrameNumber_previous = cycleFrameNumber;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }
    else if (cycleFrameNumber == CYCLEFRAME_A)
    {
        uint16_t timeMinutes = 9876; //JTS2doNow: Add function to read actual expiration time in minutes

        if (timeValue_onScreen != timeMinutes)
        {
            lcd2.setCursor(2,3);

            if      (timeMinutes < 10   ) { lcd2.print(F("   ")); } //one   digit  (  0:   9)
            else if (timeMinutes < 100  ) { lcd2.print(F("  ") ); } //two   digits ( 10:  99)
            else if (timeMinutes < 1000 ) { lcd2.print(F(" ")  ); } //three digits (100: 999)

            lcd2.print(String(timeMinutes));

            timeValue_onScreen = timeMinutes;
            didscreenUpdateOccur = SCREEN_UPDATED;
        }
    }
    else if (cycleFrameNumber == CYCLEFRAME_B)
    {
        uint16_t timeSeconds = time_sinceLatestKeyOn_seconds();

        if (timeValue_onScreen != timeSeconds)
        {
            lcd2.setCursor(1,3);

            if      (timeSeconds < 10   ) { lcd2.print(F("    ")); } //one   digit  //   0:   9
            else if (timeSeconds < 100  ) { lcd2.print(F("   ") ); } //two   digits //  10:  99
            else if (timeSeconds < 1000 ) { lcd2.print(F("  ")  ); } //three digits // 100: 999
            else if (timeSeconds < 10000) { lcd2.print(F(" ")   ); } //four  digits //1000:9999

            lcd2.print(String(timeSeconds));

            timeValue_onScreen = timeSeconds;
            didscreenUpdateOccur = SCREEN_UPDATED;
        }
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printSoC(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;
    uint8_t SoC = SoC_getBatteryStateNow_percent();

    if (SoC_onScreen != SoC)
    {
        lcd2.setCursor(10,3); //SoC screen position
        if (SoC < 10) { lcd2.print( '0');         } //add leading '0' to single digit number
        if (SoC > 99) { lcd2.print(F("99"));      } //can't display '100' (only QTY2 digits)
        else          { lcd2.print(SoC); } //print actual value

        SoC_onScreen = SoC;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printStackVoltage_actual(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;
    uint8_t packVoltageActual = LTC68042result_packVoltage_get();

    if (packVoltageActual_onScreen != packVoltageActual)
    {
        lcd2.setCursor(1,2); //actual pack voltage position
        if(packVoltageActual <  10) { lcd2.print('0'); }
        if(packVoltageActual < 100) { lcd2.print('0'); }
        lcd2.print(packVoltageActual);

        packVoltageActual_onScreen = packVoltageActual;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printStackVoltage_spoofed(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;
    uint8_t packVoltageSpoofed = vPackSpoof_getSpoofedPackVoltage();

    if (packVoltageSpoofed_onScreen != packVoltageSpoofed)
    {
        lcd2.setCursor(5,2); //spoofed pack voltage position
        if(packVoltageSpoofed <  10) { lcd2.print('0'); }
        if(packVoltageSpoofed < 100) { lcd2.print('0'); }
        lcd2.print(packVoltageSpoofed);

        packVoltageSpoofed_onScreen = packVoltageSpoofed;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printTempBattery(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    int8_t battTemp = temperature_battery_getLatest(); //get single ADC measurement

    if (battTemp_onScreen != battTemp)
    {
        battTemp_onScreen = battTemp;
        lcd2.setCursor(11,2);
        if ((battTemp >= 0) && (battTemp < 10)) { lcd2.print(' '); } //leading space on " 0" to " 9" degC
        if ( battTemp < -9 )                    { lcd2.print(-9); } //-10 degC and lower are displayed as "-9"
        else                                    { lcd2.print(battTemp); }

        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_cellBalanceStatus(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    uint8_t cellBalanceFlag = (cellBalance_areCellsBalanced() == YES) ? '_' : 'B';

    if (cellBalanceFlag_onScreen != cellBalanceFlag)
    {
        lcd2.setCursor(15,2);
        if (cellBalanceFlag == 'B') { lcd2.print('B'); }
        else                        { lcd2.print('_'); }

        cellBalanceFlag_onScreen = cellBalanceFlag;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printLTC6804Errors(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    uint8_t isoSPI_errorFlag = (LTC68042result_errorCount_get() == 0) ? '_' : 'E';

    if (isoSPI_errorFlag_onScreen != isoSPI_errorFlag)
    {
        lcd2.setCursor(16,2);
        if (isoSPI_errorFlag == 'E') { lcd2.print('E'); }
        else                         { lcd2.print('_'); }

        isoSPI_errorFlag_onScreen = isoSPI_errorFlag;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printFanStatus(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    uint8_t fanSpeed = fan_getSpeed_now();

    if (fanSpeed_onScreen != fanSpeed)
    {
        lcd2.setCursor(17,2);
        if     (fanSpeed == FAN_OFF)  { lcd2.print('_'); }
        else if(fanSpeed == FAN_LOW)  { lcd2.print('f'); }
        else if(fanSpeed == FAN_MED)  { lcd2.print('f'); }
        else if(fanSpeed == FAN_HIGH) { lcd2.print('F'); }

        fanSpeed_onScreen = fanSpeed;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printCellVoltage_hi(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    if (hiCellVoltage_onScreen != LTC68042result_hiCellVoltage_get())
    {
        hiCellVoltage_onScreen = LTC68042result_hiCellVoltage_get();
        lcd2.setCursor(1,0); //high cell voltage position
        lcd2.print( (hiCellVoltage_onScreen * 0.0001), 3 );

        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    if (LTC68042result_hiCellVoltage_get() > CELL_VMAX_REGEN) { isBacklightFlashingRequested = YES; }
    else                                                      { isBacklightFlashingRequested =  NO; }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printCellVoltage_lo(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    if (loCellVoltage_onScreen != LTC68042result_loCellVoltage_get())
    {
        loCellVoltage_onScreen = LTC68042result_loCellVoltage_get();
        lcd2.setCursor(1,1); //low screen position
        lcd2.print( (loCellVoltage_onScreen * 0.0001), 3 );

        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    static bool isBacklightOn = true;

    if (LTC68042result_loCellVoltage_get() < CELL_VMIN_ASSIST) { isBacklightFlashingRequested = YES; }
    else                                                       { isBacklightFlashingRequested =  NO; }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printCellVoltage_delta(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    static uint16_t deltaVoltage_onScreen = 0;

    uint16_t deltaVoltage_LTC6804 = LTC68042result_hiCellVoltage_get() - LTC68042result_loCellVoltage_get();

    if (deltaVoltage_onScreen != deltaVoltage_LTC6804)
    {
        deltaVoltage_onScreen = deltaVoltage_LTC6804;
        lcd2.setCursor(15,0); //delta cell voltage position
        lcd2.print( (deltaVoltage_onScreen * 0.0001), 3 );

        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printCurrent(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    static int16_t deciAmps_onScreen = 0;

    if (deciAmps_onScreen != adc_getLatestBatteryCurrent_deciAmps())
    {
        int16_t deciAmps = adc_getLatestBatteryCurrent_deciAmps();
        
        lcd2.setCursor(15,1);

        #ifdef DISPLAY_NEGATIVE_SIGN_DURING_ASSIST
            if      (deciAmps > 0) { lcd2.print('-'); } //When discharging battery (i.e. assist), we display '-' symbol, even though internally it's '+' 
            else if (deciAmps < 0) { lcd2.print('+'); } //When    charging battery (i.e. regen ), we display '+' symbol, even though internally it's '-' 
            else                   { lcd2.print(' '); }
        #elif defined DISPLAY_POSITIVE_SIGN_DURING_ASSIST
            if      (deciAmps > 0) { lcd2.print('+'); } //When discharging battery (i.e. assist), we display '+' symbol
            else if (deciAmps < 0) { lcd2.print('-'); } //When    charging battery (i.e. regen ), we display '+' symbol 
            else                   { lcd2.print(' '); }
        #endif

        int16_t abs_deciAmps = abs(deciAmps);

        if (abs_deciAmps <  100) { lcd2.print(' '); } //add one leading space (e.g. "+ 9.9")
        if (abs_deciAmps < 1000) { lcd2.print(abs_deciAmps * 0.1, 1); }
        else                     { lcd2.print(abs_deciAmps * 0.1, 0); lcd2.print(' '); }
        
        deciAmps_onScreen = deciAmps;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printMaxEverVoltage()
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    if (maxEverCellVoltage_onScreen != LTC68042result_maxEverCellVoltage_get())
    {
        maxEverCellVoltage_onScreen = LTC68042result_maxEverCellVoltage_get();
        lcd2.setCursor(7,0); //maxEver screen position
        lcd2.print( (maxEverCellVoltage_onScreen * 0.0001) , 3);

        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printMinEverVoltage()
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    if (minEverCellVoltage_onScreen != LTC68042result_minEverCellVoltage_get())
    {
        minEverCellVoltage_onScreen = LTC68042result_minEverCellVoltage_get();
        lcd2.setCursor(7,1); //minEver screen position
        lcd2.print( (minEverCellVoltage_onScreen * 0.0001) , 3);

        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printPower(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    static int16_t deci_kW_onScreen = 0; //100 watts per count (i.e. one tenth of a kW per count)

    int16_t deci_kW = (LTC68042result_packVoltage_get() * (int32_t)adc_getLatestBatteryCurrent_deciAmps()) * 0.001;

    if (deci_kW != deci_kW_onScreen)
    {
        lcd2.setCursor(15,3);

        #ifdef DISPLAY_NEGATIVE_SIGN_DURING_ASSIST
            if      (deci_kW > 0) { lcd2.print('-'); } //When discharging battery (i.e. assist), we display '-' symbol, even though internally it's '+' 
            else if (deci_kW < 0) { lcd2.print('+'); } //When    charging battery (i.e. regen ), we display '+' symbol, even though internally it's '-' 
            else                  { lcd2.print(' '); }
        #elif defined DISPLAY_POSITIVE_SIGN_DURING_ASSIST
            if      (deci_kW > 0) { lcd2.print('+'); } //When discharging battery (i.e. assist), we display '+' symbol
            else if (deci_kW < 0) { lcd2.print('-'); } //When    charging battery (i.e. regen ), we display '-' symbol 
            else                  { lcd2.print(' '); }
        #endif

        int16_t abs_deci_kW = abs(deci_kW);

        if (abs_deci_kW <  100) { lcd2.print(' '); } //add one leading space (e.g. "+ 9.9")
        lcd2.print(abs_deci_kW * 0.1, 1); //print kW

        deci_kW_onScreen = deci_kW;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }


    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Add 'g' when charging below max power
bool lcd_printGridChargerStatus(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    static uint8_t gridChargerState = 0;
    if (gpio_isGridChargerChargingNow() == YES)  { gridChargerState = 'G'; }
    else                                         { gridChargerState = '_'; }

    if (gridChargerState_onScreen != gridChargerState)
    {
        lcd2.setCursor(18,2); //grid charger status position

        if (gridChargerState == 'G') { lcd2.print('G'); }
        else                         { lcd2.print('_'); }

        gridChargerState_onScreen = gridChargerState;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_printHeaterStatus(void)
{
    bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

    static uint8_t heaterState = 0;
    if (gpio_isHeaterOnNow() == YES) { heaterState = 'H'; }
    else                             { heaterState = '_'; }

    if (heaterState_onScreen != heaterState)
    {
        lcd2.setCursor(19,2); //grid charger status position

        if (heaterState == 'H') { lcd2.print('H'); }
        else                    { lcd2.print('_'); }

        heaterState_onScreen = heaterState;
        didscreenUpdateOccur = SCREEN_UPDATED;
    }

    return didscreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

void lcd_resetVariablesToDefault(void)
{
    cycleFrameNumber = CYCLEFRAME_INIT;
    timeValue_onScreen        = 0xFFFF;
    packVoltageActual_onScreen   =   0;
    packVoltageSpoofed_onScreen  =   0;
    SoC_onScreen                 =   0;
    maxEverCellVoltage_onScreen  =   0;
    minEverCellVoltage_onScreen  =   0;
    hiCellVoltage_onScreen       =   0;
    loCellVoltage_onScreen       =   0;
    battTemp_onScreen            =  99;
    cellBalanceFlag_onScreen     = 'b';
    isoSPI_errorFlag_onScreen    = 'e';
    fanSpeed_onScreen            = 'f';
    gridChargerState_onScreen    = 'g';
    heaterState_onScreen         = 'h';
    isBacklightFlashingRequested =  NO;
}

/////////////////////////////////////////////////////////////////////////////////////////

void lcdTransmit_displayOn(void)
{
    lcd2.backlight();
    lcd2.display();
    areAllStaticValuesDisplayed = NO;
}

/////////////////////////////////////////////////////////////////////////////////////////

void lcdTransmit_displayOff(void)
{
    lcd2.noBacklight();
    lcd2.noDisplay();
}

/////////////////////////////////////////////////////////////////////////////////////////

void lcdTransmit_Warning(uint8_t warningToDisplay)
{
    static uint8_t whichRowToPrint = 0;

    lcd2.setCursor(0,whichRowToPrint);

    if (warningToDisplay == LCD_WARN_KEYON_GRID)
    {
        //                                            ********************
        if      (whichRowToPrint == 0) { lcd2.print(F("ALERT: Grid Charger "));}
        else if (whichRowToPrint == 1) { lcd2.print(F("       Plugged In!! "));}
        else if (whichRowToPrint == 2) { lcd2.print(F("LiBCM sent P1648 to "));}
        else if (whichRowToPrint == 3) { lcd2.print(F("prevent IMA start.  "));}
    }

    else if (warningToDisplay == LCD_WARN_FW_EXPIRED)
    {
        if      (whichRowToPrint == 0) { lcd2.print(F("ALERT: New firmware ")); }
        else if (whichRowToPrint == 1) { lcd2.print(F("required during beta")); }
        else if (whichRowToPrint == 2) { lcd2.print(F(" --LiBCM disabled-- ")); }
        else if (whichRowToPrint == 3) { lcd2.print(F("  www.linsight.org  ")); }
    }

    else if (warningToDisplay == LCD_WARN_COVER_GONE)
    {
        if      (whichRowToPrint == 0) { lcd2.print(F("ALERT: Safety cover ")); }
        else if (whichRowToPrint == 1) { lcd2.print(F("       not installed")); }
        else if (whichRowToPrint == 2) { lcd2.print(F(" --LiBCM disabled-- ")); }
        else if (whichRowToPrint == 3) { lcd2.print(F("  www.linsight.org  ")); }
    }

    else if (warningToDisplay == LCD_WARN_CELL_COUNT)
    {
        lcd2.setCursor(0,0); lcd2.print(F("ALERT: Measured cell"));
        lcd2.setCursor(0,1); lcd2.print(F("       count doesn't"));
        lcd2.setCursor(0,2); lcd2.print(F("       match setting"));
        lcd2.setCursor(0,3); lcd2.print(F("       in config.h )"));
    }   

    if (++whichRowToPrint > 3) { whichRowToPrint = 0; }

    areAllStaticValuesDisplayed = NO; //reprint static values once warning message goes away
}

/////////////////////////////////////////////////////////////////////////////////////////

void lcdTransmit_splashscreenKeyOff(void)
{
    lcd2.clear();
    lcd2.setCursor(0,0);
    lcd2.print(F("LiBCM v")); lcd2.print(String(FW_VERSION));
    lcd2.setCursor(0,1);
    lcd2.print(F("FW Hours Left: "));
    lcd2.print(String(REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS - eeprom_uptimeStoredInEEPROM_hours_get() ));
}

/////////////////////////////////////////////////////////////////////////////////////////

bool lcd_updateValue(uint8_t stateToUpdate)
{
    bool didScreenUpdateOccur = SCREEN_DIDNT_UPDATE;
    switch (stateToUpdate)
    {
        case LCDVALUE_CALC_CYCLEFRAME:                        whichCycleFrameToDisplay();      break;             
        case LCDVALUE_SECONDS        : didScreenUpdateOccur = lcd_printTime_unitless();        break;
        case LCDVALUE_VPACK_ACTUAL   : didScreenUpdateOccur = lcd_printStackVoltage_actual();  break;
        case LCDVALUE_VPACK_SPOOFED  : didScreenUpdateOccur = lcd_printStackVoltage_spoofed(); break;
        case LCDVALUE_LTC6804_ERRORS : didScreenUpdateOccur = lcd_printLTC6804Errors();        break;
        case LCDVALUE_CELL_HI        : didScreenUpdateOccur = lcd_printCellVoltage_hi();       break;
        case LCDVALUE_CELL_LO        : didScreenUpdateOccur = lcd_printCellVoltage_lo();       break;
        case LCDVALUE_CELL_DELTA     : didScreenUpdateOccur = lcd_printCellVoltage_delta();    break;
        case LCDVALUE_POWER          : didScreenUpdateOccur = lcd_printPower();                break;
        case LCDVALUE_CELL_MAXEVER   : didScreenUpdateOccur = lcd_printMaxEverVoltage();       break;
        case LCDVALUE_CELL_MINEVER   : didScreenUpdateOccur = lcd_printMinEverVoltage();       break;
        case LCDVALUE_SoC            : didScreenUpdateOccur = lcd_printSoC();                  break;
        case LCDVALUE_CURRENT        : didScreenUpdateOccur = lcd_printCurrent();              break;
        case LCDVALUE_TEMP_BATTERY   : didScreenUpdateOccur = lcd_printTempBattery();          break;
        case LCDVALUE_FAN_STATUS     : didScreenUpdateOccur = lcd_printFanStatus();            break;
        case LCDVALUE_GRID_STATUS    : didScreenUpdateOccur = lcd_printGridChargerStatus();    break;
        case LCDVALUE_HEATER_STATUS  : didScreenUpdateOccur = lcd_printHeaterStatus();         break;
        case LCDVALUE_BALANCE_STATUS : didScreenUpdateOccur = lcd_cellBalanceStatus();         break;
        case LCDVALUE_FLASH_BACKLIGHT: didScreenUpdateOccur = lcd_flashBacklight();            break;
        default                      : didScreenUpdateOccur = SCREEN_UPDATED;                  break;
    }

    return didScreenUpdateOccur;
}

/////////////////////////////////////////////////////////////////////////////////////////

void updateNextVariable(void)
{
    static uint8_t lcdVariableToUpdate = LCDVALUE_NO_UPDATE; //init round-robin

    uint8_t updateAttempts = 0;
    
    do
    {
        if ((++lcdVariableToUpdate) > LCDVALUE_MAX_VALUE) { lcdVariableToUpdate = 1; } //reset to first element
        updateAttempts++;
    } while ((lcd_updateValue(lcdVariableToUpdate) == SCREEN_DIDNT_UPDATE) && (updateAttempts < LCD_UPDATE_ATTEMPTS_PER_LOOP));

}

/////////////////////////////////////////////////////////////////////////////////////////

#ifdef RUN_BRINGUP_TESTER_MOTHERBOARD
    void lcdTransmit_testText(void)
    {
        lcd2.setCursor(0,0);
        //                                            1111111111
        //                                  01234567890123456789
        //4x20 screen text display format:  ********************
        lcd2.setCursor(0,0);  lcd2.print(F("00000000001111111111"));
        lcd2.setCursor(0,1);  lcd2.print(F("01234567890123456789"));
        lcd2.setCursor(0,2);  lcd2.print(F("ABCDEFGHIJKLMNOPQRST"));
        lcd2.setCursor(0,3);  lcd2.print(F("UVWXYZ HELLO WORLD!!"));
    }
#endif

/////////////////////////////////////////////////////////////////////////////////////////

//LCD character formatting:
        //      LCD Column#:
        //      00000000001111111111
        //      01234567890123456789    variable definitions:
        //      |****|****|****|****    cellMaxNow   cellMaxKey   cellDeltaV
        //Row0 "Hx.xxx(y.yyy) Dd.ddd" | x.xxx=(1,0)  y.yyy=(7,0)  d.ddd=(15,0)
        //
        //      |****|****|****|****    cellMinNow   cellMinKey   packAmps
        //Row1 "La.aaa(j.jjj) A-cc.c" | a.aaa=(1,1)  j.jjj=(7,1)  -cc.c=(15,1)
        //
        //      |****|****|****|****    VpackActual  VpackSpoof   battTemp   balancing  errorLTC  fan       charger   heater
        //Row2 "Vrrr(mmm) TkkC befgh" | rrr=(1,2)    mmm=(5,2)    kk=(11,2)  b=(15,2)   e=(16,2)  f=(17,2)  g=(18,2)  h=(19,2)
        //
        //      |****|****|****|****    uptimeKeyOn*  packSoC      power_kW     
        //Row3 "tuuuuu SoCss kW-pp.p" | tuuuuu=(0,3)  ss=(10,3)    -pp.p=(15,3)
        //                              t_firmware*
        //                              FWuuuu=(0,3)  //*This segment cycles between these two parameters
        //
//this function writes the above static data, one element per call:
bool updateNextStatic(void)
{
    static uint8_t lcdStaticElementToUpdate = LCDSTATIC_SET_DEFAULTS;

    switch (lcdStaticElementToUpdate)
    {
        case LCDSTATIC_SET_DEFAULTS:  lcd_resetVariablesToDefault();                  break;
        case LCDSTATIC_SECONDS:       lcd2.setCursor( 0,3); lcd2.print(F("tuuuuu") ); break;
        case LCDSTATIC_VPACK_ACTUAL:  lcd2.setCursor( 0,2); lcd2.print(F("Vrrr")   ); break;
        case LCDSTATIC_VPACK_SPOOFED: lcd2.setCursor( 4,2); lcd2.print(F("(mmm)")  ); break;
        case LCDSTATIC_CHAR_FLAGS:    lcd2.setCursor(15,2); lcd2.print(F("befgh")  ); break;
        case LCDSTATIC_CELL_HI:       lcd2.setCursor( 0,0); lcd2.print(F("Hx.xxx") ); break;
        case LCDSTATIC_CELL_LO:       lcd2.setCursor( 0,1); lcd2.print(F("La.aaa") ); break;
        case LCDSTATIC_CELL_DELTA:    lcd2.setCursor(13,0); lcd2.print(F(" Dd.ddd")); break;
        case LCDSTATIC_POWER:         lcd2.setCursor(13,3); lcd2.print(F("kW-pp.p")); break;
        case LCDSTATIC_CELL_MAXEVER:  lcd2.setCursor( 6,0); lcd2.print(F("(y.yyy)")); break;
        case LCDSTATIC_CELL_MINEVER:  lcd2.setCursor( 6,1); lcd2.print(F("(j.jjj)")); break;
        case LCDSTATIC_SoC:           lcd2.setCursor( 6,3); lcd2.print(F(" SoCss ")); break;
        case LCDSTATIC_CURRENT:       lcd2.setCursor(13,1); lcd2.print(F(" A-cc.c")); break;
        case LCDSTATIC_TEMP_BATTERY:  lcd2.setCursor( 9,2); lcd2.print(F(" TkkC ") ); break;
        default:                                                                      break;
    }

    bool doneDisplayingStaticValues = NO;
    
    if (++lcdStaticElementToUpdate > LCDSTATIC_MAX_VALUE)
    {
        doneDisplayingStaticValues = YES;
        lcdStaticElementToUpdate = LCDSTATIC_SET_DEFAULTS;
    }

    return doneDisplayingStaticValues;
}

/////////////////////////////////////////////////////////////////////////////////////////

//primary interface
//update one screen element (if any have changed)
void lcdTransmit_printNextElement(void)
{
    if (areAllStaticValuesDisplayed == YES) { updateNextVariable(); }
    else { areAllStaticValuesDisplayed = updateNextStatic(); } //static values only sent once each time the display turns on
}

/////////////////////////////////////////////////////////////////////////////////////////
