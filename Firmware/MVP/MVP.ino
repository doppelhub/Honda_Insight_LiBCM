//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
//Lithium battery BMS for G1 Honda Insight.  Replaces OEM BCM module.

//JTS2doLater: Make this the only line of code in this file:
#include "libcm.h"

void setup() //~t=2 milliseconds, BUT NOTE this doesn't include CPU_CLOCK warmup or bootloader delay 
{
	gpio_begin();
  Serial.begin(115200); //USB
  METSCI_begin();
  BATTSCI_begin();

  LTC68042configure_initialize();

  if( gpio_keyStateNow() == KEYON ){ LED(3,HIGH); } //LED3 turns on if key was on when LiBCM first booted
  
  Serial.print(F("\n\nWelcome to LiBCM v" FW_VERSION "," BUILD_DATE "\n\n"));
}

void loop()
{
	static uint32_t previousMillis = millis();

	key_stateChangeHandler();

	gridCharger_handler();

	//---------------------------------------------------------------------------------------

	if( (key_getSampledState() == KEYON) || (gridCharger_getSampledState() == PLUGGED_IN) )
	{
  	debugLED(1,HIGH);
	  LTC68042cell_nextVoltages(); //each call measures QTY3 cell voltages in the stack (round-robin)
	  debugLED(1,LOW);
	  
	  //---------------------------------------------------------------------------------------

	  uint8_t packVoltage_actual  = LTC68042result_stackVoltage_get();

	  uint8_t packVoltage_spoofed = (uint8_t)(packVoltage_actual*0.94); //t=20 microseconds

	  //---------------------------------------------------------------------------------------

	  if( key_getSampledState() == KEYON ) //key is on
	  {
	    METSCI_processLatestFrame();
	    //executes in ~t=5 microseconds when MCM is NOT sending data to LiBCM
	    //executes in  t=? microseconds when MCM is     sending data to LiBCM
	    
	    int16_t packCurrent_actual = adc_measureBatteryCurrent_amps(); //t=450 microseconds
	    int16_t packCurrent_spoofed;

			#if   defined(SET_CURRENT_HACK_60)
	    	packCurrent_spoofed = (int16_t)(packCurrent_actual * 0.62); //160% current hack = tell MCM 62% actual
			#elif defined(SET_CURRENT_HACK_40) 
				packCurrent_spoofed = (int16_t)(packCurrent_actual * 0.70); //140% current hack = tell MCM 70% actual
			#elif defined(SET_CURRENT_HACK_20)
				packCurrent_spoofed = (int16_t)(packCurrent_actual * 0.83); //120% current hack = tell MCM 83% actual
			#elif defined(SET_CURRENT_HACK_00)
				packCurrent_spoofed = packCurrent_actual;
			#else
				#error (SET_CURRENT_HACK_xx value not selected in config.c)
			#endif

			debugUSB_batteryCurrentSpoofed_amps(packCurrent_spoofed);

	    BATTSCI_setPackCurrent(packCurrent_spoofed);

	  	vPackSpoof_updateVoltage(packVoltage_actual, packVoltage_spoofed);

	  	BATTSCI_sendFrames();

	  }

	  debugUSB_printLatest_data();
	  lcd_refresh();
	}
	else
	{
	  //key is off & grid charger unplugged
		//JTS2doLater: Balance cells
	}

	blinkLED2(); //Heartbeat


	

	while( (millis() - previousMillis) < LOOP_RATE_MS )
	{
		; //nop until 
	}

	previousMillis = millis();
}