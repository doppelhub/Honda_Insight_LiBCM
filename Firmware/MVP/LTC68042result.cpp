//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//The latest results gathered from LTC6804 are stored here.

#include "libcm.h"

uint8_t isoSPI_errorCount = 0;
uint8_t LTC68042result_errorCount_get       (void                 ) { return isoSPI_errorCount;          }
void    LTC68042result_errorCount_set       (uint8_t newErrorCount) { isoSPI_errorCount = newErrorCount; }
void    LTC68042result_errorCount_increment (void                 ) { isoSPI_errorCount++;               }

uint8_t packVoltage_actual = 170; //JTS2do: See if MCM happy with 0 volts
void    LTC68042result_packVoltage_set (uint8_t voltage) { packVoltage_actual = voltage; }
uint8_t LTC68042result_packVoltage_get (void           ) { return packVoltage_actual; }

uint16_t minEverCellVoltage_counts = 65535; //since last key event
void     LTC68042result_minEverCellVoltage_set(uint16_t newMin_counts) { minEverCellVoltage_counts = newMin_counts; }
uint16_t LTC68042result_minEverCellVoltage_get(void                  ) { return minEverCellVoltage_counts; }

uint16_t maxEverCellVoltage_counts = 0; //since last key event
void     LTC68042result_maxEverCellVoltage_set(uint16_t newMax_counts) { maxEverCellVoltage_counts = newMax_counts; }
uint16_t LTC68042result_maxEverCellVoltage_get(void                  ) { return maxEverCellVoltage_counts; }

uint16_t loCellVoltage_counts = 34560;
void     LTC68042result_loCellVoltage_set(uint16_t newLo_counts) { loCellVoltage_counts = newLo_counts; }
uint16_t LTC68042result_loCellVoltage_get(void                 ) { return loCellVoltage_counts; }

uint16_t hiCellVoltage_counts = 34560;
void     LTC68042result_hiCellVoltage_set(uint16_t newHi_counts) { hiCellVoltage_counts = newHi_counts; }
uint16_t LTC68042result_hiCellVoltage_get(void                 ) { return hiCellVoltage_counts; }
         
//JTS2doNow: Incorporate with existing variable buffers in debugUSB.c & lcd.c

    // #ifdef PRINT_ALL_CELL_VOLTAGES_TO_USB
    //   #warning (Printing all cell voltages to USB severely reduces Superloop rate)
    //   printCellVoltage_all();
    // #endif

    // void printCellVoltage_all()
    // { // VERY SLOW & blocking (serial transmit buffer filled)
    //   // Use only for debug purposes
    //   Serial.print('\n');
    //   for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
    //   {
    //     Serial.print(F("IC "));
    //     Serial.print( (current_ic + FIRST_IC_ADDR) ,DEC);
    //     for (int i=0; i<(CELLS_PER_IC); i++)
    //     {
    //       const uint8_t NUM_DECIMAL_PLACES = 4; //JTS2doNow: change back to 4 digits
    //       Serial.print(F(" C"));
    //       Serial.print(i+1,DEC); //Note: cell voltages always reported back C1:C12 (not C13:C24)
    //       Serial.print(':');
    //       Serial.print( (cellVoltages_counts[current_ic][i] * 0.0001), NUM_DECIMAL_PLACES ); 
    //     }
    //     Serial.print('\n');
    //   }
    // }

    // debugUSB_cellHI_counts(highCellVoltage);
    // debugUSB_cellLO_counts(lowCellVoltage);

    // LTC6804_printCellVoltage_max_min(); //JTS2doNow: only calculate max/min after all cell voltages read.

    //    Serial.print(F("\nerr:LTC"));
