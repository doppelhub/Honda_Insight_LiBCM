/*****************************************************************************
LICENSE:
	Copyright (C) 2021 John Sullivan
    Copyright (C) 2006 Peter Fleury

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*****************************************************************************/

#include	<inttypes.h>
#include	<avr/io.h>
#include	<avr/interrupt.h>
#include	<avr/boot.h>
#include	<avr/pgmspace.h>
#include	<util/delay.h>
#include	<avr/eeprom.h>
#include	<avr/common.h>
#include	<stdlib.h>
#include	"command.h"

#ifndef EEWE
	#define EEWE    1
#endif

#ifndef EEMWE
	#define EEMWE   2
#endif

#ifndef F_CPU
	#define F_CPU 16000000UL
#endif

#ifndef BAUDRATE
	#define BAUDRATE 115200
#endif

#ifndef UART_BAUDRATE_DOUBLE_SPEED
	#if defined (__AVR_ATmega32__)
		#define UART_BAUDRATE_DOUBLE_SPEED 0
	#else
		#define UART_BAUDRATE_DOUBLE_SPEED 1
	#endif
#endif

#define CONFIG_PARAM_BUILD_NUMBER_LOW	0
#define CONFIG_PARAM_BUILD_NUMBER_HIGH	0
#define CONFIG_PARAM_HW_VER				0x0F
#define CONFIG_PARAM_SW_MAJOR			2
#define CONFIG_PARAM_SW_MINOR			0x0A

//Calculate the address where the bootloader starts from FLASHEND and BOOTSIZE
//adjust BOOTSIZE below and BOOTLOADER_ADDRESS in Makefile if you want to change the size of the bootloader
//#define BOOTSIZE 1024
#if FLASHEND > 0x0F000
	#define BOOTSIZE 8192
#else
	#define BOOTSIZE 2048
#endif

#define APP_END  (FLASHEND -(2*BOOTSIZE) + 1)

//MEGA2560 device signature
#define SIGNATURE_BYTES 0x1E9801

//configure MEGA2560 UART0
#define	UART_BAUD_RATE_LOW			UBRR0L
#define	UART_STATUS_REG				UCSR0A
#define	UART_CONTROL_REG			UCSR0B
#define	UART_ENABLE_TRANSMITTER		TXEN0
#define	UART_ENABLE_RECEIVER		RXEN0
#define	UART_TRANSMIT_COMPLETE		TXC0
#define	UART_RECEIVE_COMPLETE		RXC0
#define	UART_DATA_REG				UDR0
#define	UART_DOUBLE_SPEED			U2X0

//calculate UBBR from XTAL and baudrate
#ifdef UART_BAUDRATE_DOUBLE_SPEED
	#define UART_BAUD_SELECT(baudRate,xtalCpu) (((float)(xtalCpu))/(((float)(baudRate))*8.0 )-1.0+0.5)
#else
	#define UART_BAUD_SELECT(baudRate,xtalCpu) (((float)(xtalCpu))/(((float)(baudRate))*16.0)-1.0+0.5)
#endif

//States used in the receive state machine
#define	ST_START		0
#define	ST_GET_SEQ_NUM	1
#define ST_MSG_SIZE_1	2
#define ST_MSG_SIZE_2	3
#define ST_GET_TOKEN	4
#define ST_GET_DATA		5
#define	ST_GET_CHECK	6
#define	ST_PROCESS		7

// use 16bit address variable for ATmegas with <= 64K flash
#if defined(RAMPZ)
	typedef uint32_t address_t;
#else
	typedef uint16_t address_t;
#endif

//function prototypes
static void sendchar(char c);

//since this bootloader is not linked against the avr-gcc crt1 functions,
//to reduce the code size, we need to provide our own initialization
void __jumpMain	(void) __attribute__ ((naked)) __attribute__ ((section (".init9")));

#include <avr/sfr_defs.h>

//*****************************************************************************

void __jumpMain(void)
{
	asm volatile ( ".set __stack, %0" :: "i" (RAMEND) );

	//set stack pointer to top of RAM
	asm volatile ( "ldi	16, %0" :: "i" (RAMEND >> 8) );
	asm volatile ( "out %0,16" :: "i" (AVR_STACK_POINTER_HI_ADDR) );

	asm volatile ( "ldi	16, %0" :: "i" (RAMEND & 0x0ff) );
	asm volatile ( "out %0,16" :: "i" (AVR_STACK_POINTER_LO_ADDR) );

	asm volatile ( "clr __zero_reg__" );									// GCC depends on register r1 set to 0
	asm volatile ( "out %0, __zero_reg__" :: "I" (_SFR_IO_ADDR(SREG)) );	// set SREG to 0
	asm volatile ( "jmp main");												// jump to main()
}

//*****************************************************************************

void delay_ms(unsigned int timedelay)
{
	unsigned int i;
	for (i=0;i<timedelay;i++) { _delay_ms(0.5); } //JTS: janky implementation
}

//*****************************************************************************

//send single byte to USART, wait until transmission is completed
static void sendchar(char c)
{
	UART_DATA_REG	=	c;										// prepare transmission
	while (!(UART_STATUS_REG & (1 << UART_TRANSMIT_COMPLETE)));	// wait until byte sent
	UART_STATUS_REG |= (1 << UART_TRANSMIT_COMPLETE);			// delete TXCflag
}

//************************************************************************

static int Serial_Available(void)
{
	return(UART_STATUS_REG & (1 << UART_RECEIVE_COMPLETE));	// wait for data
}

//*****************************************************************************

#define	MAX_TIME_COUNT	(F_CPU >> 1)

static unsigned char recchar_timeout(void)
{
uint32_t count = 0;

	while ( !( UART_STATUS_REG & (1 << UART_RECEIVE_COMPLETE) ) )
	{	// wait for data
		count++;
		if (count > MAX_TIME_COUNT)
		{
		unsigned int	data;
		#if (FLASHEND > 0x10000)
			data	=	pgm_read_word_far(0);	//*	get the first word of the user program
		#else
			data	=	pgm_read_word_near(0);	//*	get the first word of the user program
		#endif
			if (data != 0xffff)					//*	make sure its valid before jumping to it.
			{
				asm volatile(
						"clr	r30		\n\t"
						"clr	r31		\n\t"
						"ijmp	\n\t"
						);
			}
			count	=	0;
		}
	}
	return UART_DATA_REG;
}

//*****************************************************************************

void (*app_start)(void) = 0x0000;

//*****************************************************************************
int main(void)
{
	address_t		address			=	0;
	address_t		eraseAddress	=	0;
	unsigned char	msgParseState;
	unsigned int	ii				=	0;
	unsigned char	checksum		=	0;
	unsigned char	seqNum			=	0;
	unsigned int	msgLength		=	0;
	unsigned char	msgBuffer[285];
	unsigned char	c, *p;
	unsigned char   isLeave = 0;

	unsigned long	boot_timeout;
	unsigned long	boot_timer;
	unsigned int	boot_state;

	//*	some chips dont set the stack properly
	asm volatile ( ".set __stack, %0" :: "i" (RAMEND) );
	asm volatile ( "ldi	16, %0" :: "i" (RAMEND >> 8) );
	asm volatile ( "out %0,16" :: "i" (AVR_STACK_POINTER_HI_ADDR) );
	asm volatile ( "ldi	16, %0" :: "i" (RAMEND & 0x0ff) );
	asm volatile ( "out %0,16" :: "i" (AVR_STACK_POINTER_LO_ADDR) );

	//	handle the watch dog timer
	uint8_t	mcuStatusReg;
	mcuStatusReg	=	MCUSR;

	__asm__ __volatile__ ("cli");
	__asm__ __volatile__ ("wdr");
	MCUSR	=	0;
	WDTCSR	|=	_BV(WDCE) | _BV(WDE);
	WDTCSR	=	0;
	__asm__ __volatile__ ("sei");

	// check if WDT generated the reset
	if ( mcuStatusReg & _BV(WDRF) ) { app_start(); }

	// check if keyON (PINB7 == HIGH)
	if ( (PINB & (1<<PINB7)) != 0 )
	{	//jump directly to main firmware
		asm volatile(
		"clr	r30		\n\t"
		"clr	r31		\n\t"
		"ijmp	\n\t"
		);
	}

	//************************************************************************
	//If we get here, then the bootloader delay starts and runs until timeout

	boot_timer	=	0;
	boot_state	=	0;

	boot_timeout	=	500000; // ~1 second , approx 2us per count when optimize "s"

	//set baudrate and enable USART receiver and transmiter without interrupts
	#if UART_BAUDRATE_DOUBLE_SPEED
		UART_STATUS_REG		|=	(1 <<UART_DOUBLE_SPEED);
	#endif
	UART_BAUD_RATE_LOW	=	UART_BAUD_SELECT(BAUDRATE,F_CPU);
	UART_CONTROL_REG	=	(1 << UART_ENABLE_RECEIVER) | (1 << UART_ENABLE_TRANSMITTER);

	asm volatile ("nop"); // wait until port has changed

	while (boot_state == 0)
	{
		while ( (!(Serial_Available() ) ) && (boot_state == 0)) // wait for data
		{
			_delay_ms(0.001);
			boot_timer++;
			if (boot_timer > boot_timeout) { boot_state	=	1; } // (after ++ -> boot_state=2 bootloader timeout, jump to main 0x00000 )
		}
		boot_state++; // ( if boot_state=1 bootloader received byte from UART, enter bootloader mode)
	}

	if (boot_state == 1)
	{	//main loop
		while (!isLeave)
		{
			// Collect received bytes to a complete message
			msgParseState	=	ST_START;
			while ( msgParseState != ST_PROCESS )
			{
				if (boot_state == 1)
				{
					boot_state	= 0;
					c			= UART_DATA_REG;
				} else {
					c	        = recchar_timeout();
				}

				switch (msgParseState)
				{
					case ST_START:
						if ( c == MESSAGE_START )
						{
							msgParseState	=	ST_GET_SEQ_NUM;
							checksum		=	MESSAGE_START^0;
						}
						break;

					case ST_GET_SEQ_NUM:
						seqNum			=	c;
						msgParseState	=	ST_MSG_SIZE_1;
						checksum		^=	c;
						break;

					case ST_MSG_SIZE_1:
						msgLength		=	c<<8;
						msgParseState	=	ST_MSG_SIZE_2;
						checksum		^=	c;
						break;

					case ST_MSG_SIZE_2:
						msgLength		|=	c;
						msgParseState	=	ST_GET_TOKEN;
						checksum		^=	c;
						break;

					case ST_GET_TOKEN:
						if ( c == TOKEN )
						{
							msgParseState	=	ST_GET_DATA;
							checksum		^=	c;
							ii				=	0;
						} else {
							msgParseState	=	ST_START;
						}
						break;

					case ST_GET_DATA:
						msgBuffer[ii++]	=	c;
						checksum		^=	c;
						if (ii == msgLength ) { msgParseState = ST_GET_CHECK; }
						break;

					case ST_GET_CHECK:
						if ( c == checksum ) { msgParseState = ST_PROCESS; }
						else                 { msgParseState = ST_START;   }
						break;
				}	
			}

			//process the STK500 commands, see Atmel Appnote AVR068
			switch (msgBuffer[0])
			{
				case CMD_SPI_MULTI:
				{
					unsigned char answerByte;
					unsigned char flag=0;

					if ( msgBuffer[4]== 0x30 )
					{
						unsigned char signatureIndex	=	msgBuffer[6];

						if      ( signatureIndex == 0 ) { answerByte = (SIGNATURE_BYTES >> 16) & 0x000000FF; }
						else if ( signatureIndex == 1 ) { answerByte = (SIGNATURE_BYTES >> 8 ) & 0x000000FF; }
						else                            { answerByte =	SIGNATURE_BYTES        & 0x000000FF; }
					}
					else if ( msgBuffer[4] & 0x50 )
					{
						if      (msgBuffer[4] == 0x50) { answerByte = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);  }
						else if (msgBuffer[4] == 0x58) { answerByte = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS); }
						else                           { answerByte = 0;                                           }
					}
					else                               { answerByte = 0; } // unimplemented command //return dummy value to make AVRDUDE happy
	
					if ( !flag )
					{
						msgLength		=	7;
						msgBuffer[1]	=	STATUS_CMD_OK;
						msgBuffer[2]	=	0;
						msgBuffer[3]	=	msgBuffer[4];
						msgBuffer[4]	=	0;
						msgBuffer[5]	=	answerByte;
						msgBuffer[6]	=	STATUS_CMD_OK;
					}
				}
					break;

				case CMD_SIGN_ON:
					msgLength		=	11;
					msgBuffer[1] 	=	STATUS_CMD_OK;
					msgBuffer[2] 	=	8;
					msgBuffer[3] 	=	'A';
					msgBuffer[4] 	=	'V';
					msgBuffer[5] 	=	'R';
					msgBuffer[6] 	=	'I';
					msgBuffer[7] 	=	'S';
					msgBuffer[8] 	=	'P';
					msgBuffer[9] 	=	'_';
					msgBuffer[10]	=	'2';
					break;

				case CMD_GET_PARAMETER:
				{
					unsigned char value;

					switch(msgBuffer[1])
					{
						case PARAM_BUILD_NUMBER_LOW:  value = CONFIG_PARAM_BUILD_NUMBER_LOW;  break;
						case PARAM_BUILD_NUMBER_HIGH: value = CONFIG_PARAM_BUILD_NUMBER_HIGH; break;
						case PARAM_HW_VER:            value = CONFIG_PARAM_HW_VER;            break;
						case PARAM_SW_MAJOR:          value = CONFIG_PARAM_SW_MAJOR;          break;
						case PARAM_SW_MINOR:          value = CONFIG_PARAM_SW_MINOR;          break;
						default:                      value = 0;                              break;
					}
					msgLength		=	3;
					msgBuffer[1]	=	STATUS_CMD_OK;
					msgBuffer[2]	=	value;
				}
					break;

				case CMD_LEAVE_PROGMODE_ISP:
					isLeave	=	1;
					//*	fall thru

				case CMD_SET_PARAMETER:
				case CMD_ENTER_PROGMODE_ISP:
					msgLength		=	2;
					msgBuffer[1]	=	STATUS_CMD_OK;
					break;

				case CMD_READ_SIGNATURE_ISP:
				{
					unsigned char signatureIndex = msgBuffer[4];
					unsigned char signature;
					if      ( signatureIndex == 0 ) { signature = (SIGNATURE_BYTES >>16) & 0x000000FF; }
					else if ( signatureIndex == 1 ) { signature = (SIGNATURE_BYTES >> 8) & 0x000000FF; }
					else                            { signature =  SIGNATURE_BYTES       & 0x000000FF; }
					msgLength		=	4;
					msgBuffer[1]	=	STATUS_CMD_OK;
					msgBuffer[2]	=	signature;
					msgBuffer[3]	=	STATUS_CMD_OK;
				}
					break;

				case CMD_READ_LOCK_ISP:
					msgLength		=	4;
					msgBuffer[1]	=	STATUS_CMD_OK;
					msgBuffer[2]	=	boot_lock_fuse_bits_get( GET_LOCK_BITS );
					msgBuffer[3]	=	STATUS_CMD_OK;
					break;

				case CMD_READ_FUSE_ISP:
				{
					unsigned char fuseBits;
					if ( msgBuffer[2] == 0x50 )
					{
						if ( msgBuffer[3] == 0x08 ) { fuseBits = boot_lock_fuse_bits_get( GET_EXTENDED_FUSE_BITS ); }
						else                        { fuseBits = boot_lock_fuse_bits_get( GET_LOW_FUSE_BITS );      }
					}
					else                            { fuseBits = boot_lock_fuse_bits_get( GET_HIGH_FUSE_BITS );     }
					msgLength		=	4;
					msgBuffer[1]	=	STATUS_CMD_OK;
					msgBuffer[2]	=	fuseBits;
					msgBuffer[3]	=	STATUS_CMD_OK;
				}
					break;

				case CMD_PROGRAM_LOCK_ISP:
				{
					unsigned char lockBits	=	msgBuffer[4];
					lockBits	=	(~lockBits) & 0x3C;	// mask BLBxx bits
					boot_lock_bits_set(lockBits);		// and program it
					boot_spm_busy_wait();
					msgLength		=	3;
					msgBuffer[1]	=	STATUS_CMD_OK;
					msgBuffer[2]	=	STATUS_CMD_OK;
				}
					break;

				case CMD_CHIP_ERASE_ISP:
					eraseAddress	=	0;
					msgLength		=	2;
					msgBuffer[1]	=	STATUS_CMD_FAILED;
					break;

				case CMD_LOAD_ADDRESS:
					#if defined(RAMPZ)
						address	=	( ((address_t)(msgBuffer[1])<<24)|((address_t)(msgBuffer[2])<<16)|((address_t)(msgBuffer[3])<<8)|(msgBuffer[4]) )<<1;
					#else
						address	=	( ((msgBuffer[3])<<8)|(msgBuffer[4]) )<<1;		//convert word to byte address
					#endif
					msgLength		=	2;
					msgBuffer[1]	=	STATUS_CMD_OK;
					break;

				case CMD_PROGRAM_FLASH_ISP: //falls thru
				case CMD_PROGRAM_EEPROM_ISP:
				{
					unsigned int	size	=	((msgBuffer[1])<<8) | msgBuffer[2];
					unsigned char	*p	=	msgBuffer+10;
					unsigned int	data;
					unsigned char	highByte, lowByte;
					address_t		tempaddress	=	address;

					if ( msgBuffer[0] == CMD_PROGRAM_FLASH_ISP )
					{	// erase only main section (bootloader protection)
						if (eraseAddress < APP_END )
						{
							boot_page_erase(eraseAddress);	// Perform page erase
							boot_spm_busy_wait();		// Wait until the memory is erased.
							eraseAddress += SPM_PAGESIZE;	// point to next page to be erase
						}
						
						do { /* Write FLASH */
							lowByte		=	*p++;
							highByte 	=	*p++;

							data		=	(highByte << 8) | lowByte;
							boot_page_fill(address,data);

							address	=	address + 2;	// Select next word in memory
							size	-=	2;				// Reduce number of bytes to write by two
						} while (size);					// Loop until all bytes written

						boot_page_write(tempaddress);
						boot_spm_busy_wait();
						boot_rww_enable();				// Re-enable the RWW section
					} else {
						uint16_t ii = address >> 1;
						/* write EEPROM */
						while (size)
						{
							eeprom_write_byte((uint8_t*)ii, *p++);
							address+=2;						// Select next EEPROM byte
							ii++;
							size--;
						}
					}
					msgLength		=	2;
					msgBuffer[1]	=	STATUS_CMD_OK;
				}
					break;

				case CMD_READ_FLASH_ISP: //fall thru
				case CMD_READ_EEPROM_ISP:
				{
					unsigned int	size	=	((msgBuffer[1])<<8) | msgBuffer[2];
					unsigned char	*p		=	msgBuffer+1;
					msgLength				=	size+3;

					*p++	=	STATUS_CMD_OK;
					if (msgBuffer[0] == CMD_READ_FLASH_ISP )
					{
						unsigned int data;

						do { // Read FLASH
							#if (FLASHEND > 0x10000)
								data	=	pgm_read_word_far(address);
							#else
								data	=	pgm_read_word_near(address);
							#endif
							*p++	=	(unsigned char)data;		//LSB
							*p++	=	(unsigned char)(data >> 8);	//MSB
							address	+=	2;							// Select next word in memory
							size	-=	2;
						}while (size);
					} else {
						do { //Read EEPROM
							EEARL	=	address;			// Setup EEPROM address
							EEARH	=	((address >> 8));
							address++;					// Select next EEPROM byte
							EECR	|=	(1<<EERE);			// Read EEPROM
							*p++	=	EEDR;				// Send EEPROM data
							size--;
						} while (size);
					}
					*p++	=	STATUS_CMD_OK;
				}
					break;

				default:
					msgLength		=	2;
					msgBuffer[1]	=	STATUS_CMD_FAILED;
					break;
			}

			//send answer message back
			sendchar(MESSAGE_START); checksum = MESSAGE_START^0;
			sendchar(seqNum);        checksum ^= seqNum;
			
			c = ((msgLength>>8)&0xFF);
			sendchar(c);             checksum ^= c;
			
			c = msgLength&0x00FF;
			sendchar(c);             checksum ^= c;
			sendchar(TOKEN);         checksum ^= TOKEN;

			p = msgBuffer;

			while ( msgLength )
			{
				c = *p++;
				sendchar(c);
				checksum ^=c;
				msgLength--;
			}
			sendchar(checksum);
			seqNum++;
		}
	}

	asm volatile ("nop");			// wait until port has changed

	//Exit bootloader
	UART_STATUS_REG	&=	0xfd;
	boot_rww_enable();				// enable application section

	asm volatile( //jump to user code
			"clr	r30		\n\t"
			"clr	r31		\n\t"
			"ijmp	\n\t"
			);

	//Code never reaches this point, but the compiler doesn't understand this
	for(;;); //Prevents GCC from generating exit return code
}