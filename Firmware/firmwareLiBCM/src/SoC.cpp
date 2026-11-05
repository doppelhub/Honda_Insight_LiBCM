//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//maintains battery state of charge

//JTS2doLater: add "REMAP_ACTUAL_SoC_TO_FULL_SCALE_PERCENT" feature //remaps actual SoC and displays to user as "0%=empty" and "100%=full"
//JTS2doLater:
//-Save SoC(RAM) to EEPROM
//-If value in RAM is more than 10% different from EEPROM value, update EEPROM.
//-Also store if key recently turned off (keyState_previous == on && keyState_now == off)

#include "libcm.h"

uint16_t stackFull_Calculated_mAh   = STACK_mAh_NOM; //JTS2doLater: add cell wear adjustment over time
uint16_t packCharge_Now_mAh         = 0;
uint8_t  packCharge_Now_percent     = 0;
uint16_t packCharge_Now_deciPercent = 0;

uint16_t SoC_getBatteryStateNow_mAh(void) { return packCharge_Now_mAh; }
void     SoC_setBatteryStateNow_mAh(uint16_t newPackCharge_mAh) { packCharge_Now_mAh = newPackCharge_mAh; }

void     SoC_setBatteryStateNow_percent(uint8_t newSoC_percent) { packCharge_Now_mAh = (uint16_t)(stackFull_Calculated_mAh * 0.01) * newSoC_percent; }
uint8_t  SoC_getBatteryStateNow_percent(void)     { return packCharge_Now_percent;     }
uint16_t SoC_getBatteryStateNow_deciPercent(void) { return packCharge_Now_deciPercent; } 

/////////////////////////////////////////////////////////////////////////////////////////

//integrate current over time (coulomb counting)
//LiBCM uses this function to determine SoC while keyON
void SoC_integrateCharge_adcCounts(int16_t adcCounts)
{
    //determine how much 'charge' (in ADC counts) went into the battery
    int16_t deltaCharge_counts = adcCounts - ADC_NOMINAL_0A_COUNTS; //positive during assist, negative during regen //adcCounts already calibrated to zero crossing
    
    //Time for some dimensional analysis!
    //Note: items in brackets are units (e.g. "123 [amps]", "456 [volts]")
    //We know that: 1 [ A] = 1 [ Coulomb] / 1 [ s] 
    //      and so: 1 [mA] = 1 [uCoulomb] / 1 [ms] 
    // If we then divide both sides by 1 mA, we get:
    // 1 [] = (1 [uCoulomb/ms] / 1 [mA])... the equation below includes this version of '1' (to help us cancel units)       
    //then:
    // deltaCharge_uCoulomb [uCoulomb] = (deltaCharge_counts [delta_Counts] / 1) * (ADC_MILLIAMPS_PER_COUNT [mA] / 1 [delta_Counts]) * (1 [uCoulomb/ms] / 1 [mA]) * (time_loopPeriod_ms_get() [ms] / 1)
    //cancel units:
    // deltaCharge_uCoulomb [uCoulomb] = (deltaCharge_counts [            ] / 1) * (ADC_MILLIAMPS_PER_COUNT [  ] / 1 [            ]) * (1 [uCoulomb/  ] / 1 [  ]) * (time_loopPeriod_ms_get() [  ] / 1)
    //therefore:
    // deltaCharge_uCoulomb            =  deltaCharge_counts                     *  ADC_MILLIAMPS_PER_COUNT                                                       *  time_loopPeriod_ms_get()
    //int32_t deltaCharge_uCoulomb = (int32_t)deltaCharge_counts * (ADC_MILLIAMPS_PER_COUNT * time_loopPeriod_ms_get());
    //Note: if the battery current value isn't updated each loop, then multiply by the number of loops required for each result
    //JTS2doLater: change time_loopPeriod_ms_get() to delta millis since last run //prevents accumulated error if we don't meet our loop time
    int32_t deltaCharge_uCoulomb = (int32_t)deltaCharge_counts * (time_loopPeriod_ms_get() * ADC_MILLIAMPS_PER_COUNT);

    //Notes:
    //5 Ah is 18.0E9 uCoulombs, whereas 2^32 is ~4.3E9 counts...
    //so uint32_t isn't large enough to store the entire battery's charge...
    //so instead we'll store the total battery charge in microAmpHours (1 uAh = 3600 uC)...
    //and we'll decriment an intermediate uCoulomb buffer to zero
    static int32_t intermediateChargeBuffer_uCoulomb = 0;

    intermediateChargeBuffer_uCoulomb += deltaCharge_uCoulomb; //gets more positive during assist //gets more negative during regen

    #define ONE_MILLIAMPHOUR_IN_MICROCOULOMBS 3600000

    while (intermediateChargeBuffer_uCoulomb > ONE_MILLIAMPHOUR_IN_MICROCOULOMBS )
    {   //assist
        intermediateChargeBuffer_uCoulomb -= ONE_MILLIAMPHOUR_IN_MICROCOULOMBS;  //remove 1 mAh from buffer
        if (packCharge_Now_mAh > 0) { packCharge_Now_mAh--; } //pack discharged 1 mAh (assist)
    }

    while (intermediateChargeBuffer_uCoulomb < -ONE_MILLIAMPHOUR_IN_MICROCOULOMBS)
    {   //regen
        intermediateChargeBuffer_uCoulomb += ONE_MILLIAMPHOUR_IN_MICROCOULOMBS; //add 1 mAh to buffer
        if (packCharge_Now_mAh < 65535) { packCharge_Now_mAh++; } //pack charged 1 mAh (regen)
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

//Estimate SoC based on resting cell voltage.
//LiBCM uses this function to determine SoC while keyOFF
//EHW5 cells settle to final 'resting' voltage in ten minutes, but are fairly close to that value after just one minute
void SoC_updateUsingLatestOpenCircuitVoltage(void)
{
    uint8_t batterySoC_percent = SoC_estimateFromRestingCellVoltage_percent(); //determine resting SoC

    Serial.print(F("\nOld SoC: "));
    Serial.print( String(SoC_getBatteryStateNow_percent()) );
    Serial.print(F("%, New SoC:"));
    Serial.print(String(batterySoC_percent));
    Serial.print('%');

    if (LTC68042result_hiCellVoltage_get() > CELL_VMAX_REGEN)       { Serial.print(F("\nDANGER: Cell(s) Overcharged!!")); }
    if (LTC68042result_loCellVoltage_get() < CELL_VMIN_GRIDCHARGER) { Serial.print(F("\nDANGER: Cell(s) Discharged!!" )); }

    SoC_setBatteryStateNow_percent(batterySoC_percent); //update SoC
}

/////////////////////////////////////////////////////////////////////////////////////////

//Calling this function when battery is sourcing/sinking current will cause estimation error
//Wait at least ten minutes after keyOff for most accurate results
uint8_t SoC_estimateFromRestingCellVoltage_percent(void)
    {
        //array index is SoC percentage
        const uint16_t LUT_voltageToSoC[101] = {
            VCELL_SoC00, VCELL_SoC01, VCELL_SoC02, VCELL_SoC03, VCELL_SoC04, VCELL_SoC05, VCELL_SoC06, VCELL_SoC07, VCELL_SoC08, VCELL_SoC09,
            VCELL_SoC10, VCELL_SoC11, VCELL_SoC12, VCELL_SoC13, VCELL_SoC14, VCELL_SoC15, VCELL_SoC16, VCELL_SoC17, VCELL_SoC18, VCELL_SoC19,
            VCELL_SoC20, VCELL_SoC21, VCELL_SoC22, VCELL_SoC23, VCELL_SoC24, VCELL_SoC25, VCELL_SoC26, VCELL_SoC27, VCELL_SoC28, VCELL_SoC29,
            VCELL_SoC30, VCELL_SoC31, VCELL_SoC32, VCELL_SoC33, VCELL_SoC34, VCELL_SoC35, VCELL_SoC36, VCELL_SoC37, VCELL_SoC38, VCELL_SoC39,
            VCELL_SoC40, VCELL_SoC41, VCELL_SoC42, VCELL_SoC43, VCELL_SoC44, VCELL_SoC45, VCELL_SoC46, VCELL_SoC47, VCELL_SoC48, VCELL_SoC49,
            VCELL_SoC50, VCELL_SoC51, VCELL_SoC52, VCELL_SoC53, VCELL_SoC54, VCELL_SoC55, VCELL_SoC56, VCELL_SoC57, VCELL_SoC58, VCELL_SoC59,
            VCELL_SoC60, VCELL_SoC61, VCELL_SoC62, VCELL_SoC63, VCELL_SoC64, VCELL_SoC65, VCELL_SoC66, VCELL_SoC67, VCELL_SoC68, VCELL_SoC69,
            VCELL_SoC70, VCELL_SoC71, VCELL_SoC72, VCELL_SoC73, VCELL_SoC74, VCELL_SoC75, VCELL_SoC76, VCELL_SoC77, VCELL_SoC78, VCELL_SoC79,
            VCELL_SoC80, VCELL_SoC81, VCELL_SoC82, VCELL_SoC83, VCELL_SoC84, VCELL_SoC85, VCELL_SoC86, VCELL_SoC87, VCELL_SoC88, VCELL_SoC89,
            VCELL_SoC90, VCELL_SoC91, VCELL_SoC92, VCELL_SoC93, VCELL_SoC94, VCELL_SoC95, VCELL_SoC96, VCELL_SoC97, VCELL_SoC98, VCELL_SoC99,
            VCELL_SoC100
        };

        uint16_t cellVoltage = LTC68042result_loCellVoltage_get(); //JTS2doLater: need an algorithm to look at hi cell, too.
        uint8_t indexLo = 0;
        uint8_t indexHi = 100;

        while (indexHi - indexLo > 1)
        {
            uint8_t indexMid = indexLo + ((indexHi - indexLo) >> 1);

            if (cellVoltage < LUT_voltageToSoC[indexMid]) { indexHi = indexMid; }
            else                                          { indexLo = indexMid; }
        }

        return indexLo;
    }

/////////////////////////////////////////////////////////////////////////////////////////

void SoC_handler(void)
{
    if (packCharge_Now_mAh > stackFull_Calculated_mAh) { packCharge_Now_mAh = stackFull_Calculated_mAh; }

    packCharge_Now_deciPercent = (uint16_t)( ((uint32_t)packCharge_Now_mAh * 1000) / stackFull_Calculated_mAh);    
    packCharge_Now_percent = packCharge_Now_deciPercent / 10;
}

/////////////////////////////////////////////////////////////////////////////////////////
