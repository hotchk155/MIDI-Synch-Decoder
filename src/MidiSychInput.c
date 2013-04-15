////////////////////////////////////////////////////////////
//
// MIDI SYNCH RECEIVER
//
// Code for PIC12F1822
// Compiled with SourceBoost C
//
// hotchk155/2013
//
// Firmware version 
// 1.00 arpie baseline
//
////////////////////////////////////////////////////////////

#include <system.h>
#include <memory.h>

// 8MHz internal oscillator block, reset disabled
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 8000000
typedef unsigned char byte;

// define the I/O pins. Note that 
// PORTA.5 is the UART RX pin
// PORTA.3 is left as VPP only
#define P_LED		porta.4
#define P_RUN		porta.0
#define P_SYNCH		porta.1
#define P_RESTART	porta.2

// how long the output pulses in terms of main loop cycles
// A value of 100 gives ~2ms output pulse duration
#define OUTPUT_HIGH_TIME 100

volatile byte bRunning = 0;
volatile byte bSynchCount = 0;
volatile byte bRestartCount = 0;

////////////////////////////////////////////////////////////
// INTERRUPT HANDLER CALLED WHEN CHARACTER RECEIVED AT 
// SERIAL PORT
void interrupt( void )
{
	// check if this is serial rx interrupt
	if(pir1.5)
	{

		// get the byte
		byte b = rcreg;
				
		switch(b)
		{
			case 0xf8:
				P_SYNCH = 1;
				bSynchCount = OUTPUT_HIGH_TIME;
				break;
			case 0xfa: // START
				P_RESTART = 1;
				P_RUN = 1;
				bRestartCount = OUTPUT_HIGH_TIME;
				break;
			case 0xfb: // CONTINUE
				P_RUN = 1;
				break;
			case 0xfc: // STOP
				P_RUN = 0;
				break;
		}				
	}
}

////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void init_usart()
{
	pir1.1 = 1;		//TXIF 		
	pir1.5 = 0;		//RCIF
	
	pie1.1 = 0;		//TXIE 		no interrupts
	pie1.5 = 1;		//RCIE 		interrupt on receive
	
	baudcon.4 = 0;	// SCKP		synchronous bit polarity 
	baudcon.3 = 1;	// BRG16	enable 16 bit brg
	baudcon.1 = 0;	// WUE		wake up enable off
	baudcon.0 = 0;	// ABDEN	auto baud detect
		
	txsta.6 = 0;	// TX9		8 bit transmission
	txsta.5 = 0;	// TXEN		transmit enable
	txsta.4 = 0;	// SYNC		async mode
	txsta.3 = 0;	// SEDNB	break character
	txsta.2 = 0;	// BRGH		high baudrate 
	txsta.0 = 0;	// TX9D		bit 9

	rcsta.7 = 1;	// SPEN 	serial port enable
	rcsta.6 = 0;	// RX9 		8 bit operation
	rcsta.5 = 1;	// SREN 	enable receiver
	rcsta.4 = 1;	// CREN 	continuous receive enable
		
	spbrgh = 0;		// brg high byte
	spbrg = 15;		// brg low byte (31250)	
	
}

////////////////////////////////////////////////////////////
// ENTRY POINT
void main()
{ 
	// osc control / 8MHz / internal
	osccon = 0b01110010;
	
	// enable serial receive interrupt
	intcon = 0b11000000;
	pie1.5 = 1;

	// configure io
	trisa = 0b00100000;              	
	ansela = 0b00000000;
	porta=0;

	apfcon.7=1; // RX on RA5
	apfcon.2=1;	// TX on RA4

	// startup flash
	P_LED=1; delay_ms(200);
	P_LED=0; delay_ms(200);
	P_LED=1; delay_ms(200);
	P_LED=0; delay_ms(200);
	P_LED=1; delay_ms(200);
	P_LED=0; delay_ms(200);

	// initialise USART
	init_usart();

	// loop forever		
	unsigned long count = 0;
	for(;;)
	{
		P_LED = ((count & 0xf000UL) == 0x1000UL);
		++count;
			
		if(bSynchCount)
		{
			if(!--bSynchCount) 
				P_SYNCH = 0;
		}
		if(bRestartCount)
		{
			if(!--bRestartCount) 
				P_RESTART = 0;
		}		
	}
}