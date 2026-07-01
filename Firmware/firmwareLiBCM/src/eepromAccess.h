//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef eepromAccess_h
    #define eepromAccess_h

    #define EEPROM_LIBCM_DISABLED_REGEN  0xAA
    #define EEPROM_REGEN_NEVER_LIMITED   0x55
    #define EEPROM_LIBCM_DISABLED_ASSIST 0xCC
    #define EEPROM_ASSIST_NEVER_LIMITED  0x33

    #define EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_16b 0xFFFF
    #define EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b  0xFF
    #define EEPROM_ADDRESS_FORMATTED_VALUE           0x00

    #define BYTES_IN_DATE 12
    #define BYTES_IN_TIME  9

    #define REQUIRED_FIRMWARE_UPDATE_PERIOD_DAYS 120
    #define REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS (REQUIRED_FIRMWARE_UPDATE_PERIOD_DAYS * 24)

    #define FIRMWARE_EXPIRED   0b10101010 //alternating bit pattern for EEPROM read/write integrity
    #define FIRMWARE_UNEXPIRED 0b01010101

    #define EEPROM_OFFSET_TIME 0
    #define EEPROM_OFFSET_DIST 2
    #define EEPROM_OFFSET_ASST 4
    #define EEPROM_OFFSET_RGEN 6

    void eeprom_keyOffCheckForExpiredFirmware(void);
    
    uint16_t eeprom_hoursSinceLastFirmwareUpdate_get(void);
    void     eeprom_hoursSinceLastFirmwareUpdate_set(uint16_t);

    uint8_t eeprom_expirationStatus_get(void);

    uint8_t eeprom_hasLibcmDisabledAssist_get(void);
    void    eeprom_hasLibcmDisabledAssist_set(uint8_t);

    uint8_t eeprom_hasLibcmDisabledRegen_get(void);
    void    eeprom_hasLibcmDisabledRegen_set(uint8_t);

    uint16_t eeprom_maxCellVoltageDelta_get(void);
    void     eeprom_maxCellVoltageDelta_set(uint16_t);  

    uint8_t eeprom_delayKeyON_ms_get(void);
    void    eeprom_delayKeyON_ms_set(uint8_t);

    int8_t eeprom_getVspoofOffset_BVO(void);
    int8_t eeprom_getVspoofOffset_MDV(void);
    int8_t eeprom_getVspoofOffset_SPF(void);

    void eeprom_setVspoofOffset_BVO(int8_t);
    void eeprom_setVspoofOffset_MDV(int8_t);
    void eeprom_setVspoofOffset_SPF(int8_t);

    void eeprom_resetDebugValues(void);

    //config.h "sticky override" tunables (see config.h for behavior description)
    uint8_t eeprom_stackSoCMax_get(void);
    void    eeprom_stackSoCMax_set(uint8_t);

    uint8_t eeprom_stackSoCMin_get(void);
    void    eeprom_stackSoCMin_set(uint8_t);

    uint16_t eeprom_cellVmaxRegen_get(void);
    void     eeprom_cellVmaxRegen_set(uint16_t);

    uint16_t eeprom_cellVminAssist_get(void);
    void     eeprom_cellVminAssist_set(uint16_t);

    uint16_t eeprom_cellVmaxGridcharger_get(void);
    void     eeprom_cellVmaxGridcharger_set(uint16_t);

    uint16_t eeprom_cellVminGridcharger_get(void);
    void     eeprom_cellVminGridcharger_set(uint16_t);

    uint16_t eeprom_cellVminKeyoff_get(void);
    void     eeprom_cellVminKeyoff_set(uint16_t);

    uint8_t eeprom_cellBalanceMinSoC_get(void);
    void    eeprom_cellBalanceMinSoC_set(uint8_t);

    uint8_t eeprom_cellBalanceMaxTemp_C_get(void);
    void    eeprom_cellBalanceMaxTemp_C_set(uint8_t);

    uint8_t eeprom_coolBatteryAboveTempC_Keyoff_get(void);
    void    eeprom_coolBatteryAboveTempC_Keyoff_set(uint8_t);

    uint8_t eeprom_coolBatteryAboveTempC_Gridcharging_get(void);
    void    eeprom_coolBatteryAboveTempC_Gridcharging_set(uint8_t);

    uint8_t eeprom_coolBatteryAboveTempC_Keyon_get(void);
    void    eeprom_coolBatteryAboveTempC_Keyon_set(uint8_t);

    uint8_t eeprom_heatBatteryBelowTempC_Keyon_get(void);
    void    eeprom_heatBatteryBelowTempC_Keyon_set(uint8_t);

    uint8_t eeprom_heatBatteryBelowTempC_Gridcharging_get(void);
    void    eeprom_heatBatteryBelowTempC_Gridcharging_set(uint8_t);

    uint8_t eeprom_heatBatteryBelowTempC_Keyoff_get(void);
    void    eeprom_heatBatteryBelowTempC_Keyoff_set(uint8_t);

    uint8_t eeprom_keyoffDisableThermalManagementBelowSoCPercent_get(void);
    void    eeprom_keyoffDisableThermalManagementBelowSoCPercent_set(uint8_t);

    uint8_t eeprom_poweroffDelayAfterKeyoffPackEmptyMinutes_get(void);
    void    eeprom_poweroffDelayAfterKeyoffPackEmptyMinutes_set(uint8_t);

    //value of 0 means feature is disabled (LiBCM never turns off due to elapsed days alone)
    uint8_t eeprom_poweroffDelayAfterKeyoffDays_get(void);
    void    eeprom_poweroffDelayAfterKeyoffDays_set(uint8_t);

    bool eeprom_isPositiveSignDuringAssist_get(void);
    void eeprom_isPositiveSignDuringAssist_set(bool);

    uint16_t eeprom_debugUsbUpdatePeriodGridcharge_ms_get(void);
    void     eeprom_debugUsbUpdatePeriodGridcharge_ms_set(uint16_t);

    bool eeprom_isAssistDisabled_get(void);
    void eeprom_isAssistDisabled_set(bool);

    bool eeprom_isRegenDisabled_get(void);
    void eeprom_isRegenDisabled_set(bool);

    bool eeprom_isCellVoltageMismatchIgnored_get(void);
    void eeprom_isCellVoltageMismatchIgnored_set(bool);

    uint8_t eeprom_minSpoofedVoltage60S_get(void);
    void    eeprom_minSpoofedVoltage60S_set(uint8_t);

    void eeprom_applyConfigOverrides(void);

    //hardware-identity fields with no safe default -- applied at boot (not keyOff) so a fresh/reset board can escape the "unconfigured" state
    void eeprom_applyBootCriticalConfigOverrides(void);

    //fatal-halts if a boot-critical hardware field is unconfigured, or an invalid hardware combination is stored
    void eeprom_validateHardwareConfig(void);

    //current-hack multiplier selection (see adc.cpp) -- EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE_8b (0xFF) means unconfigured
    #define CURRENT_HACK_00 0
    #define CURRENT_HACK_20 1
    #define CURRENT_HACK_40 2
    #define CURRENT_HACK_60 3

    uint8_t eeprom_currentHackMode_get(void);
    void    eeprom_currentHackMode_set(uint8_t);

    //grid charger type -- no safe default (see eeprom_currentHackMode_get() comment above)
    #define GRIDCHARGER_TYPE_NOT_1500W 0
    #define GRIDCHARGER_TYPE_1500W     1

    uint8_t eeprom_gridChargerType_get(void);
    void    eeprom_gridChargerType_set(uint8_t);

    //battery type -- no safe default (see eeprom_currentHackMode_get() comment above)
    #define BATTERY_TYPE_VALUE_5AhG3 0
    #define BATTERY_TYPE_VALUE_47Ah  1

    uint8_t eeprom_batteryType_get(void);
    void    eeprom_batteryType_set(uint8_t);

    //voltage-spoofing mode -- has a safe default (see eeprom_gridChargerInputCurrentLimit_get() comment above)
    #define VOLTAGE_SPOOFING_MODE_DISABLE               0
    #define VOLTAGE_SPOOFING_MODE_ASSIST_ONLY_VARIABLE  1
    #define VOLTAGE_SPOOFING_MODE_ASSIST_ONLY_BINARY    2
    #define VOLTAGE_SPOOFING_MODE_ASSIST_AND_REGEN      3 //DEPRECATED (regen too strong) -- see vPackSpoof.cpp
    #define VOLTAGE_SPOOFING_MODE_LINEAR                4

    uint8_t eeprom_voltageSpoofingMode_get(void);
    void    eeprom_voltageSpoofingMode_set(uint8_t);

    //stack size -- no safe default (see eeprom_currentHackMode_get() comment above) //see LTC68042configure_totalIC_get() for the runtime IC count
    #define STACK_SIZE_VALUE_48S 0
    #define STACK_SIZE_VALUE_60S 1

    uint8_t eeprom_stackSize_get(void);
    void    eeprom_stackSize_set(uint8_t);

    void eeprom_verifyDataValid(void);

    void eeprom_batteryHistory_reset(void);

    void eeprom_resetAll(void);
    void eeprom_resetAll_userConfirm(void);

    void eeprom_batteryHistory_incrementValue(uint8_t indexTemperature, uint8_t indexSoC);

    uint16_t eeprom_batteryHistory_getValue(uint8_t indexTemperature, uint8_t indexSoC);

    void eeprom_wattHourHistory_storeSession(uint16_t time_s, uint16_t distance_TBD, uint16_t assist_Wh, uint16_t regen_Wh);
    void eeprom_wattHourHistory_printTripHistory(void);
    void eeprom_wattHourHistory_reset(void);

    void eeprom_begin(void);

    void writeToEEPROM_uint16(uint16_t startAddress, uint16_t value);

    void eeprom_printAll(void);

#endif
