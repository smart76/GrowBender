/*
 * LCD.cpp
 *
 * Created: 07.02.2016 12:37:32
 *  Author: smart
 */ 
#include "LCD.h"
unsigned  char  lcd_codepage[]=
{
	0x41,0xa0,0x42,0xa1,0xe0,0x45,0xa3,0xa4,
	0xa5,0xa6,0x4b,0xa7,0x4d,0x48,0x4f,0xa8,
	0x50,0x43,0x54,0xa9,0xaa,0x58,0xe1,0xab,
	0xac,0xe2,0xad,0xae,0x62,0xaf,0xb0,0xb1,
	0x61,0xb2,0xb3,0xb4,0xe3,0x65,0xb6,0xb7,
	0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0x6f,0xbe,
	0x70,0x63,0xbf,0x79,0xe4,0x78,0xe5,0xc0,
	0xc1,0xe6,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
};
bool canWait = false;
void InitLCD()
{
	DDRD |= (1<<LCDRS) | (1<<LCDRW);
	DDRB |= (1<<LCDE);
	SetDirection(true);
	
	ClearRW();   //переключаемся в режим записи
	ClearRS();   //записывать будем команды
	 _delay_ms(15);
	 //выбираем 8-ми битную шину и 2х строчный режим
	 lcd_write(LCD_FUNCTION /*| LCD_FUNCTION_8BIT*/ | LCD_FUNCTION_2LINES );
	 _delay_ms(5);
	 /*lcd_write(LCD_FUNCTION | LCD_FUNCTION_8BIT | LCD_FUNCTION_2LINES );
	 _delay_us( 100 );
	 lcd_write(LCD_FUNCTION | LCD_FUNCTION_8BIT | LCD_FUNCTION_2LINES );
	 */
	 //разрешаем автоматическое увеличение текущей позиции
	 lcd_write(LCD_MODE | LCD_MODE_INC);
 
	 //включаем дисплей
	 lcd_write(LCD_ON | LCD_ON_DISPLAY);
 
	 //установка начального адреса
	 lcd_write(LCD_SET_DDRAM_ADDR | 0x00);
 
	 SetRS(); //теперь будем записывать данные, выводимые на ЖКИ
	 canWait = true;
}

static void lcd_wait(void)
{

	if (canWait)
	{
		ClearRS();
	 	SetRW(); // set read
		DDRB &= ~(1<<LCDD7); // set input
		//PORTB &= ~(1<<LCDD7); 
		//SetDirection(false); // secure way
		while((PINB & (1<<LCDD7)))
		{
			SignalE();
		}
		_delay_us( 100 );		
		DDRB |= (1<<LCDD7); // set output
		//SetDirection(true); // set output
	
 		SetRS();
		ClearRW(); // set write
	}
	//else 
// 

	_delay_us(40);

}
void lcd_goto( unsigned char x, unsigned char y )
{
	unsigned char address = x;
	if ( y ) {
		address += 0x40;
	}
	ClearRS();
	lcd_write( LCD_SET_DDRAM_ADDR |  address );
	SetRS();
}
void lcd_putc(unsigned char c)
{
	if ( c == 0xA8) {     //буква 'Ё'
		c = 0xA2;
	}
	else if ( c ==0xB8) { //буква 'ё'
		c = 0xB5;
	}
	else if ( c >= 0xC0 ) {
		c = lcd_codepage[ c - 0xC0 ];
	}
	lcd_write( c );
}
void lcd_puts( const char *str)
{
	while( *str ) {
		lcd_putc( *str++ );
	}
}

void lcd_clear()
{
	ClearRS();
//lcd_wait();
_delay_us(50);
//SignalE();
lcd_write(0);
	SetRS();
//lcd_wait();
_delay_us(50);
}
void lcd_write(uint8_t pData)
{
	SetDataPins(pData);
	SignalE();
#ifdef LCD4BITMODE
	SetDataPins(pData<<4);
	SignalE();
#endif
	//_delay_us(40);
	lcd_wait();
}
void SignalE()
{
	SetE();
	_delay_us(1);
	ClearE();
	_delay_us(1);
}
void SetDirection(bool pSetOutput)
{
#ifdef LCD4BITMODE
	if (pSetOutput)
	{
		//DDRC |= (1<<LCDD0) | (1<<LCDD1) | (1<<LCDD2) | (1<<LCDD3);
		DDRD |= (1<<LCDD4) | (1<<LCDD5) | (1<<LCDD6) | (1<<LCDD7);
	}
	else
	{
		//DDRC &= ~((1<<LCDD0) | (1<<LCDD1) | (1<<LCDD2) | (1<<LCDD3));
		DDRD &= ~((1<<LCDD4) | (1<<LCDD5) | (1<<LCDD6) | (1<<LCDD7));
	}
#else
dd
	if (pSetOutput)
	{
		DDRC |= (1<<LCDD0) | (1<<LCDD1) | (1<<LCDD2) | (1<<LCDD3);
		DDRB |= (1<<LCDD4) | (1<<LCDD5) | (1<<LCDD6) | (1<<LCDD7);
	}
	else
	{
		DDRC &= ~((1<<LCDD0) | (1<<LCDD1) | (1<<LCDD2) | (1<<LCDD3));
		DDRB &= ~((1<<LCDD4) | (1<<LCDD5) | (1<<LCDD6) | (1<<LCDD7));
	}
#endif
	
}

void SetDataPins(uint8_t pData)
{
#ifdef LCD4BITMODE
#else
	if ((pData & 1) == 0)
		PORTC &= ~(1<<LCDD0);
	else
		PORTC |= (1<<LCDD0);
	if ((pData & 2) == 0)
		PORTC &= ~(1<<LCDD1);
	else
		PORTC |= (1<<LCDD1);
	if ((pData & 4) == 0)
		PORTC &= ~(1<<LCDD2);
	else
		PORTC |= (1<<LCDD2);
	if ((pData & 8) == 0)
		PORTC &= ~(1<<LCDD3);
	else
		PORTC |= (1<<LCDD3);
#endif
	if ((pData & 0x10) == 0)
		PORTD &= ~(1<<LCDD4);
	else
		PORTD |= (1<<LCDD4);
	if ((pData & 0x20) == 0)
		PORTD &= ~(1<<LCDD5);
	else
		PORTD |= (1<<LCDD5);
	if ((pData & 0x40) == 0)
		PORTD &= ~(1<<LCDD6);
	else
		PORTD |= (1<<LCDD6);
	if ((pData & 0x80) == 0)
		PORTD &= ~(1<<LCDD7);
	else
		PORTD |= (1<<LCDD7);
}

uint8_t GetDataPins()
{
	uint8_t res = 0;
#ifdef LCD4BITMODE
#else
	if ((PINC & (1<<LCDD0)) != 0)
		res |=  0x01;
	if ((PINC & (1<<LCDD1)) != 0)
		res |= 0x02;
	if ((PINC & (1<<LCDD2)) != 0)
		res |= 0x04;
	if ((PINC & (1<<LCDD3)) != 0)
		res |= 0x08;
#endif
	if ((PIND & (1<<LCDD4)) != 0)
		res |=  0x10;
	if ((PIND & (1<<LCDD5)) != 0)
		res |= 0x20;
	if ((PIND & (1<<LCDD6)) != 0)
		res |= 0x40;
	if ((PIND & (1<<LCDD7)) != 0)
		res |= 0x80;
		
	return res;
}