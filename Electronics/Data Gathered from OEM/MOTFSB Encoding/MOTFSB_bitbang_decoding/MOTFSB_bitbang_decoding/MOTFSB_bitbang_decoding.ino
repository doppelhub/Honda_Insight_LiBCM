int MOTFSApin=12;
int MOTFSBpin=13;

//known good
//char frameorder[]={"bababababababababdbdbcbc"};

//test case: is babababa repeating acceptable input?
//char frameorder[]={"ba"};
//no CEL

//test case: add an 'E' frame
//char frameorder[]={"bababababababababebebcbc"};
//causes P1648 error (can't flash SCS codes because the MCM isn't connected).

//test case: add an 'E' frame in different spot
//char frameorder[]={"bababababababababdbdbebe"};
//same error (P1648).  Hence, error order doesn't appear to matter.

//can we broadcast just bdbdbcbc?
//char frameorder[]={"bdbdbcbc"};
//no CEL

//can we broadcast just bd?
//char frameorder[]={"bd"};
//no CEL

//can we broadcast just bc?
//char frameorder[]={"bc"};
//no CEL
//Thus, we can repeatedly broadcast any of the following without error: "bd" or "ba" or "bc"

//What if we repeatedly broadcast 'aaaaaaa'?
//char frameorder[]={"a"};
//P1645

//What about a made up sequence with previously good signals?
//char frameorder[]={"bababcbcbdbd"};
//no CEL

//What if we only broadcast each frame one time?
//char frameorder[]={"babcbd"};
//P1645... probably "data format" error.  Thus, we have to broadcast each packet twice

//What if we only broadcast an error every once in a while (note 'bebe' on the end)?
//char frameorder[]={"bababababababababababababababababababababababababababababababababebe"};
//Still get a P1648

//What if we wait longer between frames?
//char frameorder[]={"bba"};
//no CEL.  Thus, 'b' frame (all 0s) length isn't that important, as long as less than 5 seconds to send pattern twice.

//What if arbitrary # b-frames?:
//char frameorder[]={"babbabbbabbbba"};
//no CEL. Thus, 'b' frame (all 0s) length isn't that important, as long as less than 5 seconds to send pattern twice. 

//Let's make up a new frame 'g'=0b1111111100:
//char frameorder[]={"bababgbg"};
//no CEL

//Let's make up a new frame 'g'=0b1111111000:
//char frameorder[]={"bababgbg"};
//no CEL

//Let's make up a new frame 'g'=0b1111110000:
//char frameorder[]={"bababgbg"};
//no CEL

//Let's make up a new frame 'g'=0b1111100000:
//char frameorder[]={"bababgbg"};
//no CEL

//Let's make up a new frame 'g'=0b1111000000:
//char frameorder[]={"bababgbg"};
//no CEL

//Let's make up a new frame 'g'=0b1110000000:
//char frameorder[]={"bababgbg"};
//no CEL

//Let's make up a new frame 'g'=0b1100000000:
//char frameorder[]={"bababgbg"};
//no CEL

//Let's make up a new frame 'g'=0b1000000000:
//char frameorder[]={"bababgbg"};
//no CEL
//wonder if P codes are more than a single frame.  Earlier we got 3 P codes with just two 10 bit patterns ('E' & 'F').
//I suspect the ECU has a mask and is only looking for predefined IMA codes.

//Let's only use our new frame 'g'=0b1000000000:
//char frameorder[]={"bg"};
//no CEL

//What if we only broadcast '0' all the time? (24Hz clk still going):
//char frameorder[]={"b"};
//P1645

//Let's make up a new frame 'g'=0b1011001100 (loosely based on 'E'):
//char frameorder[]={"bg"};
//no CEL

//Let's make up a new frame 'g'=0b1111001100 (loosely based on 'E'):
//char frameorder[]={"bg"};
//P1577

//Let's make up a new frame 'g'=0b1011001000 (loosely based on above P1577):
//char frameorder[]={"bg"};
//no CEL

//Let's make up a new frame 'g'=0b1111000100 (loosely based on above P1577):
//char frameorder[]={"bg"};
//no CEL

//Let's make up a new frame 'g'=0b1110001100 (loosely based on above P1577):
//char frameorder[]={"bg"};
//P1439

//Let's make up a new frame 'g'=0b1100001100 (loosely based on above P1577):
//char frameorder[]={"bg"};
//no CEL

//Let's make up a new frame 'g'=0b1101001100
//char frameorder[]={"bg"};
//P1445

//Let's just start at 0 and work up to 255.  I think there's a start bit (data goes high), followed by 8 bits of data.
//'g'=0b1 00000 000 no CEL
//'g'=0b1 00000 001 no CEL
//'g'=0b1 00000 010 no CEL
//'g'=0b1 00000 011 no CEL
//'g'=0b1 00000 100 no CEL
//'g'=0b1 00000 101 no CEL
//'g'=0b1 00000 110 no CEL
//'g'=0b1 00000 111 no CEL
//'g'=0b1 00001 000 no CEL
//'g'=0b1 00001 001 no CEL
//'g'=0b1 00001 010 no CEL
//'g'=0b1 00001 011 no CEL
//'g'=0b1 00001 100 no CEL
//'g'=0b1 00001 101 no CEL
//'g'=0b1 00001 110 no CEL
//'g'=0b1 00001 111 no CEL
//'g'=0b1 00010 000 no CEL
//'g'=0b1 00010 001 no CEL
//'g'=0b1 00010 010 no CEL
//'g'=0b1 00010 011 no CEL
//'g'=0b1 00010 100 no CEL
//'g'=0b1 00010 101 no CEL
//'g'=0b1 00010 110 P1449
//'g'=0b1 00010 111 no CEL
//'g'=0b1 00011 000 no CEL
//'g'=0b1 00011 001 no CEL
//'g'=0b1 00011 010 no CEL
//'g'=0b1 00011 011 no CEL
//'g'=0b1 00011 100 no CEL
//'g'=0b1 00011 101 no CEL
//'g'=0b1 00011 110 P1647
//'g'=0b1 00011 111 no CEL
//'g'=0b1 00100 000 no CEL
//'g'=0b1 00100 001 no CEL
//'g'=0b1 00100 010 no CEL
//'g'=0b1 00100 011 no CEL
//'g'=0b1 00100 100 no CEL
//'g'=0b1 00100 101 no CEL
//'g'=0b1 00100 110 P1438
//'g'=0b1 00100 111 no CEL
//'g'=0b1 00101 000 no CEL
//'g'=0b1 00101 001 no CEL
//'g'=0b1 00101 010 no CEL
//'g'=0b1 00101 011 no CEL
//'g'=0b1 00101 100 no CEL
//'g'=0b1 00101 101 no CEL
//'g'=0b1 00101 110 no CEL
//'g'=0b1 00101 111 no CEL
//'g'=0b1 00110 000 no CEL
//'g'=0b1 00110 001 no CEL
//'g'=0b1 00110 010 no CEL
//'g'=0b1 00110 011 no CEL
//'g'=0b1 00110 100 no CEL
//'g'=0b1 00110 101 no CEL
//'g'=0b1 00110 110 P1584
//'g'=0b1 00110 111 no CEL
//'g'=0b1 00111 000 no CEL
//'g'=0b1 00111 001 no CEL
//'g'=0b1 00111 010 no CEL
//'g'=0b1 00111 011 no CEL
//'g'=0b1 00111 100 no CEL
//'g'=0b1 00111 101 no CEL
//'g'=0b1 00111 110 P1565
//'g'=0b1 00111 111 no CEL
//'g'=0b1 01000 000 no CEL
//'g'=0b1 01000 001 no CEL
//'g'=0b1 01000 010 no CEL
//'g'=0b1 01000 011 no CEL
//'g'=0b1 01000 100 no CEL
//'g'=0b1 01000 101 no CEL
//'g'=0b1 01000 110 P1635
//'g'=0b1 01000 111 no CEL
//'g'=0b1 01001 000 no CEL
//'g'=0b1 01001 001 no CEL
//'g'=0b1 01001 010 no CEL
//'g'=0b1 01001 011 no CEL
//'g'=0b1 01001 100 no CEL
//'g'=0b1 01001 101 no CEL
//'g'=0b1 01001 110 no CEL
//'g'=0b1 01001 111 no CEL
//'g'=0b1 01010 000 no CEL
//'g'=0b1 01010 001 no CEL
//'g'=0b1 01010 010 no CEL
//'g'=0b1 01010 011 no CEL
//'g'=0b1 01010 100 no CEL
//'g'=0b1 01010 101 no CEL
//'g'=0b1 01010 110 P1586
//'g'=0b1 01010 111 no CEL
//'g'=0b1 01011 000 no CEL
//'g'=0b1 01011 001 no CEL
//'g'=0b1 01011 010 no CEL
//'g'=0b1 01011 011 no CEL
//'g'=0b1 01011 100 no CEL
//'g'=0b1 01011 101 no CEL
//'g'=0b1 01011 110 P1580
//'g'=0b1 01011 111 no CEL
//'g'=0b1 01100 000 no CEL
//'g'=0b1 01100 001 no CEL
//'g'=0b1 01100 010 no CEL
//'g'=0b1 01100 011 no CEL
//'g'=0b1 01100 100 no CEL
//'g'=0b1 01100 101 no CEL
//'g'=0b1 01100 110 no CEL
//'g'=0b1 01100 111 no CEL
//'g'=0b1 01101 000 no CEL
//'g'=0b1 01101 001 no CEL
//'g'=0b1 01101 010 no CEL
//'g'=0b1 01101 011 no CEL
//'g'=0b1 01101 100 no CEL
//'g'=0b1 01101 101 no CEL
//'g'=0b1 01101 110 P1568
//'g'=0b1 01101 111 no CEL
//switched to only testing cases where last three bits '110'... I suspect '110' is an alert mask.
//'g'=0b1 01110 110 P1582
//'g'=0b1 01111 110 P1440
//'g'=0b1 10000 110 no CEL
//'g'=0b1 10001 110 no CEL
//'g'=0b1 10010 110 P1448
//'g'=0b1 10011 110 P1581
//'g'=0b1 10100 110 P1445
//'g'=0b1 10101 110 no CEL
//'g'=0b1 10110 110 P1583
//'g'=0b1 10111 110 P1572
//'g'=0b1 11000 110 P1439
//'g'=0b1 11001 110 P1573 (no CEL though)
//'g'=0b1 11010 110 P1585
//'g'=0b1 11011 110 P1576
//'g'=0b1 11100 110 P1577
//'g'=0b1 11101 110 P1648
//'g'=0b1 11110 110 P1649 (no CEL though)
//'g'=0b1 11111 110 no CEL

//so given the data and theory above, I should be able to set three P codes (arbitrarily picked the last 3: P1648/P1577/P1576)
//char frameorder[]={"bgbgbhbhbjbj"};
//Success.  The above three errors show up exactly as expected.  Unless there are more hidden codes elsewhere, we've got this cat skinned.

//Summary: 1 start bit (high), followed by 8 data bits.  IMA errors always end in '110'.  Thus, all IMA errors take form 0b1xxxxx110.
//21 'hard' IMA errors identified ('hard'=CEL illuminated)
//2 'soft' IMA errors identified ('soft'=CEL not lit).  There are probably more 'soft' IMA errors

//Leaving us with a known good IMA output:
char frameorder[]={"bc"};

void setup() {
pinMode(MOTFSApin,OUTPUT);  //clock
pinMode(MOTFSBpin,OUTPUT);  //data
Serial.begin(115200);
}

void loop() 
{
  for(int ii=0;ii<sizeof(frameorder);ii++)  //replace '10' w/ 'sizeof(frameorder)'
  { Serial.println("");
    bitbang_MOTFSB_frame(frameorder[ii]);
  }
}

void bitbang_MOTFSB_frame(char nn)
{  
  if (nn=='g')  //note: 'g' is a variable bitstream with no set pattern.
  {
    bangbit(1); //start bit (always high, defines frame)
    
    bangbit(1); //data bit 1 (MSB)
    bangbit(1); //data bit 2
    bangbit(1); //data bit 3
    bangbit(0); //data bit 4
    bangbit(1); //data bit 5
    
    bangbit(1); //data bit 6
    bangbit(1); //data bit 7
    bangbit(0); //data bit 8 (LSB)
    bangbit(0); //always low(?)
    Serial.print("G");
  }
  else if(nn=='a')
  {
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(0);  
    Serial.print("A");
  }
  else if (nn=='b')
  { 
    bangbit(0);
    bangbit(0);
    bangbit(0);
    bangbit(0);
    bangbit(0);
    bangbit(0);
    bangbit(0);
    bangbit(0);
    bangbit(0);
    bangbit(0); 
    Serial.print("B");
  }
  else if (nn=='c')
  { 
    bangbit(1);
    bangbit(1);
    bangbit(0);
    bangbit(0);
    bangbit(1);
    bangbit(1);
    bangbit(0);
    bangbit(0);
    bangbit(0);
    bangbit(0); 
    Serial.print("C");
  }
  else if (nn=='d')
  {
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(0);
    bangbit(1);
    bangbit(1);
    bangbit(0);
    bangbit(0);
    bangbit(0); 
    Serial.print("D");
  }
  else if (nn=='e')
  {
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(0);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(0);
    bangbit(0); 
    Serial.print("E");
  }
  else if (nn=='f')
  {
    bangbit(1);
    bangbit(1);
    bangbit(0);
    bangbit(0);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(1);
    bangbit(0);
    bangbit(0); 
    Serial.print("F");
  }
    else if (nn=='h')
  {
    bangbit(1); //start bit (always high, defines frame)
    
    bangbit(1); //data bit 1 (MSB)
    bangbit(1); //data bit 2
    bangbit(1); //data bit 3
    bangbit(0); //data bit 4
    bangbit(0); //data bit 5
    
    bangbit(1); //data bit 6
    bangbit(1); //data bit 7
    bangbit(0); //data bit 8 (LSB)
    bangbit(0); //always low(?)
    Serial.print("H");
  }
  else if (nn=='j')
  {
    bangbit(1); //start bit (always high, defines frame)
    
    bangbit(1); //data bit 1 (MSB)
    bangbit(1); //data bit 2
    bangbit(0); //data bit 3
    bangbit(1); //data bit 4
    bangbit(1); //data bit 5
    
    bangbit(1); //data bit 6
    bangbit(1); //data bit 7
    bangbit(0); //data bit 8 (LSB)
    bangbit(0); //always low(?)
    Serial.print("J");
  }
}

void bangbit(bool jj)
{
    digitalWrite(MOTFSApin,LOW);  //clock low
    digitalWrite(MOTFSBpin,jj);  //set data
    delay(20);  //20 ms
    digitalWrite(MOTFSApin,HIGH);  //clock high
    delay(20);
}
