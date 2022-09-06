//Copyright 2021-2022(c) John Sullivan
//Tests PCB (using custom external hardware)

//Not used when deployed in vehicle

#include "libcm.h"

void serialUSB_waitForEmptyBuffer(void)
{	//This function verifies serial data is sent to host prior to starting each test (in case of brownout/reset)
	while(Serial.availableForWrite() != 63) { ; } //do nothing
}

//the 'quality' of this code is... poor.  Not written to be good/fast/etc... just to test each PCB after assembly
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

				//verify LTC6804s
				Serial.print(F("\nTesting LTC6804s"));
				serialUSB_waitForEmptyBuffer();

				LTC68042result_errorCount_set(0);

				//Give LTC6804s time to settle (due to high impedance test LED string)
				LTC68042configure_wakeup();
				delay(50);

				//start cell conversion, read back all cell voltages, validate, and store in array
				for(int ii=0; ii<100; ii++) { LTC68042cell_nextVoltages(); delay(5); } //generate a bunch of isoSPI traffic to check for errors

				Serial.print(F("\nLTC6804 - isoSPI error count is "));
				Serial.print(String(LTC68042result_errorCount_get()));
				Serial.print(F(": "));

				if( LTC68042result_errorCount_get() == 0 ) { Serial.print(F("pass")); }
				else { Serial.print(F("FAIL!! !! !! !! !")); didTestFail = true; } //at least one isoSPI error occurred

				//display all cell voltages
				for(uint8_t ii=0; ii<TOTAL_IC; ii++) { debugUSB_printOneICsCellVoltages( ii, 3); }

				Serial.print(F("\nmax cell: "));
				Serial.print(String(LTC68042result_hiCellVoltage_get()));
				Serial.print(F("\nmin cell: "));
				Serial.print(String(LTC68042result_loCellVoltage_get()));

				Serial.print(F("\n\nLTC6804 - verify no shorts: "));
				serialUSB_waitForEmptyBuffer();
				//verify all cells are in range
				if( (LTC68042result_loCellVoltage_get() > 15000) && /* verify all cells above 1.5000 volts */
				    (LTC68042result_hiCellVoltage_get() - LTC68042result_loCellVoltage_get() < 1000) && /* verify cells reasonably balanced*/
				    (LTC68042result_hiCellVoltage_get() < 41000) ) //default returned data is 65535 (if no data sent)
				{
					Serial.print(F("pass"));
					if( LTC68042result_hiCellVoltage_get() > 30000 ) { testToRun = TEST_TYPE_THERMAL_IMAGER; } //lithium batteries connected
					else                                             { testToRun = TEST_TYPE_GAUNTLET;       } //LED BMS board connected

				} else {
					Serial.print(F("FAIL!! !! !! !! !! ! (check BMS leads & supply)"));
					didTestFail = true;
				}
			}

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			if(testToRun == TEST_TYPE_GAUNTLET)
			{
				/////////////////////////////////////////////////////////////////////

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
				lcd_begin();
				lcd_printStaticText();
				lcd_displayOn();

				//test if 4x20 screen dims when 5V buck turned off (USB powers at lower voltage)
				for(int ii=0; ii<6; ii++)
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

				for(int ii=0; ii<6; ii++)
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
					if(numberLoopedBack == numberToLoopback) { Serial.print(F("pass")); }
					else                                     { Serial.print(F("FAIL!! !! !! !! !! !! !! !! ")); didTestFail = true; }
				}

				/////////////////////////////////////////////////////////////////////

				//test BATTSCI & METSCI
				Serial.print(F("\nPowering up current sensor, RS485, & static 5V load"));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_turnPowerSensors_on();
					delay(10);
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
					if(numberLoopedBack == numberToLoopback) { Serial.print(F("pass")); }
					else                                     { Serial.print(F("FAIL!! !! !! !! !! !! !! !! ")); didTestFail = true; }

			    	BATTSCI_disable();
			   		METSCI_disable();
			    }

				/////////////////////////////////////////////////////////////////////

				//measure unpowered temperature sensors

				gpio_turnTemperatureSensors_off(); //they should already be off

				//OEM sensors
				{
					uint16_t tempYEL = adc_getTemperature(PIN_TEMP_YEL);
					uint16_t tempGRN = adc_getTemperature(PIN_TEMP_GRN);
					uint16_t tempWHT = adc_getTemperature(PIN_TEMP_WHT);
					uint16_t tempBLU = adc_getTemperature(PIN_TEMP_BLU);
					Serial.print(F("\nUnpowered YEL/GRN/WHT/BLU temp sensors are "));
					Serial.print( String(tempYEL) + '/');
					Serial.print( String(tempGRN) + '/');
					Serial.print( String(tempWHT) + '/');
					Serial.print( String(tempBLU) + ": ");

					if((tempYEL > 950) && (tempGRN > 950) && (tempWHT > 950) && (tempBLU > 950)) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !! !! !!")); didTestFail=true; }
				}

				//lithium module sensors
				{
					uint16_t tempBAY1 = adc_getTemperature(PIN_TEMP_BAY1);
					uint16_t tempBAY2 = adc_getTemperature(PIN_TEMP_BAY2);
					uint16_t tempBAY3 = adc_getTemperature(PIN_TEMP_BAY3);
					Serial.print(F("\nUnpowered BAY1/BAY2/BAY3 temp sensors are "));
					Serial.print( String(tempBAY1) + '/');
					Serial.print( String(tempBAY2) + '/');
					Serial.print( String(tempBAY3) + ": ");

					if((tempBAY1 > 950) && (tempBAY2 > 950) && (tempBAY3 > 950)) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !! !! !!")); didTestFail=true; }
				}

				/////////////////////////////////////////////////////////////////////

				//measure powered temperature sensors
				Serial.print(F("\n\nTurning temp sensors on"));
				serialUSB_waitForEmptyBuffer();

				gpio_turnTemperatureSensors_on();
				delay(500); //wait for temp sensor LPF

				//OEM sensors
				{
					uint16_t tempYEL = adc_getTemperature(PIN_TEMP_YEL);
					uint16_t tempGRN = adc_getTemperature(PIN_TEMP_GRN);
					uint16_t tempWHT = adc_getTemperature(PIN_TEMP_WHT);
					uint16_t tempBLU = adc_getTemperature(PIN_TEMP_BLU);
					Serial.print(F("\n  Powered YEL/GRN/WHT/BLU temp sensors are "));
					Serial.print( String(tempYEL) + '/');
					Serial.print( String(tempGRN) + '/');
					Serial.print( String(tempWHT) + '/');
					Serial.print( String(tempBLU) + ": ");

					if((tempYEL > 462) && (tempYEL < 562) &&
					   (tempGRN > 462) && (tempGRN < 562) &&
					   (tempWHT > 462) && (tempWHT < 562) &&
					   (tempBLU > 462) && (tempBLU < 562)) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !! !! !!")); didTestFail=true; }
				}

				//lithium module sensors
				{
					uint16_t tempBAY1 = adc_getTemperature(PIN_TEMP_BAY1);
					uint16_t tempBAY2 = adc_getTemperature(PIN_TEMP_BAY2);
					uint16_t tempBAY3 = adc_getTemperature(PIN_TEMP_BAY3);
					Serial.print(F("\n  Powered BAY1/BAY2/BAY3 temp sensors are "));
					Serial.print( String(tempBAY1) + '/');
					Serial.print( String(tempBAY2) + '/');
					Serial.print( String(tempBAY3) + ": ");

					if((tempBAY1 > 462) && (tempBAY1 < 562) &&
					   (tempBAY2 > 462) && (tempBAY2 < 562) &&
					   (tempBAY3 > 462) && (tempBAY3 < 562)) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !! !! !!")); didTestFail=true; }
				}

				gpio_turnTemperatureSensors_off();

				/////////////////////////////////////////////////////////////////////

				//test current sensor, both OEM fan speeds, and onboard fans
				//Lambda Gen is set to 2V@3A, and is open-drained thru QTY3 fan drive FETs (in parallel, tested one at a time)
				//current sensor has QTY19 turns, so 3A is seen as 57 amps assist.

				Serial.print(F("\n\nCurrent sensor 10b result @ 0A is "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_setFanSpeed_OEM('0'); //turn all FETs off (0A thru sensor)
					gpio_setFanSpeed_PCB('0');     //turn all FETs off (0A thru sensor)
					delay(10);

					uint16_t resultADC = analogRead(PIN_BATTCURRENT); // 0A is 330 counts
					Serial.print(String(resultADC));
					Serial.print(F(" counts: "));
					if((resultADC > 328) && (resultADC < 336)) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !! !! !!")); didTestFail=true; }
				}

				Serial.print(F("\nCurrent sensor 10b result @ 3A through OEM_FAN_L is "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_setFanSpeed_OEM('L'); //test OEM fan low speed relay
					gpio_setFanSpeed_PCB('0');
					delay(500);

					uint16_t resultADC = analogRead(PIN_BATTCURRENT); // 3A * 19 turns = '57 A' = 595 counts
					Serial.print(String(resultADC));
					Serial.print(F(" counts: "));
					if((resultADC > 585) && (resultADC < 605)) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !! !! !!")); didTestFail=true; }
				}

				Serial.print(F("\nCurrent sensor 10b result @ 3A through OEM_FAN_H is "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_setFanSpeed_OEM('H'); //test OEM fan low speed relay
					gpio_setFanSpeed_PCB('0');
					delay(500);

					uint16_t resultADC = analogRead(PIN_BATTCURRENT); // 3A * 19 turns = '57 A' = 595 counts
					Serial.print(String(resultADC));
					Serial.print(F(" counts: "));
					if((resultADC > 585) && (resultADC < 605)) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !! !! !!")); didTestFail=true; }
				}

				Serial.print(F("\nCurrent sensor 10b result @ 3A through onboard fans is "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_setFanSpeed_OEM('0'); //test OEM fan low speed relay
					gpio_setFanSpeed_PCB('H');
					delay(500);

					uint16_t resultADC = analogRead(PIN_BATTCURRENT); // 3A * 19 turns = '57 A' = 595 counts
					Serial.print(String(resultADC));
					Serial.print(F(" counts: "));
					if((resultADC > 585) && (resultADC < 605)) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !! !! !!")); didTestFail=true; }
				}

				//test that OEM current sensor turns off correctly
				Serial.print(F("\nTurning current sensor off: "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_turnPowerSensors_off();
					delay(500);

					gpio_setFanSpeed_PCB('H'); //should already be here
					delay(100);

					uint16_t resultADC = analogRead(PIN_BATTCURRENT); // 0A is 330 counts
					if((resultADC > 328) && (resultADC < 336)) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !! !! !!")); didTestFail=true; }
				}

				//turn all FETs off (so they don't overheat)
				gpio_setFanSpeed_PCB('0');
				gpio_setFanSpeed_OEM('0');

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
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !")); didTestFail = true; }

					analogWrite(PIN_VPIN_OUT_PWM,127); // 127 counts = 2.5 volts
					delay(200); //LPF
					result = analogRead(PIN_VPIN_IN);
					Serial.print(F("\nVPIN Loopback @ 2.5V: "));
					Serial.print(String(result));
					if((result < 532) && (result > 492)) { Serial.print(F(" pass")); } //492 counts is 2.4 volts //532 counts is 2.6 volts
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !")); didTestFail = true; }

					analogWrite(PIN_VPIN_OUT_PWM,255); // 255 counts = 5.0 volts
					delay(200); //LPF
					result = analogRead(PIN_VPIN_IN);
					Serial.print(F("\nVPIN Loopback @ 5.0V: "));
					Serial.print(String(result));
					if(result > 984) { Serial.print(F(" pass")); } //984 counts is 4.8 volts
					else { Serial.print(F("FAIL!! !! !! !! !! !! !! !")); didTestFail = true; }

				}

				/////////////////////////////////////////////////////////////////////

				//Test Grid Charger IGBT & GRID_SENSE
				Serial.print(F("\n\nTesting if grid IGBT is off: "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_turnGridCharger_off(); //turn IGBT off (power applied to grid charger side)
					delay(25);
					if( gpio_isGridChargerPluggedInNow() == false ) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !")); didTestFail = true; }
				}

				Serial.print(F("\nTesting if grid IGBT is on:  "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_turnGridCharger_on(); //turn IGBT on... grid sense should now see power
					delay(25);
					if( gpio_isGridChargerPluggedInNow() == true ) { Serial.print(F("pass")); }
					else { Serial.print(F("FAIL!! !! !! !! !! !")); didTestFail = true; }
				}

				gpio_turnGridCharger_off();

				/////////////////////////////////////////////////////////////////////

				//Test GRID_PWM & 12V_KEYON
				//GRID_PWM is open-collectored to 12V_KEYON
				//12V_KEYON is pulled up to onboard +12V
				Serial.print(F("\nTesting 12V_KEYON state is ON: "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_setGridCharger_powerLevel('H'); //open drain is left floating (negative logic)
					delay(100);
					if( gpio_keyStateNow() == true ) { Serial.print(F("pass")); } //key appears on
					else { Serial.print(F("FAIL!! !! !! !")); didTestFail = true; }
				}

				Serial.print(F("\nTesting GRID_PWM: "));
				serialUSB_waitForEmptyBuffer();
				{
					gpio_setGridCharger_powerLevel('0'); //open drain is shorted (negative logic)
					delay(100);
					if( gpio_keyStateNow() == false ) { Serial.print(F("pass")); } //GRID_PWM's open collector is pulling to 0V
					else { Serial.print(F("FAIL!! !! !! !")); didTestFail = true; }
				}
				//Don't add any code here
				Serial.print(F("\nTesting 12V_KEYON state is OFF: "));
				serialUSB_waitForEmptyBuffer();
				{
					if( gpio_keyStateNow() == false ) { Serial.print(F("pass")); } //key appears off
					else { Serial.print(F("FAIL!! !! !! !")); didTestFail = true; }
				}

				gpio_setGridCharger_powerLevel('H'); //turn GRID_PWM off (negative logic)

				/////////////////////////////////////////////////////////////////////

				//test buzzer high frequency
				gpio_turnBuzzer_on_highFreq();

				if( didTestFail == false) { Serial.print(F("\n\nUnit PASSED!\n\n")); delay(100); }
				else {	          Serial.print(F("\n\nUnit FAILED!! !! !! !!\n\n")); delay(999); }
				gpio_turnBuzzer_off();

			}

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			if(testToRun == TEST_TYPE_THERMAL_IMAGER)
			{
				//(manually) verify discharge FETs working (with thermal imager)

				//JTS note: The LTC6804 cannot enable discharge resistors with high impedance spoofed BMS voltage (i.e. from series LED chain).
				//This is due to -2.5V PFET gate threshold voltage & also LTC6804 "Discharge Switch On-Resistance vs Cell Voltage" (p14).
				//Therefore, to test discharge resistors, run test again with actual batteries plugged in, then use thermal imager.

				//JTS2doLater: Solder together a 75 Ohm test board - similar to existing LED test board - so that the above is no longer an issue.

				LTC68042configure_wakeup();
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
					LTC68042configure_setBalanceResistors( (FIRST_IC_ADDR + ii), cellDischargeBitmap, LTC6804_DISCHARGE_TIMEOUT_02_SECONDS);
					delay(1800); //wait for visual inspection

					LTC68042configure_programVolatileDefaults(); //disables all discharge FETs
					delay(1800); //wait for cool down

					cellDischargeBitmap = 0b0000101010101010; //discharge cells 2/4/6/8/10/12
					LTC68042configure_setBalanceResistors( (FIRST_IC_ADDR + ii), cellDischargeBitmap, LTC6804_DISCHARGE_TIMEOUT_02_SECONDS);
					delay(1800); //wait for visual inspection

					LTC68042configure_programVolatileDefaults(); //disables all discharge FETs
					delay(1800); //wait for cool down
				}
			}
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			blinkLED2(); //Heartbeat
		}
	#endif
}
