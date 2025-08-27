//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles all ADC calls
//JTS2doLater: Remove Arduino functions
//JTS2doLater: use interrupts, so ADC calls don't block execution (each conversion takes ~112 us)

#include "libcm.h"

int8_t calibratedCurrentSensorOffset = 0; //calibrated when current is known to be zero (e.g. each time after key turns off)

int16_t battCurrent_deciAmps = 0;
int16_t spoofedCurrent_deciAmps = 0;

/////////////////////////////////////////////////////////////////////////////////////////

int16_t  adc_getLatestBatteryCurrent_amps    (void) { return ((battCurrent_deciAmps * 13) >> 7); } //multiply by 0.102 (ideally 0.100) //JTS2doLater: Don't use this function
int16_t  adc_getLatestBatteryCurrent_deciAmps(void) { return   battCurrent_deciAmps;             }

/////////////////////////////////////////////////////////////////////////////////////////

int16_t adc_getLatestSpoofedCurrent_amps    (void) { return ((spoofedCurrent_deciAmps * 13) >> 7); } //multiply by 0.102 (ideally 0.100) //JTS2doLater: Don't use this function
int16_t adc_getLatestSpoofedCurrent_deciAmps(void) { return spoofedCurrent_deciAmps;      }

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t adc_packVoltage_VpinIn(void) //returns pack voltage (in volts)
{
    //Derivation:
          //packVoltage_VpinIn  = VPIN_0to5v                               * 52 
          //                      VPIN_0to5v = adc_VPIN_raw() * 5 ) / 1024      //adc returns 10b result
          //packVoltage_VpinIn  =              adc_VPIN_raw() * 5 ) / 1024 * 52
          //packVoltage_VpinIn  =              adc_VPIN_raw() * 260 / 1024
          //packVoltage_VpinIn ~=              adc_VPIN_raw() /  4        + 3 //approximately equal between 120VDC and 250VDC
          //packVoltage_VpinIn ~=              adc_VPIN_raw() >> 2        + 3
    uint8_t packVoltage_VpinIn  = (uint8_t)(analogRead(PIN_VPIN_IN) >> 2) + 3;

    return packVoltage_VpinIn; //pack voltage in volts
} 

/////////////////////////////////////////////////////////////////////////////////////////

//sample battery current and update in deciAmps
//Returned current value is not accurate enough for coulomb counting (use "SoC_integrateCharge_adcCounts" for that)
void sampleAndProcessBatteryCurrent(void)
{
    //Regardless of I2V resistance value, 0A (no regen or assist) is ~332 counts (~1.62 volts when VREF is 5V)
    //Actual VCC voltage doesn't matter, since ADC reference is also VCC (the two values are ratiometric)
    //ADC counts increase as assist current increases //1023 counts is maximum assist //0 counts is maximum regen

    uint16_t battCurrent_countsRAW = analogRead(PIN_BATTCURRENT);
    uint16_t battCurrent_counts;

    //bound adc result
    if      ( (calibratedCurrentSensorOffset > 0)                            &&
            (battCurrent_countsRAW > (1023 - calibratedCurrentSensorOffset))  ) { battCurrent_counts = 1023; } //prevent values greater than 2^10-1
    else if ( (calibratedCurrentSensorOffset < 0)                            &&
            (battCurrent_countsRAW < (   0 - calibratedCurrentSensorOffset))  ) { battCurrent_counts = 0; } //prevent wraparound
    else /* result will be between 0 & 1023 */                                  { battCurrent_counts = battCurrent_countsRAW + calibratedCurrentSensorOffset; }

    SoC_integrateCharge_adcCounts(battCurrent_counts);

    //convert current sensor's 10b adc result to deciAmps
    //see "RevC/V&V/OEM Current Sensor.ods" for measured results
    //see SPICE simulation for complete derivation
    constexpr uint8_t SCALAR_HIGH_ASSIST = 35; //prevents uint16 overflow during heavy assist (max assist is 1023 adc counts) 
    constexpr uint8_t SCALAR_OTHERWISE   = 69; //more accurate during regen and light assist, but overflows at very high assist current
    constexpr uint16_t MAX_COUNTS_NO_OVERFLOW = 65535/SCALAR_OTHERWISE; //uint16_t transition point //adc results above this value will overflow 

    if (battCurrent_counts < MAX_COUNTS_NO_OVERFLOW) { battCurrent_deciAmps = (int16_t)((battCurrent_counts * SCALAR_OTHERWISE  ) >> 5) - 715; } //200 mA uncertainty
    else                                             { battCurrent_deciAmps = (int16_t)((battCurrent_counts * SCALAR_HIGH_ASSIST) >> 4) - 741; } //300 mA uncertainty
}

/////////////////////////////////////////////////////////////////////////////////////////

void adc_updateBatteryCurrent(void)
{
    sampleAndProcessBatteryCurrent();

    #if   defined (SET_CURRENT_HACK_60)
        spoofedCurrent_deciAmps = ((int16_t)(battCurrent_deciAmps *  9) >> 4); //multiply by 0.563 (ideally 0.549) = tell MCM 56.3% actual
    #elif defined (SET_CURRENT_HACK_40) 
        spoofedCurrent_deciAmps = ((int16_t)(battCurrent_deciAmps * 11) >> 4); //multiply by 0.688 (ideally 0.686) = tell MCM 68.8% actual
    #elif defined (SET_CURRENT_HACK_20)
        spoofedCurrent_deciAmps = ((int16_t)(battCurrent_deciAmps * 13) >> 4); //multiply by 0.813 (ideally 0.800) = tell MCM 81.3% actual
    #elif defined (SET_CURRENT_HACK_00)
        spoofedCurrent_deciAmps = battCurrent_deciAmps;
    #else
        #error (SET_CURRENT_HACK_xx value not selected in config.h)
    #endif

    BATTSCI_setSpoofedCurrent_deciAmps(spoofedCurrent_deciAmps);
}

/////////////////////////////////////////////////////////////////////////////////////////

//only call this function when no current is flowing through the sensor (e.g. when key is off).
//MCM closes  pre-contactor ~190 ms after keyOn
//MCM closes main contactor ~330 ms after keyOn
void adc_calibrateBatteryCurrentSensorOffset(uint8_t isDebugTextSent)
{
    uint16_t adcResult = analogRead(PIN_BATTCURRENT);
    int8_t delta = ADC_NOMINAL_0A_COUNTS - adcResult;
    bool didTestPass = false;

    //verify returned value is in the right ballpark
    if ((delta > -10) && (delta < +10)) 
    {
        calibratedCurrentSensorOffset = delta;
        didTestPass = true;
    } 
    
    if (isDebugTextSent == DEBUG_TEXT_ENABLED)
    {
        Serial.print(F("\nADC 0A offset: "));
        Serial.print(delta);

        if (didTestPass == true) { Serial.print(F(" (pass)")); }
        else                     { Serial.print(F(" (fail)")); }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

uint16_t adc_getTemperature(uint8_t tempToMeasure) { return analogRead(tempToMeasure); }

/////////////////////////////////////////////////////////////////////////////////////////
