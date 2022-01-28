#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup()
{
  pinMode(13,OUTPUT);
}

void loop()
{
  lcd.begin();
  delay(50);

  lcd.backlight();
  lcd.print(" Hello, world!");

  for(uint8_t ii=0; ii<20; ii++)
  {
    lcd.print('!');
    digitalWrite(13,!digitalRead(13));
    delay(100);
  }

  Wire.end();
  delay(50);
}
