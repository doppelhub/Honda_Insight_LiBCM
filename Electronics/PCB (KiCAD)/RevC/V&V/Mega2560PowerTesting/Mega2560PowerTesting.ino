#include <avr/power.h>
#include <avr/sleep.h>

void blinkLED5s(void)
{
  for(int ii=0; ii<50; ii++)
  {
    digitalWrite(13,HIGH);
    delay(100);
    digitalWrite(13,LOW);
    delay(100);
  }
}

void setup () 
{
  pinMode(13,OUTPUT);
  //set_sleep_mode(SLEEP_MODE_IDLE);  //82 mV
  //set_sleep_mode(SLEEP_MODE_PWR_SAVE);  //38 mV //timer2 can still run (but this makes oscillator stay on)
  //set_sleep_mode(SLEEP_MODE_IDLE);  //37 mV
  //set_sleep_mode(SLEEP_MODE_STANDBY); //35 mV //oscillator stays running
  //set_sleep_mode(SLEEP_MODE_PWR_DOWN); //35mV
  //sleep_enable();
  //sleep_cpu();

  ADCSRA = 0; //disable ADC
  power_all_disable ();

  //clock_prescale_set(clock_div_1);
}

void loop()
{
  digitalWrite(13,!digitalRead(13));
  delay(100);
}
