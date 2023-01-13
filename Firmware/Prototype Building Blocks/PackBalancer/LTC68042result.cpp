//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//The latest results gathered from LTC6804 are stored here.

#include "libcm.h"

uint8_t isoSPI_errorCount = 0;
uint8_t LTC68042result_errorCount_get       (void                 ) { return isoSPI_errorCount;          }
void    LTC68042result_errorCount_set       (uint8_t newErrorCount) { isoSPI_errorCount = newErrorCount; }
void    LTC68042result_errorCount_increment (void                 ) { isoSPI_errorCount++;               }

uint8_t packVoltage_actual = 170;
void    LTC68042result_packVoltage_set (uint8_t voltage) { packVoltage_actual = voltage; }
uint8_t LTC68042result_packVoltage_get (void           ) { return packVoltage_actual; }

uint16_t minEverCellVoltage_counts = 65535; //since last key event
void     LTC68042result_minEverCellVoltage_set(uint16_t newMin_counts) { minEverCellVoltage_counts = newMin_counts; }
uint16_t LTC68042result_minEverCellVoltage_get(void                  ) { return minEverCellVoltage_counts; }

uint16_t maxEverCellVoltage_counts = 0; //since last key event
void     LTC68042result_maxEverCellVoltage_set(uint16_t newMax_counts) { maxEverCellVoltage_counts = newMax_counts; }
uint16_t LTC68042result_maxEverCellVoltage_get(void                  ) { return maxEverCellVoltage_counts; }

uint16_t loCellVoltage_counts = 34567;
void     LTC68042result_loCellVoltage_set(uint16_t newLo_counts) { loCellVoltage_counts = newLo_counts; }
uint16_t LTC68042result_loCellVoltage_get(void                 ) { return loCellVoltage_counts; }

uint16_t hiCellVoltage_counts = 34567;
void     LTC68042result_hiCellVoltage_set(uint16_t newHi_counts) { hiCellVoltage_counts = newHi_counts; }
uint16_t LTC68042result_hiCellVoltage_get(void                 ) { return hiCellVoltage_counts; }

//---------------------------------------------------------------------------------------

//All cell voltages in this array are guaranteed to be acquired at the same time
//(The similar array in LTC68042cell.c is a circular buffer that can contain data from two different ADC events)
//Note: LTC cells are 1-indexed, whereas array data is 0-indexed
//  Example: cellVoltages_counts[0][ 1] is IC_1 cell_02
//  Example: cellVoltages_counts[3][11] is IC_4 cell_12
uint16_t cellVoltagesCompleteFrame_counts[TOTAL_IC][CELLS_PER_IC];

void LTC68042result_specificCellVoltage_set(uint8_t icNumber, uint8_t cellNumber, uint16_t cellVoltage)
{
    cellVoltagesCompleteFrame_counts[icNumber][cellNumber] = cellVoltage;
}

uint16_t LTC68042result_specificCellVoltage_get (uint8_t icNumber, uint8_t cellNumber)
{
    return cellVoltagesCompleteFrame_counts[icNumber][cellNumber];
}
