//Copyright 2021(c) John Sullivan
//Tests PCB (using custom external hardware)

#include "libcm.h"

void serialUSB_waitForEmptyBuffer(void)
{	//This function verifies serial data is sent to host prior to starting each test (in case of brownout/reset)
	while(Serial.availableForWrite() != 63) { ; } //do nothing
}

//the 'quality' of this code is... poor.  Not written to be good/fast/etc... just to work
void bringupTester_run(void)
{
	#ifdef RUN_BRINGUP_TESTER
		while(1) //this function never returns
		{
			uint8_t testToRun = TEST_TYPE_UNDEFINED;
			bool didTestFail = false;

			if( gpio_isCoverInstalled() == true ) //wait for user to push button
			{
				Serial.print(F("\nRunning LiBCM Bringup Tester\n"));

				/////////////////////////////////////////////////////////////////////

				//test buzzer low frequency
				Serial.print(F("\nTesting Buzzer(low)"));
				serialUSB_waitForEmptyBuffer();
				gpio_turnBuzzer_on_lowFreq();
				delay(100);
				gpio_turnBuzzer_off();

				/////////////////////////////////////////////////////////////////////

				//verify LTC6804s working with no short circuits
				Serial.print(F("\nLTC6804 - verify isoSPI: "));
				serialUSB_waitForEmptyBuffer();
				
				LTC68042result_errorCount_set(0);

				//Give LTC6804s time to settle (due to high impedance test LED string)
				LTC68042configure_wakeupCore();
				delay(100);

				//start cell conversion, read back all cell voltages, validate, and store in array
				for(int ii=0; ii<100; ii++) { LTC68042cell_nextVoltages(); delay(10); } //generate a bunch of isoSPI traffic to check for errors

				Serial.print(F("\nLTC6804 - isoSPI error count: "));
				Serial.print(String(LTC68042result_errorCount_get()));
				Serial.print(F(", "));
			
				if( LTC68042result_errorCount_get() == 0 ) { Serial.print("pass"); }
				else { Serial.print("FAIL!!!!!!!!!!!!!"); didTestFail = true; } //at least one isoSPI error occurred

				//display all cell voltages
				for(uint8_t ii=0; ii<TOTAL_IC; ii++) { debugUSB_printOneICsCellVoltages( ii, 3); }

				Serial.print("\nmax cell: ");
				Serial.print(String(LTC68042result_hiCellVoltage_get()));
				Serial.print("\nmin cell: ");
				Serial.print(String(LTC68042result_loCellVoltage_get()));	
		
				Serial.print(F("\nLTC6804 - verify no shorts: "));
				serialUSB_waitForEmptyBuffer();
				//verify all cells are plugged in
				if( (LTC68042result_loCellVoltage_get() > 15000) && /* verify all cells above 1.5000 volts */
				    (LTC68042result_hiCellVoltage_get() - LTC68042result_loCellVoltage_get() < 1000) && /* verify cells reasonably balanced*/
				    (LTC68042result_hiCellVoltage_get() < 41000) ) //default returned data is 65535 (if no data sent)
				{
					Serial.print("pass");
					if( LTC68042result_hiCellVoltage_get() > 30000 ) { testToRun = TEST_TYPE_THERMAL_IMAGER; }
					else                                             { testToRun = TEST_TYPE_GAUNTLET;       }

				} else {
					Serial.print("FAIL!!!!!!!!!!!!!!!! (check BMS leads & supply)");
					didTestFail = true;
				}
			}

			/////////////////////////////////////////////////////////////////////

			if(testToRun == TEST_TYPE_GAUNTLET)
			{	
				//test LEDs/display

				Serial.print(F("\nTesting LED1/2/3/4"));
				serialUSB_waitForEmptyBuffer();
				LED(1,HIGH);
				LED(2,HIGH);
				LED(3,HIGH);
				LED(4,HIGH);

				Serial.print(F("\nTesting LiDisplay FET (Red LED)"));
				serialUSB_waitForEmptyBuffer();
				gpio_turnHMI_on();

				//Turn 4x20 screen on
				Serial.print(F("\nWriting Data to 4x20 LCD"));
				serialUSB_waitForEmptyBuffer();		
				lcd_initialize();
				lcd_printStaticText();
				lcd_displayON();

				//test if 4x20 screen dims when 5V buck turned off (USB powers at lower voltage)
				for(int ii=0; ii<5; ii++)
				{
					digitalWrite(PIN_TURNOFFLiBCM,HIGH); //4x20 screen should dim
					delay(200);
					digitalWrite(PIN_TURNOFFLiBCM,LOW); //4x20 screen full brightness
					delay(200);
				}

				//Turn LEDs off
				LED(1,LOW);
				LED(2,LOW);
				LED(3,LOW);
				LED(4,LOW);
				gpio_turnHMI_off();

				/////////////////////////////////////////////////////////////////////

				//Test MCMe isolated voltage divider
				Serial.print(F("\n\nTesting MCMe (LED should blink)"));
				serialUSB_waitForEmptyBuffer();

				for(int ii=0; ii<5; ii++)
				{
					spoofVoltageMCMe_setSpecificPWM(0); //LED should be full brightness
					delay(200);
					spoofVoltageMCMe_setSpecificPWM(255); //LED should be dim
					delay(200);
				}

				/////////////////////////////////////////////////////////////////////

				//test LiDisplay loopback
				Serial.print(F("\nLiDisplay loopback test: "));
				serialUSB_waitForEmptyBuffer();
				{
					uint8_t numberToLoopback = 0b10101110;
					LiDisplay_writeByte(numberToLoopback);
					delay(10);

					uint8_t numberLoopedBack = 0;
					while ( LiDisplay_bytesAvailableToRead() != 0 ) { numberLoopedBack = LiDisplay_readByte(); }
					if(numberLoopedBack == numberToLoopback) { Serial.print("pass"); }
					else                                     { Serial.print("FAIL!!!!!!!!!!!!!!!!!!!!!!!!"); didTestFail = true; }
				}

				/////////////////////////////////////////////////////////////////////

				//test BATTSCI & METSCI
				Serial.print(F("\nPowering up current sensor, RS485, & static 5V load"));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_turnPowerSensors_on();
					delay(20);
				}

				Serial.print(F("\nBATTSCI -> METSCI loopback test: "));
				serialUSB_waitForEmptyBuffer();
				{
					BATTSCI_enable();
					METSCI_enable();

					uint8_t numberToLoopback = 0b10101110;
					BATTSCI_writeByte(numberToLoopback);  //send data on BATTSCI (which is connected to METSCI)
					delay(10);

					uint8_t numberLoopedBack = 0;
					while ( METSCI_bytesAvailableToRead() != 0 ) { numberLoopedBack = METSCI_readByte(); }
					if(numberLoopedBack == numberToLoopback) { Serial.print("pass"); }
					else                                     { Serial.print("FAIL!!!!!!!!!!!!!!!!!!!!!!!!"); didTestFail = true; }

			    	BATTSCI_disable();
			   		METSCI_disable();
			    }

				/////////////////////////////////////////////////////////////////////

				Serial.print(F("\n\nMeasuring temp with sensors disabled (should be ~5V)"));
				serialUSB_waitForEmptyBuffer();

				gpio_turnTemperatureSensors_off();
				delay(250); //wait for temp sensor LPF

				Serial.print(F("\nYEL  temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_YEL)));
				Serial.print(F("\nGRN  temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_GRN)));
				Serial.print(F("\nWHT  temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_WHT)));
				Serial.print(F("\nBLU  temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_BLU)));
				Serial.print(F("\nBay1 temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_BAY1)));
				Serial.print(F("\nBay2 temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_BAY2)));
				Serial.print(F("\nBay3 temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_BAY3)));

				Serial.print(F("\n\nMeasuring temp with sensors enabled. Should be ~2.5V (~512 counts)"));
				serialUSB_waitForEmptyBuffer();
				gpio_turnTemperatureSensors_on();
				delay(250); //wait for temp sensor LPF
				
				Serial.print(F("\nYEL  temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_YEL)));
				Serial.print(F("\nGRN  temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_GRN)));
				Serial.print(F("\nWHT  temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_WHT)));
				Serial.print(F("\nBLU  temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_BLU)));
				Serial.print(F("\nBay1 temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_BAY1)));
				Serial.print(F("\nBay2 temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_BAY2)));
				Serial.print(F("\nBay3 temp is: ")); Serial.print(String(adc_getTemperature(PIN_TEMP_BAY3)));
				Serial.print('\n');

				//JTS2do: add pass/fail bounding 

				/////////////////////////////////////////////////////////////////////

				//test current sensor, both OEM fan speeds, and onboard fans

				//Lambda Gen is set to 2V@3A, but is open-drained thru QTY3 fan drive FETs (in parallel, tested one at a time)
				//JTS2do: Add bounding
				Serial.print(F("\n\nTesting current sensor @ 0A: "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_setFanSpeed_OEM('0'); 
					delay(100);
					uint16_t result = analogRead(PIN_BATTCURRENT); // 0A is 330 counts
					Serial.print(String(result));
					Serial.print(F(", "));

					gpio_setFanSpeed_OEM('L'); 
					delay(500);
					result = analogRead(PIN_BATTCURRENT); // 3A * 19 turns = '57 A' // 54
					Serial.print(String(result));
					Serial.print(F(", "));

					gpio_setFanSpeed_OEM('H'); //Lambda Gen is set to 3A, but is open-drained thru OEM fan drive FETs
					delay(500);
					result = analogRead(PIN_BATTCURRENT);
					Serial.print(String(result));
					Serial.print(F(", "));		

					gpio_setFanSpeed('H'); //Lambda Gen is set to 3A, but is open-drained thru onboard fan drive FETs
					delay(500);
					result = analogRead(PIN_BATTCURRENT);
					Serial.print(String(result));		

					//ensure all FETs are off (so they don't overheat)
					gpio_setFanSpeed('0');
					gpio_setFanSpeed_OEM('0');
				}

				//test that current sensor turns off
				gpio_turnPowerSensors_off();
				//JTS2do: verify ~2.5 volts with current sensor turned off

				/////////////////////////////////////////////////////////////////////

				//VPIN in & out loopback
				Serial.print(F("\n\nVPIN Loopback @ 0.0V: "));
				serialUSB_waitForEmptyBuffer();
				{
					analogWrite(PIN_VPIN_OUT_PWM,0); // 0 counts = 0.0 volts
					delay(200); //LPF
					uint16_t result = analogRead(PIN_VPIN_IN);
					Serial.print(String(result));
					if(result < 40) { Serial.print(F(" pass")); } //40 counts is 0.2 volts
					else { Serial.print(F("FAIL!!!!!!!!!!!!!!!!!!!!!!")); didTestFail = true; }

					analogWrite(PIN_VPIN_OUT_PWM,127); // 127 counts = 2.5 volts
					delay(200); //LPF
					result = analogRead(PIN_VPIN_IN);
					Serial.print(F("\nVPIN Loopback @ 2.5V: "));
					Serial.print(String(result));
					if((result < 532) && (result > 492)) { Serial.print(F(" pass")); } //492 counts is 2.4 volts //532 counts is 2.6 volts
					else { Serial.print(F("FAIL!!!!!!!!!!!!!!!!!!!!!!")); didTestFail = true; }

					analogWrite(PIN_VPIN_OUT_PWM,255); // 255 counts = 5.0 volts
					delay(200); //LPF
					result = analogRead(PIN_VPIN_IN);
					Serial.print(F("\nVPIN Loopback @ 5.0V: "));
					Serial.print(String(result));
					if(result > 984) { Serial.print(F(" pass")); } //984 counts is 4.8 volts
					else { Serial.print(F("FAIL!!!!!!!!!!!!!!!!!!!!!!")); didTestFail = true; }

				}

				/////////////////////////////////////////////////////////////////////

				//Test Grid Charger IGBT & sense
				Serial.print(F("\n\nTesting if grid IGBT is off: "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_turnGridCharger_off(); //turn IGBT off (power applied to grid charger side)
					delay(25);
					if( gpio_isGridChargerPluggedInNow() == false ) { Serial.print("pass"); }
					else { Serial.print("FAIL!!!!!!!!!!!!!!!!"); didTestFail = true; }
				}

				Serial.print(F("\nTesting if grid IGBT is on:  "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_turnGridCharger_on(); //turn IGBT on... grid sense should now see power
					delay(25);
					if( gpio_isGridChargerPluggedInNow() == true ) { Serial.print("pass"); }
					else { Serial.print("FAIL!!!!!!!!!!!!!!!!"); didTestFail = true; }
				}

				gpio_turnGridCharger_off(); //JTS2do: Verify state

				/////////////////////////////////////////////////////////////////////

				//Test GRID_PWM & 12V_KEYON
				//GRID_PWM is open-collectored to 12V_KEYON
				//12V_KEYON is pulled up to onboard +12V
				Serial.print(F("\nTesting 12V_KEYON state is ON: "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_setGridCharger_powerLevel('H'); //open drain is left floating (negative logic)
					delay(100);
					if( gpio_keyStateNow() == true ) { Serial.print("pass"); } //key appears on
					else { Serial.print("FAIL!!!!!!!!!!"); didTestFail = true; }
				}

				Serial.print(F("\nTesting GRID_PWM: "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_setGridCharger_powerLevel('0'); //open drain is shorted (negative logic)
					delay(100);
					if( gpio_keyStateNow() == false ) { Serial.print("pass"); } //GRID_PWM's open collector is pulling to 0V
					else { Serial.print("FAIL!!!!!!!!!!"); didTestFail = true; }
				}
				//Don't add any code here
				Serial.print(F("\nTesting 12V_KEYON state is OFF: "));
				serialUSB_waitForEmptyBuffer();
				{
					if( gpio_keyStateNow() == false ) { Serial.print("pass"); } //key appears off
					else { Serial.print("FAIL!!!!!!!!!!"); didTestFail = true; }
				}

				gpio_setGridCharger_powerLevel('H'); //turn GRID_PWM off (negative logic)

				/////////////////////////////////////////////////////////////////////

				//test buzzer high frequency
				gpio_turnBuzzer_on_highFreq();

				if( didTestFail == false) { Serial.print("\n\nUnit PASSED!\n\n"); delay(100); }
				else {	          Serial.print("\n\nUnit FAILED!!!!!!!!!!!\n\n"); delay(999); }
				gpio_turnBuzzer_off();

			}

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			if(testToRun == TEST_TYPE_THERMAL_IMAGER)
			{
				//(manually) verify discharge FETs working (with thermal imager)

				//JTS note: The LTC6804 cannot enable discharge resistors with high impedance spoofed BMS voltage (i.e. from series LED chain).
				//This is due to -2.5V PFET gate threshold voltage & also LTC6804 "Discharge Switch On-Resistance vs Cell Voltage" (p14).
				//Therefore, to test discharge resistors, run test again with actual batteries plugged in, then use thermal imager.
				
				LTC68042configure_wakeupCore();
				delay(1);

				for(uint8_t ii=0; ii<3; ii++)
				{
					gpio_turnBuzzer_on_highFreq();
					delay(100);
					gpio_turnBuzzer_off();
					delay(100);
				}

				//activate LTC6804 discharge FETs
				for(uint8_t ii=0; ii<TOTAL_IC; ii++)
				{
					gpio_turnBuzzer_on_highFreq();
					delay(50);
					gpio_turnBuzzer_off();

					uint16_t cellDischargeBitmap = 0b0000010101010101; //discharge cells 1/3/5/7/9/11
					LTC68042configure_cellBalancing_setCells( (FIRST_IC_ADDR + ii), cellDischargeBitmap);
					delay(1800); //wait for visual inspection

					LTC68042configure_cellBalancing_disable(); //disable all discharge FETs
					delay(1800); //wait for cool down

					cellDischargeBitmap = 0b0000101010101010; //discharge cells 2/4/6/8/10/12
					LTC68042configure_cellBalancing_setCells( (FIRST_IC_ADDR + ii), cellDischargeBitmap);
					delay(1800); //wait for visual inspection

					LTC68042configure_cellBalancing_disable(); //disable all discharge FETs
					delay(1800); //wait for cool down
				}
			}
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			blinkLED2(); //Heartbeat
		}
	#endif
}