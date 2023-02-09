//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042configure_h
	#define LTC68042configure_h

	//JTS2doLater: LiBCM also needs to determine cell count, and then sound an alarm if different from user-entered value (safety issue)
	//choose number of LTC ICs in isoSPI network
	#ifdef RUN_BRINGUP_TESTER
		#define TOTAL_IC 5
	#elif defined STACK_IS_48S
		#define TOTAL_IC 4
	#elif defined STACK_IS_60S
		#define TOTAL_IC 5
	#endif

	#define FIRST_IC_ADDR  2 //lowest address.  All additional IC addresses must be sequential
	#define CELLS_PER_IC  12 //Each LTC6804 measures QTY12 cells


	 // |CHG | Dec  |Channels to convert   |
	 // |----|------|----------------------|
	 // |000 | 0    | All GPIOS and 2nd Ref|
	 // |001 | 1    | GPIO 1               |
	 // |010 | 2    | GPIO 2               |
	 // |011 | 3    | GPIO 3               |
	 // |100 | 4    | GPIO 4               |
	 // |101 | 5    | GPIO 5               |
	 // |110 | 6    | Vref2                |
	#define AUX_CH_ALL 0
	#define AUX_CH_GPIO1 1
	#define AUX_CH_GPIO2 2
	#define AUX_CH_GPIO3 3
	#define AUX_CH_GPIO4 4
	#define AUX_CH_GPIO5 5
	#define AUX_CH_VREF2 6


	//JTS2doLater: Does reducing corner frequency to 26 Hz reduce assist/regen noise? //Add 214 ms wait before reading 
	//ADC LPF Fcorner:       Total conversion time (QTY12 cells/IC)
	//ADCOPT(CFGR0[0] = 0)    
	// MD = 01 27000 Hz        1.2 ms fast
	// MD = 10  7000 Hz        2.5 ms (default)
	// MD = 11    26 Hz      213.5 ms filtered
	//
	//ADCOPT(CFGR0[0] = 1)
	// MD = 01 14000 Hz        1.3 ms
	// MD = 10  3000 Hz        3.0 ms
	// MD = 11  2000 Hz        4.4 ms
	// |command    |  10   |   9   |   8   |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
	// |-----------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
	// |ADCV:      |   0   |   1   | MD[1] | MD[2] |   1   |   1   |  DCP  |   0   | CH[2] | CH[1] | CH[0] |
	// |ADAX:      |   1   |   0   | MD[1] | MD[2] |   1   |   1   |  DCP  |   0   | CHG[2]| CHG[1]| CHG[0]|
	#define MD_FAST 1
	#define MD_NORMAL 2
	#define MD_FILTERED 3

	
	// |CH | Dec  | Channels to convert |
	// |---|------|---------------------|
	// |000| 0    | All Cells           |
	// |001| 1    | Cell 1 and Cell 7   |
	// |010| 2    | Cell 2 and Cell 8   |
	// |011| 3    | Cell 3 and Cell 9   |
	// |100| 4    | Cell 4 and Cell 10  |
	// |101| 5    | Cell 5 and Cell 11  |
	// |110| 6    | Cell 6 and Cell 12  |
	#define CELL_CH_ALL 0
	#define CELL_CH_1and7 1
	#define CELL_CH_2and8 2
	#define CELL_CH_3and9 3
	#define CELL_CH_4and10 4
	#define CELL_CH_5and11 5
	#define CELL_CH_6and12 6


	//|DCP | Discharge Permitted During conversion                                |
	//|----|----------------------------------------------------------------------|
	//|0   | Discharge FETs temporarily turned off prior to each cell measurement |
	//|1   | Discharge FET states not altered                                     |
	#define DCP_DISABLED 0
	#define DCP_ENABLED 1

	#ifdef RUN_BRINGUP_TESTER
		#define IS_DISCHARGE_ALLOWED_DURING_CONVERSION DCP_ENABLED
	#else
		#define IS_DISCHARGE_ALLOWED_DURING_CONVERSION DCP_DISABLED
	#endif

	static const unsigned int crc15Table[256] = {
	  0x0,    0xc599, 0xceab, 0xb32,  0xd8cf, 0x1d56, 0x1664, 0xd3fd,
	  0xf407, 0x319e, 0x3aac, 0xff35, 0x2cc8, 0xe951, 0xe263, 0x27fa,
	  0xad97, 0x680e, 0x633c, 0xa6a5, 0x7558, 0xb0c1, 0xbbf3, 0x7e6a,
	  0x5990, 0x9c09, 0x973b, 0x52a2, 0x815f, 0x44c6, 0x4ff4, 0x8a6d,
	  0x5b2e, 0x9eb7, 0x9585, 0x501c, 0x83e1, 0x4678, 0x4d4a, 0x88d3,
	  0xaf29, 0x6ab0, 0x6182, 0xa41b, 0x77e6, 0xb27f, 0xb94d, 0x7cd4,
	  0xf6b9, 0x3320, 0x3812, 0xfd8b, 0x2e76, 0xebef, 0xe0dd, 0x2544,
	  0x2be,  0xc727, 0xcc15, 0x98c,  0xda71, 0x1fe8, 0x14da, 0xd143,
	  0xf3c5, 0x365c, 0x3d6e, 0xf8f7, 0x2b0a, 0xee93, 0xe5a1, 0x2038,
	  0x7c2,  0xc25b, 0xc969, 0xcf0,  0xdf0d, 0x1a94, 0x11a6, 0xd43f,
	  0x5e52, 0x9bcb, 0x90f9, 0x5560, 0x869d, 0x4304, 0x4836, 0x8daf,
	  0xaa55, 0x6fcc, 0x64fe, 0xa167, 0x729a, 0xb703, 0xbc31, 0x79a8,
	  0xa8eb, 0x6d72, 0x6640, 0xa3d9, 0x7024, 0xb5bd, 0xbe8f, 0x7b16,
	  0x5cec, 0x9975, 0x9247, 0x57de, 0x8423, 0x41ba, 0x4a88, 0x8f11,
	  0x57c,  0xc0e5, 0xcbd7, 0xe4e,  0xddb3, 0x182a, 0x1318, 0xd681,
	  0xf17b, 0x34e2, 0x3fd0, 0xfa49, 0x29b4, 0xec2d, 0xe71f, 0x2286,
	  0xa213, 0x678a, 0x6cb8, 0xa921, 0x7adc, 0xbf45, 0xb477, 0x71ee,
	  0x5614, 0x938d, 0x98bf, 0x5d26, 0x8edb, 0x4b42, 0x4070, 0x85e9,
	  0xf84,  0xca1d, 0xc12f, 0x4b6,  0xd74b, 0x12d2, 0x19e0, 0xdc79,
	  0xfb83, 0x3e1a, 0x3528, 0xf0b1, 0x234c, 0xe6d5, 0xede7, 0x287e,
	  0xf93d, 0x3ca4, 0x3796, 0xf20f, 0x21f2, 0xe46b, 0xef59, 0x2ac0,
	  0xd3a,  0xc8a3, 0xc391, 0x608,  0xd5f5, 0x106c, 0x1b5e, 0xdec7,
	  0x54aa, 0x9133, 0x9a01, 0x5f98, 0x8c65, 0x49fc, 0x42ce, 0x8757,
	  0xa0ad, 0x6534, 0x6e06, 0xab9f, 0x7862, 0xbdfb, 0xb6c9, 0x7350,
	  0x51d6, 0x944f, 0x9f7d, 0x5ae4, 0x8919, 0x4c80, 0x47b2, 0x822b,
	  0xa5d1, 0x6048, 0x6b7a, 0xaee3, 0x7d1e, 0xb887, 0xb3b5, 0x762c,
	  0xfc41, 0x39d8, 0x32ea, 0xf773, 0x248e, 0xe117, 0xea25, 0x2fbc,
	  0x846,  0xcddf, 0xc6ed, 0x374,  0xd089, 0x1510, 0x1e22, 0xdbbb,
	  0xaf8,  0xcf61, 0xc453, 0x1ca,  0xd237, 0x17ae, 0x1c9c, 0xd905,
	  0xfeff, 0x3b66, 0x3054, 0xf5cd, 0x2630, 0xe3a9, 0xe89b, 0x2d02,
	  0xa76f, 0x62f6, 0x69c4, 0xac5d, 0x7fa0, 0xba39, 0xb10b, 0x7492,
	  0x5368, 0x96f1, 0x9dc3, 0x585a, 0x8ba7, 0x4e3e, 0x450c, 0x8095
	};
	/*Code used to generate this crc15 table:
	void generate_crc15_table()
	{
	  int remainder;
	  for(int i = 0; i<256;i++)
	  {
	    remainder =  i<< 7;
	    for (int bit = 8; bit > 0; --bit)
	        {
	          if ((remainder & 0x4000) > 0)//equivalent to remainder & 2^14 simply check for MSB
	          {
	            remainder = ((remainder << 1)) ;
	            remainder = (remainder ^ 0x4599);
	          } else {
	            remainder = ((remainder << 1));
	          }
	       }
	    crc15Table[i] = remainder&0xFFFF;
	  }
	}
	*/

	//'DCTO' Discharge timeout values (inclusive) //see Table12
	#define LTC6804_DISCHARGE_TIMEOUT_02_SECONDS  0x00 //software timer disabled
	#define LTC6804_DISCHARGE_TIMEOUT_30_SECONDS  0x10 //0.5 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_01_MINUTE   0x20 //1 minute
	#define LTC6804_DISCHARGE_TIMEOUT_02_MINUTES  0x30 //2 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_03_MINUTES  0x40 //3 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_04_MINUTES  0x50 //4 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_05_MINUTES  0x60 //5 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_10_MINUTES  0x70 //10 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_15_MINUTES  0x80 //15 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_20_MINUTES  0x90 //20 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_30_MINUTES  0xA0 //30 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_40_MINUTES  0xB0 //40 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_60_MINUTES  0xC0 //60 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_75_MINUTES  0xD0 //75 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_90_MINUTES  0xE0 //90 minutes
	#define LTC6804_DISCHARGE_TIMEOUT_120_MINUTES 0xF0 //120 minutes

	#define LTC6804_CORE_ALREADY_AWAKE true
	#define LTC6804_CORE_JUST_WOKE_UP  false

	#define BROADCAST_TO_ALL_ICS 16 //valid LTC6804 addresses are 0:15

	#define LTC6804_MASK_REFON_BIT 0x02

	void LTC68042configure_initialize(void);

	void LTC68042configure_handleKeyStateChange(void);

	bool LTC68042configure_wakeup(void);

	uint16_t LTC68042configure_calcPEC15(uint8_t len, uint8_t *data);

	void LTC68042configure_spiWrite( uint8_t length, uint8_t *data);

	void LTC68042configure_spiWriteRead(uint8_t *TxData, uint8_t TXlen, uint8_t *rx_data, uint8_t RXlen);

	void LTC68042configure_programVolatileDefaults(void);
	
	void LTC68042configure_setBalanceResistors(uint8_t icAddress, uint16_t cellBitmap, uint8_t softwareTimeout);

#endif