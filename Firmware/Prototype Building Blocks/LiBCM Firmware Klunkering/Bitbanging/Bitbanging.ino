//#include "METSCI.h"
//#include "BATTSCI.h"

#define BattCurrent_Pin A0
#define FanOEMlow_Pin A1
#define FanOEMhigh_Pin A2
#define Temp_YEL_Pin A3
#define Temp_GRN_Pin A4
#define Temp_WHT_Pin A5
#define Temp_BLU_Pin A6
#define VPIN_IN_Pin A7
#define TurnOffLiBCM_Pin A8
#define DEBUG_FET_PIN A9
#define DEBUG_IO1_PIN A10
#define DEBUG_IO2_PIN A11
#define LED1_Pin A12
#define LED2_Pin A13
#define LED3_Pin A14
#define LED4_Pin A15

#define BATTSCIdir_Pin 2
#define METSCI_DIR_PIN 3
#define VPIN_OUT_PIN 4
#define SPI_EXT_CS_PIN 5 
#define TEMP_EN_PIN 6
#define ConnE_PWM_Pin 7
#define GridPWM_Pin 8
#define GridSense_Pin 9
#define GridEn_Pin 10
#define FanOnPWM_Pin 11
#define Load5v_Pin 12
#define KEY_ON_PIN 13

//Serial3
#define HLINE_TX_PIN 14
#define HLINE_RX_PIN 15

//Serial2
#define METSCI_TX_PIN 16
#define METSCI_RX_PIN 17

//Serial1
#define BATTSCI_TX_PIN 18
#define BATTSCI_RX_PIN 19


#define DEBUG_SDA_PIN 20
#define DEBUG_CLK_PIN 21


uint16_t ADCresult=0;

void setup() {
  // put your setup code here, to run once:
pinMode(LED1_Pin,OUTPUT);
pinMode(LED2_Pin,OUTPUT);
pinMode(LED3_Pin,OUTPUT);
pinMode(LED4_Pin,OUTPUT);
pinMode(TurnOffLiBCM_Pin,OUTPUT);
pinMode(ConnE_PWM_Pin,OUTPUT);
pinMode(FanOnPWM_Pin,OUTPUT);
pinMode(Load5v_Pin,OUTPUT);
pinMode(FanOEMlow_Pin,OUTPUT);
pinMode(FanOEMhigh_Pin,OUTPUT);
pinMode(METSCI_DIR_PIN,OUTPUT);
pinMode(BATTSCIdir_Pin,OUTPUT);
pinMode(GridEn_Pin,OUTPUT);
pinMode(VPIN_OUT_PIN,OUTPUT);

digitalWrite(LED1_Pin,HIGH);
digitalWrite(LED2_Pin,LOW);
digitalWrite(TurnOffLiBCM_Pin,LOW);
digitalWrite(ConnE_PWM_Pin,LOW);
digitalWrite(FanOnPWM_Pin,LOW);
digitalWrite(Load5v_Pin,HIGH);
digitalWrite(FanOEMlow_Pin,LOW);
digitalWrite(FanOEMhigh_Pin,LOW);
digitalWrite(METSCI_DIR_PIN,LOW); // METSCI Set LO to receive Data. Must be low when key OFF (to prevent backdriving MCM)
digitalWrite(BATTSCIdir_Pin,HIGH); //BATTSCI Set HI to send    Data. Must be low when key OFF (to prevent backdriving MCM)
digitalWrite(GridEn_Pin,LOW);
digitalWrite(VPIN_OUT_PIN,LOW);
digitalWrite(TurnOffLiBCM_Pin,LOW);

//Note: Changing this value messes with delay timing!
//TCCR0B = (TCCR0B & B11111000) | B00000001; // for PWM frequency of 62500 Hz D04 & D13
//TCCR0B = (TCCR0B & B11111000) | B00000010; // for PWM frequency of  7813 Hz D04 & D13
//TCCR0B = (TCCR0B & B11111000) | B00000011; // for PWM frequency of   977 Hz D04 & D13 (DEFAULT)

TCCR1B = (TCCR1B & B11111000) | B00000001; // for PWM frequency of 31372 Hz D11 & D12
//TCCR2B = (TCCR2B & B11111000) | B00000001; // for PWM frequency of 31372 Hz D09 & D10
//TCCR3B = (TCCR3B & B11111000) | B00000001; // for PWM frequency of 31372 Hz D02 & D03 & D05

//TCCR4B = (TCCR4B & B11111000) | B00000001; // for PWM frequency of 31372 Hz D06 & D07 & D08
TCCR4B = (TCCR4B & B11111000) | B00000010; // for PWM frequency of  3921 Hz D06 & D07 & D08
//TCCR4B = (TCCR4B & B11111000) | B00000011; // for PWM frequency of   490 Hz D06 & D07 & D08
//TCCR4B = (TCCR4B & B11111000) | B00000101; // for PWM frequency of    31 Hz D06 & D07 & D08

analogReference(EXTERNAL); //use 5V AREF pin

Serial.begin(115200);
Serial1.begin(9600,SERIAL_8E1); //BATTSCI
Serial2.begin(9600,SERIAL_8E1); //METSCI

}

void loop()
{
  ADCresult = analogRead(BattCurrent_Pin);
  Serial.print("\nFirst RAW 10b ADC Value is: " + String(ADCresult) );
  
  for(int ii=0; ii<63; ii++)
  { 
    ADCresult += analogRead(BattCurrent_Pin);
  }
  Serial.print(" | 64x ADC result is: " + String(ADCresult>>6) );      
}
