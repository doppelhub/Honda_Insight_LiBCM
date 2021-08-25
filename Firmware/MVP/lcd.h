#ifndef lcd_h
	#define lcd_h

	void lcd_initialize(void);

	void lcd_incrementLoopCount(void);

	void lcd_displayON(void);

	void lcd_displayOFF(void);

	void lcd_printStaticText(void);

	void lcd_printCellVoltage_hiLoDelta(uint16_t highCellVoltage, uint16_t lowCellVoltage);

	//The following could be a single function with enum:

	void lcd_printMaxEverVoltage(uint16_t voltage);

	void lcd_printMinEverVoltage(uint16_t voltage);

	void lcd_printStackVoltage_actual(uint8_t stackVoltage);

	void lcd_printStackVoltage_spoofed(uint8_t stackVoltage);

	void lcd_printNumErrors(uint8_t errorCount);

	void lcd_printPower(uint8_t packVoltage, int16_t packAmps);

#endif