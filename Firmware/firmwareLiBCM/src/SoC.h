//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef soc_h
    #define soc_h

    void SoC_integrateCharge_adcCounts(int16_t adcCounts);

    uint16_t SoC_getBatteryStateNow_mAh(void);
    void     SoC_setBatteryStateNow_mAh(uint16_t newPackCharge_mAh);

    void     SoC_setBatteryStateNow_percent(uint8_t newSoC);
    uint8_t  SoC_getBatteryStateNow_percent(void);
    uint16_t SoC_getBatteryStateNow_deciPercent(void); //JTS2doLater: Use this in Battsci, etc.

    uint8_t SoC_estimateFromRestingCellVoltage_percent(void);

    void SoC_updateUsingLatestOpenCircuitVoltage(void);

    void SoC_handler(void);

    #ifdef BATTERY_TYPE_5AhG3
        #define STACK_mAh_NOM 5000 //nominal pack size (0:100% SoC)

        #define VCELL_SoC100 42000
        #define VCELL_SoC99 41820
        #define VCELL_SoC98 41640
        #define VCELL_SoC97 41460
        #define VCELL_SoC96 41280
        #define VCELL_SoC95 41100
        #define VCELL_SoC94 40980
        #define VCELL_SoC93 40860
        #define VCELL_SoC92 40740
        #define VCELL_SoC91 40620
        #define VCELL_SoC90 40500
        #define VCELL_SoC89 40400
        #define VCELL_SoC88 40300
        #define VCELL_SoC87 40200
        #define VCELL_SoC86 40100
        #define VCELL_SoC85 40000
        #define VCELL_SoC84 39880
        #define VCELL_SoC83 39760
        #define VCELL_SoC82 39640
        #define VCELL_SoC81 39520
        #define VCELL_SoC80 39400
        #define VCELL_SoC79 39260
        #define VCELL_SoC78 39120
        #define VCELL_SoC77 38980
        #define VCELL_SoC76 38840
        #define VCELL_SoC75 38700
        #define VCELL_SoC74 38620
        #define VCELL_SoC73 38540
        #define VCELL_SoC72 38460
        #define VCELL_SoC71 38380
        #define VCELL_SoC70 38300
        #define VCELL_SoC69 38230
        #define VCELL_SoC68 38160
        #define VCELL_SoC67 38090
        #define VCELL_SoC66 38020
        #define VCELL_SoC65 37950
        #define VCELL_SoC64 37890
        #define VCELL_SoC63 37830
        #define VCELL_SoC62 37770
        #define VCELL_SoC61 37710
        #define VCELL_SoC60 37650
        #define VCELL_SoC59 37580
        #define VCELL_SoC58 37510
        #define VCELL_SoC57 37440
        #define VCELL_SoC56 37370
        #define VCELL_SoC55 37300
        #define VCELL_SoC54 37250
        #define VCELL_SoC53 37200
        #define VCELL_SoC52 37150
        #define VCELL_SoC51 37100
        #define VCELL_SoC50 37050
        #define VCELL_SoC49 37000
        #define VCELL_SoC48 36950
        #define VCELL_SoC47 36900
        #define VCELL_SoC46 36850
        #define VCELL_SoC45 36800
        #define VCELL_SoC44 36750
        #define VCELL_SoC43 36700
        #define VCELL_SoC42 36650
        #define VCELL_SoC41 36600
        #define VCELL_SoC40 36550
        #define VCELL_SoC39 36480
        #define VCELL_SoC38 36410
        #define VCELL_SoC37 36340
        #define VCELL_SoC36 36270
        #define VCELL_SoC35 36200
        #define VCELL_SoC34 36140
        #define VCELL_SoC33 36080
        #define VCELL_SoC32 36020
        #define VCELL_SoC31 35960
        #define VCELL_SoC30 35900
        #define VCELL_SoC29 35800
        #define VCELL_SoC28 35700
        #define VCELL_SoC27 35600
        #define VCELL_SoC26 35500
        #define VCELL_SoC25 35400
        #define VCELL_SoC24 35360
        #define VCELL_SoC23 35320
        #define VCELL_SoC22 35280
        #define VCELL_SoC21 35240
        #define VCELL_SoC20 35200
        #define VCELL_SoC19 35160
        #define VCELL_SoC18 35120
        #define VCELL_SoC17 35080
        #define VCELL_SoC16 35040
        #define VCELL_SoC15 35000
        #define VCELL_SoC14 34900
        #define VCELL_SoC13 34800
        #define VCELL_SoC12 34700
        #define VCELL_SoC11 34540
        #define VCELL_SoC10 34200
        #define VCELL_SoC09 33900
        #define VCELL_SoC08 33600
        #define VCELL_SoC07 33200
        #define VCELL_SoC06 32800
        #define VCELL_SoC05 32200
        #define VCELL_SoC04 31700
        #define VCELL_SoC03 31000
        #define VCELL_SoC02 30500
        #define VCELL_SoC01 29500
        #define VCELL_SoC00 00000

    #elif defined BATTERY_TYPE_47Ah
        #define STACK_mAh_NOM 47000

        #define VCELL_SoC100 42000
        #define VCELL_SoC99 41800
        #define VCELL_SoC98 41600
        #define VCELL_SoC97 41400
        #define VCELL_SoC96 41200
        #define VCELL_SoC95 41000
        #define VCELL_SoC94 40880
        #define VCELL_SoC93 40760
        #define VCELL_SoC92 40640
        #define VCELL_SoC91 40520
        #define VCELL_SoC90 40400
        #define VCELL_SoC89 40260
        #define VCELL_SoC88 40120
        #define VCELL_SoC87 39980
        #define VCELL_SoC86 39840
        #define VCELL_SoC85 39700
        #define VCELL_SoC84 39600
        #define VCELL_SoC83 39500
        #define VCELL_SoC82 39400
        #define VCELL_SoC81 39300
        #define VCELL_SoC80 39200
        #define VCELL_SoC79 39100
        #define VCELL_SoC78 39000
        #define VCELL_SoC77 38900
        #define VCELL_SoC76 38800
        #define VCELL_SoC75 38700
        #define VCELL_SoC74 38600
        #define VCELL_SoC73 38500
        #define VCELL_SoC72 38400
        #define VCELL_SoC71 38300
        #define VCELL_SoC70 38200
        #define VCELL_SoC69 38100
        #define VCELL_SoC68 38000
        #define VCELL_SoC67 37900
        #define VCELL_SoC66 37800
        #define VCELL_SoC65 37700
        #define VCELL_SoC64 37580
        #define VCELL_SoC63 37460
        #define VCELL_SoC62 37340
        #define VCELL_SoC61 37220
        #define VCELL_SoC60 37100
        #define VCELL_SoC59 37020
        #define VCELL_SoC58 36940
        #define VCELL_SoC57 36860
        #define VCELL_SoC56 36780
        #define VCELL_SoC55 36770
        #define VCELL_SoC54 36640
        #define VCELL_SoC53 36580
        #define VCELL_SoC52 36520
        #define VCELL_SoC51 36460
        #define VCELL_SoC50 36400
        #define VCELL_SoC49 36360
        #define VCELL_SoC48 36320
        #define VCELL_SoC47 36280
        #define VCELL_SoC46 36240
        #define VCELL_SoC45 36200
        #define VCELL_SoC44 36160
        #define VCELL_SoC43 36120
        #define VCELL_SoC42 36080
        #define VCELL_SoC41 36040
        #define VCELL_SoC40 36000
        #define VCELL_SoC39 35960
        #define VCELL_SoC38 35920
        #define VCELL_SoC37 35880
        #define VCELL_SoC36 35840
        #define VCELL_SoC35 35800
        #define VCELL_SoC34 35760
        #define VCELL_SoC33 35720
        #define VCELL_SoC32 35680
        #define VCELL_SoC31 35640
        #define VCELL_SoC30 35600
        #define VCELL_SoC29 35540
        #define VCELL_SoC28 35480
        #define VCELL_SoC27 35420
        #define VCELL_SoC26 35360
        #define VCELL_SoC25 35300
        #define VCELL_SoC24 35240
        #define VCELL_SoC23 35180
        #define VCELL_SoC22 35120
        #define VCELL_SoC21 35060
        #define VCELL_SoC20 35000
        #define VCELL_SoC19 34880
        #define VCELL_SoC18 34760
        #define VCELL_SoC17 34640
        #define VCELL_SoC16 34520
        #define VCELL_SoC15 34400
        #define VCELL_SoC14 34320
        #define VCELL_SoC13 34240
        #define VCELL_SoC12 34160
        #define VCELL_SoC11 34080
        #define VCELL_SoC10 34000
        #define VCELL_SoC09 33900
        #define VCELL_SoC08 33800
        #define VCELL_SoC07 33700
        #define VCELL_SoC06 33600
        #define VCELL_SoC05 33200
        #define VCELL_SoC04 32600
        #define VCELL_SoC03 32000
        #define VCELL_SoC02 31000
        #define VCELL_SoC01 30000
        #define VCELL_SoC00 00000

    #else
        #error (Battery type not specified in config.h)
    #endif

#endif
