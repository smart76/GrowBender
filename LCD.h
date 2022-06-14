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
#include <util/delay.h>

#define LCD4BITMODE
#define LCDRS PD2
#define LCDRW PD3
#define LCDE PB4

#define LCDD4 PD4
#define LCDD5 PD5
#define LCDD6 PD6
#define LCDD7 PD7

#define SetE(); PORTB |= (1<<LCDE)
#define SetRS();  PORTD |= (1<<LCDRS)
#define SetRW();  PORTD |= (1<<LCDRW)
#define ClearE();   PORTB &= ~(1<<LCDE)
#define ClearRS();  PORTD &= ~(1<<LCDRS)
#define ClearRW();  PORTD &= ~(1<<LCDRW)

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

void InitLCD();
void SetDataPins(uint8_t pData);
void SetDirection(bool pSetOutput);

uint8_t GetDataPins();
void SignalE();
void lcd_write(uint8_t pData);
void lcd_putc(unsigned char c);
void lcd_puts( const char *str);
void lcd_clear();

void lcd_goto( unsigned char x, unsigned char y );
#endif /* LCD_H_ */