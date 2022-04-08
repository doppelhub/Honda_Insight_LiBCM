//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042gpio_h
	#define LTC68042gpio_h

	int8_t LTC6804_rdaux(uint8_t reg, uint8_t nIC, uint16_t aux_codes[][6], uint8_t addr_first_ic);

	void LTC6804_rdaux_reg(uint8_t reg, uint8_t nIC,uint8_t *data, uint8_t addr_first_ic);

	void LTC6804_adax();


#endif