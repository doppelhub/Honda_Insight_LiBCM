//Code to verify upload works

#define PIN_FANOEM_LOW A1
#define PIN_FANOEM_HI A2
#define PIN_TURNOFFLiBCM A8
#define PIN_HMI_EN A9
#define PIN_LED1 A12
#define PIN_LED2 A13
#define PIN_LED3 A14
#define PIN_LED4 A15

#define PIN_GRID_SENSE 9
#define PIN_GRID_EN 10
#define PIN_FAN_PWM 11
#define PIN_KEY_ON 13

void setup()
{
  //Prevent LiBCM from turning off the 12V->5V DCDC
  pinMode(PIN_TURNOFFLiBCM,OUTPUT);
  digitalWrite(PIN_TURNOFFLiBCM,LOW);

  Serial.begin(115200);
  
  pinMode(PIN_LED1,OUTPUT);
  digitalWrite(PIN_LED1,HIGH);
  pinMode(PIN_LED2,OUTPUT);
  pinMode(PIN_LED3,OUTPUT);
  pinMode(PIN_LED4,OUTPUT);

  pinMode(PIN_HMI_EN,OUTPUT);

  pinMode(PIN_FAN_PWM,OUTPUT);
  analogWrite(PIN_FAN_PWM,254);
  pinMode(PIN_FANOEM_LOW,OUTPUT);
  digitalWrite(PIN_FANOEM_LOW,HIGH);


  Serial.println("upload tester v0.0.1\n\n");
  Serial.println("OEM fan should be on low speed");
  Serial.println("LiBCM fans should be on full speed");
}

void loop()
{
  Serial.print("\nToggling LED2 & screen ");
  digitalWrite(PIN_LED2,   !digitalRead(PIN_LED2  ));
  digitalWrite(PIN_HMI_EN, !digitalRead(PIN_HMI_EN));
  
  for(uint8_t ii=0; ii<=9; ii++)
  {
    delay(100);
    Serial.print( String(ii) );
    digitalWrite(PIN_LED3, !digitalRead(PIN_LED3));
  }
}
