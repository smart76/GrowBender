#pragma once
#define UARTBUFFERSIZE 400
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//#include "General.h"
//#include "Common.h"

extern uint8_t mUARTBuffer[UARTBUFFERSIZE];
extern uint16_t mUARTHead;
extern uint16_t mUARTTail;

void InitUART();
void FlashBuffer();
unsigned char uart_getc( void );
void uart_putb(uint8_t *data,  uint8_t len);
void uart_putc( char c );
void uart_puts( char *str );
void uart_putN(uint16_t n );
void uart_putNLn(uint16_t n );
void uart_putF(float n );
void monitor_puts( char *str );
void monitor_putc( char c );
void monitor_putN( uint16_t n );
void IMUWrite(char * buff, int len);