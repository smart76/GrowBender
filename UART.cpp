/*
 * UART.cpp
 *
 * Created: 04.11.2013 13:04:21
 *  Author: smart
 */ 
#include <stdlib.h>
#include "UART.h"

uint8_t mUARTBuffer[UARTBUFFERSIZE];
uint16_t mUARTHead = 0;
uint16_t mUARTTail = 0;

char sCmdBuff[5];

void InitUART()
{	
	// port 0 (command) initialization
	
	UBRR0H = 0x00;
	UBRR0L = 0x10; //115200
	//UBRR0L = 0x86; //9600
		
	UCSR0A = (1<<U2X0); // 2x speed mode
	UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);
	//8 бит данных, 1 стоп бит, без контроля четности
	UCSR0C = ( 1 <<  UCSZ01 ) | ( 1 << UCSZ00 );
	//( 1 << URSEL ) |
	// ;
}


unsigned char uart_getc( void )
{
	//ждем приема байта
	while( (UCSR0A & ( 1 << RXC0 ) ) == 0 );
	//считываем принятый байт
	return UDR0;
}

void uart_putc( char c )
{
	//ждем окончания передачи предыдущего байта
	while( ( UCSR0A & ( 1 << UDRE0 ) ) == 0 );
	UDR0 = c;
}

void uart_puts( char *str )
{
	unsigned char c;
	//char* str1 = str;
	while( ( c = *str++ ) != 0 ) {
		uart_putc( c );
	}
	//monitor_puts(str1);
}

void uart_putNLn(uint16_t n )
{
	itoa(n, sCmdBuff, 10);
	uart_puts(sCmdBuff);
	uart_puts("\r\n");
}

void uart_putN(uint16_t n )
{
	itoa(n, sCmdBuff, 10);
	uart_puts(sCmdBuff);
}

void uart_putF(float n )
{
	dtostrf((double)n, 2, 2, sCmdBuff);
	uart_puts(sCmdBuff);
	uart_puts("\r\n");
}

void uart_putb(uint8_t *data,  uint8_t len)
{
	for (uint8_t i = 0; i<len; i++)
	{
		while((UCSR0A & (1<<UDRE0)) == 0)
		;
		UDR0 = data[i];
	}
}

void FlashBuffer()
{
	mUARTTail = mUARTHead =0;
}

ISR(USART_RX_vect)
{
	if (mUARTHead >= UARTBUFFERSIZE)
	{
		//SetBuzzer(1);
		mUARTHead = mUARTTail = 0;
	//	return;
	}
	mUARTBuffer[mUARTHead] = UDR0;
	mUARTHead++;
	//UDR0 = mUARTBuffer[mUARTHead-1];
}
