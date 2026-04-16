//
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef lidisplay_h
    #define lidisplay_h
	#define COLOR_CITRUS_00 21	// Original Citrus Photo
	#define COLOR_CITRUS_01 59	// New Citrus photo
	#define COLOR_RED_00    61	// Red
	#define COLOR_SILVER_00 60	// Silver

	// Nextion HMI colour codes:
	#define NEXTION_BLK 0
	#define NEXTION_BLU 31
	#define NEXTION_CYN 2047
	#define NEXTION_GRY 44373
	#define NEXTION_GRN 2016
	#define NEXTION_ORN 64480
	#define NEXTION_PUR 22556
	#define NEXTION_RED 63488
	#define NEXTION_WHT 65535
	#define NEXTION_YEL 65504
	#define NEXTION_GRIDCCHARGE_60S_T16_CLR	65516	// For 60S FoMoCo "E" block for element T16

	// Copy one of the above values and paste it after LIDISPLAY_SPLASH_PIC
	#define LIDISPLAY_SPLASH_PIC COLOR_CITRUS_00 // Default is COLOR_CITRUS_00 for the old Citrus photo.

	// Grid charger page cell colour sensitivity to imbalance
	#define LIDISPLAY_CELL_COLOR_BIN_SIZE_COUNTS 64 // 64 = 6.4mV window between cell colours on the grid charging page.  Don't go below CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE

	#define LIDISPLAY_SPLASH_PAGE_MS 6000 // How long the splash page shows on LiDisplay
	#define LIDISPLAY_GRID_CHARGE_PAGE_COOLDOWN_MS 6000 // Keep displaying the grid charging page this long before showing splash page when GC unplugged


    void LiDisplay_begin(void);

    void LiDisplay_handler(void);

    void LiDisplay_keyOn(void);

    void LiDisplay_keyOff(void);

    void LiDisplay_gridChargerUnplugged(void);

    void LiDisplay_gridChargerPluggedIn(void);

    void LiDisplay_setPageNumber(uint8_t page); // Candidate for deletion -- page selection should probably only be done within LiDisplay.cpp

	void LiDisplay_writeInstructionTerminationBytes(void);

	// Only serial commands below.
	void LiDisplay_serialBegin(void);

    uint8_t LiDisplay_bytesAvailableForWrite(void);

	String LiDisplay_printString(String data);

    uint8_t LiDisplay_writeByte(uint8_t data);

    uint8_t LiDisplay_readByte(void);

    uint8_t LiDisplay_bytesAvailableToRead(void);

#endif
