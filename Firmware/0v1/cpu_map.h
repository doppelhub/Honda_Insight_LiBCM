//Central pin mapping for different hardware revisions.

#ifdef CPU_MAP_MEGA2560
	#ifdef LIBCM_HW_REVB 

		//Serial port pins and interrupt vectors
		#define USBSERIAL_RX USART_RX_vect
		#define USBSERIAL_UDRE	USART_UDRE_vect

		//analog input signals fed to ADC
		// #define PIN_BATTCURRENT A0
		// #define PIN_TEMP_YEL A3
		// #define PIN_TEMP_GRN A4
		// #define PIN_TEMP_WHT A5
		// #define PIN_TEMP_BLU A6
		// #define PIN_VPIN_IN A7
		#define ADC_DDR DDRF
		#define ADC_DIR	PORTF
		#define ADC_CURRENT_BIT  0 //mega pin A0 PF0
		#define ADC_TEMP_YEL_BIT 3 //mega pin A3 PF3
		#define ADC_TEMP_GRN_BIT 4 //mega pin A4 PF4
		#define ADC_TEMP_WHT_BIT 5 //mega pin A5 PF5
		#define ADC_TEMP_BLU_BIT 6 //mega pin A6 PF6
		#define ADC_VPIN_IN_BIT  7 //mega pin A7 PF7
		#define ADC_MASK ((1<<ADC_CURRENT_BIT)|(1<<ADC_TEMP_YEL_BIT)|(1<<ADC_TEMP_GRN_BIT)|(1<<ADC_TEMP_WHT_BIT)|(1<<ADC_TEMP_BLU_BIT)|(1<<ADC_VPIN_IN_BIT))

		//pwm output signals
		// #define PIN_VPIN_OUT_PWM 4 PG5 
		// #define PIN_MCM_E_CONTROL_PWM 7 PH4
		// #define PIN_GRID_PWM 8 PH5
		// #define PIN_FAN_PWM 11 PB5


		//digital output signals
		// #define PIN_FANOEM_LOW A1 PF1
		// #define PIN_FANOEM_HI A2 PF2
		// #define PIN_TURNOFFLiBCM A8 PK0
		// #define PIN_HMI_EN A9 PK1 
		// #define PIN_BATTSCI_DE A10 PK2
		// #define PIN_BATTSCI_REn A11 PK3
		// #define PIN_LED1 A12 PK4
		// #define PIN_LED2 A13 PK5
		// #define PIN_LED3 A14 PK6
		// #define PIN_LED4 A15 PK7
		// #define PIN_METSCI_DE 2 PE4 
		// #define PIN_METSCI_REn 3 PE5
		// #define PIN_SPI_EXT_CS 5 PE3 
		// #define PIN_TEMP_SENSOR_EN 6 PH3 
		// #define PIN_GRID_EN 10 PB4
		// #define PIN_I_SENSOR_EN 12 PB6
		// #define PIN_IGNITION_SENSE 13 PB7
		// #define PIN_SPI_CS SS

		//digital input signals
		// #define PIN_GRID_SENSE 9 PH6

	#endif
#endif