/*
 * LCD.h
 *
 * Created: 06.02.2016 23:55:37
 *  Author: smart
 */ 


#ifndef LCD_H_
#define LCD_H_
#include <stdlib.h>
#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <util/twi.h>
#include <string.h>

#define LCD4BITMODE
#define LCDRS 0
#define LCDRW 1
#define LCDE 2
#define LCDBL 3

#define LCDD4 4
#define LCDD5 5
#define LCDD6 6
#define LCDD7 7

// #define SetE(); PORTB |= (1<<LCDE)
// #define SetRS();  PORTD |= (1<<LCDRS)
// #define SetRW();  PORTD |= (1<<LCDRW)
// #define ClearE();   PORTB &= ~(1<<LCDE)
// #define ClearRS();  PORTD &= ~(1<<LCDRS)
// #define ClearRW();  PORTD &= ~(1<<LCDRW)

#define LCD_CLR              _BV( 0 ) /*Очистить дисплей*/
#define LCD_HOME             _BV( 1 ) /*Переход в начало экрана*/
#define LCD_MODE             _BV( 2 ) /*Сдвиг курсора или экрана при записи символа*/
#define LCD_MODE_INC         _BV( 1 )
#define LCD_MODE_SHIFT       _BV( 0 )
#define LCD_ON               _BV( 3 ) /*Вкл/выкл дисплей, курсор, мигание*/
#define LCD_ON_DISPLAY       _BV( 2 )
#define LCD_ON_CURSOR        _BV( 1 )
#define LCD_ON_BLINK         _BV( 0 )
#define LCD_FUNCTION         _BV( 5 ) /*Разрядность шины данных, кол-во строк, размер шрифта*/
#define LCD_FUNCTION_8BIT    _BV( 4 )
#define LCD_FUNCTION_2LINES  _BV( 3 )
#define LCD_FUNCTION_10DOTS  _BV( 2 )
#define LCD_SET_CGRAM_ADDR   _BV( 6 ) /*Установка адреса CGRAM*/
#define LCD_SET_DDRAM_ADDR   _BV( 7 ) /*Установка адреса DDRAM*/

#if DEVICE0
#define I2C_LCD_ADDRESS_WRITE 0b01111110
#define I2C_LCD_ADDRESS_READ 0b01111111
#endif

#if DEVICE1
#define I2C_LCD_ADDRESS_WRITE 0b01001110
#define I2C_LCD_ADDRESS_READ 0b01001111
#endif

#define I2C_SPEED			 4000000L     //120kHz (100 default) normal mode, this value must be used for a genuine WMP

#define I2C_PULLUPS_ENABLE   PORTC |= (1<<PC4) | (1<<PC5)   // PIN A4&A5 (SDA&SCL)
#define I2C_PULLUPS_DISABLE  PORTC &= ~(1<<4); PORTC &= ~(1<<5)
#define INTERNAL_I2C_PULLUPS 

void InitLCD();
void SetDataPins(uint8_t pData);
void SetDirection(bool pSetOutput);

uint8_t GetDataPins();
void SignalE();
void lcd_write(uint8_t pData);
void lcd_putc(unsigned char c);
void lcd_puts( const char *str);
void lcd_clear();

void i2c_init(void);
uint8_t i2c_write(uint8_t  data);
uint8_t i2c_read();
void i2c_writeByte(uint8_t pVal);
extern uint8_t mPortData;
void lcd_goto( unsigned char x, unsigned char y );


#endif /* LCD_H_ */