//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042gpio_h
    #define LTC68042gpio_h

    uint8_t LTC6804_rdaux(uint8_t reg, uint8_t nIC, uint8_t addr_first_ic);

    void LTC6804_adax(void);

    bool LTC6804gpio_areAllVoltageReferencesPassing(void);

#endif
