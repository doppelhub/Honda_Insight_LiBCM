This tool is used to create an ISP that will program an Arduino Uno to program the LiBCM bootloader onto an Arduino Mega.  This custom bootloader is identical to the shipping version, except that on power-up it checks to see if the key is turned on, in which case the bootloader immediately jumps to LiBCM firmware.



See instructions here:
http://www.gammon.com.au/bootloader

Load Atmega_Board_Programmer.ino onto an Arduino Uno, then connect it to the mega as shown in the above link.  

Open debug window at 115200 and follow instructions