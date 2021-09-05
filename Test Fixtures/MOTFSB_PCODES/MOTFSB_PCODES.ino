//Uses an Arduino Uno to directly decode P-codes sent by MCM's MOTFSA(CLK) & MOTFSB(DATA) lines
//Sends decoded Pcodes to:
//  -USB Serial Monitor
//  -4x20 display (if connected)

#include "lcd_I2C.h"

//Must make the following connections
#define MOTFSA_CLK A2
#define MOTFSB_DAT A3

//If using 4x20 display, connect pins as follows
#define SDA_4X20 A4
#define CLK_4X20 A5 
//also connect power and ground

lcd_I2C_jts lcd2(0x27);

uint8_t  isDataLo(void){return !digitalRead(MOTFSB_DAT);}
uint8_t  isDataHi(void){return  digitalRead(MOTFSB_DAT);}
uint8_t isClockLo(void){return !digitalRead(MOTFSA_CLK);}
uint8_t isClockHi(void){return  digitalRead(MOTFSA_CLK);}

uint8_t getNextBit(void)
{
  //verify clock is low
  while( isClockHi() ) {;}
  //clock is now low
  
  //wait for clock to go high
  while( isClockLo() ) {;}
  //clock just went high
  delay(10);
  
  uint8_t dataStatus = isDataHi();
  return dataStatus;
}

void waitForDataHi(void)
{
  while( getNextBit() == 0 ){;}
}

void setup()
{
  lcd2.begin(20,4);
  lcd2.clear();
  delay(100);
  lcd2.display();
  lcd2.backlight();
  Serial.begin(115200);
  pinMode(MOTFSA_CLK, INPUT_PULLUP); //enable pullup
  pinMode(MOTFSB_DAT, INPUT_PULLUP); //enable pullup
}

void loop()
{
 //verify we're not in the middle of a transmission
  uint8_t successiveLowBits = 0;
  for(int ii=0; ii<7; ii++)
  {
    if(getNextBit() == 0) {successiveLowBits++;}
  }
    
  if(successiveLowBits == 7)
  {     
    waitForDataHi(); //wait for start bit
  
    uint8_t dataPacket = 0b00000000;
    
    for(int ii=7; ii>0; ii--) //get next 8 bits
    {
      dataPacket = (uint8_t)( dataPacket | (uint8_t)(getNextBit() << ii) );
    }
   
    char errorText[20];
    
    switch(dataPacket)
    {                                   //*******************  
      case 0b00100110: strcpy(errorText, "1438 MDM Temp      "); break;
      case 0b11000110: strcpy(errorText, "1439 Short Circuit "); break;
      case 0b01111110: strcpy(errorText, "1440 IGBT Wiring   "); break;
      case 0b10100110: strcpy(errorText, "1445 Bypass Relay  "); break;
      case 0b10010110: strcpy(errorText, "1448 Batt airflow  "); break;
      case 0b00010110: strcpy(errorText, "1449 Batt Overheat "); break;
      case 0b00111110: strcpy(errorText, "1565 Invalid Hall  "); break;
      case 0b01101110: strcpy(errorText, "1568 BCM Vsense    "); break;
      case 0b10111110: strcpy(errorText, "1572 PDU temp sig  "); break;
      case 0b11001110: strcpy(errorText, "1573 ???           "); break;
      case 0b11011110: strcpy(errorText, "1576 VPIN != VMCME "); break;
      case 0b11100110: strcpy(errorText, "1577 VPIN != V6804 "); break;
      case 0b01011110: strcpy(errorText, "1580 BCM I Offset  "); break;
      case 0b10011110: strcpy(errorText, "1581 phase wiring  "); break;
      case 0b01110110: strcpy(errorText, "1582 U Phase wiring"); break;
      case 0b10110110: strcpy(errorText, "1583 V Phase wiring"); break;
      case 0b00110110: strcpy(errorText, "1584 W Phase wiring"); break;
      case 0b11010110: strcpy(errorText, "1585 Sum(UVW) != 0A"); break;
      case 0b01010110: strcpy(errorText, "1586 I_BCM != I_MCM"); break;
      case 0b01000110: strcpy(errorText, "1635 BCM +-12V Bad "); break;
      case 0b00011110: strcpy(errorText, "1647 Bad ECM Sig   "); break;
      case 0b11101110: strcpy(errorText, "1648 BATTSCI/METSCI"); break;
      case 0b11110110: strcpy(errorText, "1649 ABS Signal    "); break;
      default:         strcpy(errorText, "?:"); break;
    }

    Serial.print("\nP" + String(errorText) );

    static uint8_t lastDataPacket = 0;
    
    if(dataPacket != lastDataPacket)
    {
      static uint8_t lcdRow = 0;
      lcd2.setCursor(0,lcdRow);
      lcd2.print('P' + String(errorText) );
      lcdRow++;
      if(lcdRow == 4) {lcdRow = 0;}
      
      if(errorText[0] == '?')
      {
        lcd2.print(String(dataPacket,BIN));
        Serial.print(String(dataPacket,BIN));
      }
    }


    lastDataPacket = dataPacket; 
  }

  lcd2.setCursor(19,3); //move to bottom right

  static uint8_t displayTick = 0;
  if(displayTick == 0)
  {
    displayTick = 1;
    lcd2.print('*');
  } else
  {
    displayTick = 0;
    lcd2.print('.');
  }
}
