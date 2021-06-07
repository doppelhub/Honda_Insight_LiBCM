// scratch space... not used right now

// void turnGridCharger_Off(void)
// {
//   analogWrite(GRIDPWM_PIN,255); //Set power to zero
//   delay(100);
//   digitalWrite(GRIDEN_PIN,LOW); //Turn charger off
//   Serial.println(F("Charger turned off"));
// }



// //check for full cell(s)   
//         uint16_t cellVoltage_highest = 0;   
//         for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
//           {
//             for (int i=0; i<12; i++)
//             {
//               if( cell_codes[current_ic][i] > cellVoltage_highest )
//               {
//                 cellVoltage_highest = cell_codes[current_ic][i];
//               }
//               if( cell_codes[current_ic][i] > 39000 ) //10 mV per count, e.g. "39000" = 3.900 volts
//               {
//                 Serial.println("Cell " + String(current_ic * 12 + i + 1) + " is full." );
//                 //turnGridCharger_Off();
//               }
//             }
//           }
//         Serial.println("Highest cell voltage is: " + String(cellVoltage_highest , DEC) );
//         delay(500);



// case 8: //JTS toggle grid charger
//     {
//       uint8_t gridEnabled      =   digitalRead(GRIDEN_PIN    );
//       uint8_t gridPowerPresent = !(digitalRead(GRIDSENSE_PIN));

//       Serial.println("GRIDEN   : " + String(gridEnabled     ) );
//       Serial.println("GRIDSENSE: " + String(gridPowerPresent) );
  
//       if( gridEnabled || !(gridPowerPresent) ) //charger is enabled, or unplugged
//       {
//         turnGridCharger_Off();
//       }
//       if( !(gridEnabled) && gridPowerPresent ) //charger off & plugged in 
//       {
//         digitalWrite(GRIDEN_PIN,HIGH); //Turn charger on
//         delay(100);
//         for(int ii=255; ii>=0; ii--)
//         {
//           analogWrite(GRIDPWM_PIN,ii);
//           delay(2);
//         }
//         Serial.println(F("Charger turned on"));
//       }
//       break;
//     }

//     case 9: //JTS Toggle Fans
//     {
//       Serial.println(F("Toggling Fans")); 
//       if( digitalRead(FAN_PWM_PIN) ) //fans are on
//       {
//         for(int ii=255; ii>=50; ii--)
//         {
//           analogWrite(FAN_PWM_PIN,ii);
//           delay(10);
//         }
//         analogWrite(FAN_PWM_PIN,0);
//         Serial.println(F("Fans turned off"));
//       } else { //fans are off
//         for(int ii=75; ii<=255; ii++)
//         {
//           analogWrite(FAN_PWM_PIN,ii);
//           delay(10);
//         }
//         Serial.println(F("Fans turned on"));
//       }
//       break;
//     }

//     default:
//       Serial.println(F("Incorrect Option"));
//       break;
//   }
// }

//   pinMode(GRIDPWM_PIN,  OUTPUT); //JTS Move this to init code
//   pinMode(GRIDSENSE_PIN, INPUT);
//   pinMode(GRIDEN_PIN,   OUTPUT);
//   pinMode(FAN_PWM_PIN,  OUTPUT);