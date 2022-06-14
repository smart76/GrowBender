/*
 * LCD.cpp
 *
 * Created: 07.02.2016 12:37:32
 *  Author: smart
 */ 
#include "LCDI2C.h"
// unsigned  char  lcd_codepage[]=
// {
// 	0x41,0xa0,0x42,0xa1,0xe0,0x45,0xa3,0xa4,
// 	0xa5,0xa6,0x4b,0xa7,0x4d,0x48,0x4f,0xa8,
// 	0x50,0x43,0x54,0xa9,0xaa,0x58,0xe1,0xab,
// 	0xac,0xe2,0xad,0xae,0x62,0xaf,0xb0,0xb1,
// 	0x61,0xb2,0xb3,0xb4,0xe3,0x65,0xb6,0xb7,
// 	0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0x6f,0xbe,
// 	0x70,0x63,0xbf,0x79,0xe4,0x78,0xe5,0xc0,
// 	0xc1,0xe6,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
// };
#define I2CWAITTIMEOUT 100

bool canWait = false;
uint8_t mPortData = 8; 
uint8_t mWaitTimeout;
void InitLCD()
{
	//uart_puts("first send\r\n");
	i2c_init();

	mPortData = (1<<LCDBL) | (1<<LCDD5) | (1<<LCDD4);
	i2c_writeByte(mPortData);
	SignalE();
	_delay_ms(15);
	//uart_puts("second send\r\n");
	i2c_writeByte(mPortData);
	SignalE();

	mPortData &= ~(1<<LCDD4);
	//uart_puts("third send\r\n");
	i2c_writeByte(mPortData);
	SignalE();
	
	lcd_write(12); // display = on
	lcd_write(0b00101000); // funct

// 	lcd_clear(); // clear
// 	_delay_ms(100);
	// canWait = true;
}

static void lcd_wait(void)
{

	if (canWait)
	{
		// 1. set rs rw d7 
		// set e e
		// 4. send
		// 5. clear e
		// . send
		// 4. read d7
		// set e
		// send 
		// clear e
		// send
		
		// read busy flag 
		mPortData &= ~(1<<LCDRS);
		mPortData |= (1<<LCDRW);
		SignalE();
		
		mWaitTimeout = 0;
		while(i2c_read() & (1<<LCDD7) && mWaitTimeout < I2CWAITTIMEOUT)
		{
			SignalE();
			mWaitTimeout++;
			//uart_puts("z\r\n");
		}
		_delay_us(40);		
	}
	else
		_delay_us(100);
}

void lcd_goto(unsigned char x, unsigned char y )
{
	uint8_t address = 0;
	switch(y)
	{
		case 0:
			address = x;
		break;
		case 1:
			address = 0x40 + x;
		break;
		case 2:
			address = 0x14 + x;
		break;
		case 3:
			address = 0x54 + x;
		break;
	}
	
	mPortData &= ~(1<<LCDRS); // function
	mPortData &= ~(1<<LCDRW); // write
	//i2c_writeByte(mPortData);
	
	lcd_write(LCD_SET_DDRAM_ADDR | address);

	mPortData |= (1<<LCDRS); // data
	i2c_writeByte(mPortData);
	//SetRS();
}
void lcd_putc(unsigned char c)
{
// 	if ( c == 0xA8) {     //буква 'Ё'
// 		c = 0xA2;
// 	}
// 	else if ( c ==0xB8) { //буква 'ё'
// 		c = 0xB5;
// 	}
// 	else if ( c >= 0xC0 ) {
// 		c = lcd_codepage[ c - 0xC0 ];
// 	}
	lcd_write( c );
}
void lcd_puts( const char *str)
{
	while( *str ) {
		lcd_write( *str++ );
	}
}

void lcd_clear()
{
	mPortData &= ~(1<<LCDRS); // ClearRS();
	//SignalE();
//lcd_wait();
//_delay_us(50);
//SignalE();
lcd_write(1);
	//mPortData |= (1<<LCDRS); //SetRS();
	SignalE();
//lcd_wait();
_delay_ms(100);
}
void lcd_write(uint8_t pData)
{
	SetDataPins(pData);
	//i2c_writeByte(mPortData);
	SignalE();
	SetDataPins(pData<<4);
	//i2c_writeByte(mPortData);
	SignalE();

	//_delay_us(40);
	//lcd_wait();
}

void lcd_read(uint8_t pData)
{
	SetDataPins(pData);
	SignalE();
	SetDataPins(pData<<4);
	SignalE();

	//_delay_us(40);
	lcd_wait();
}

void SignalE()
{
	mPortData |= (1<<LCDE);
	i2c_writeByte(mPortData);
	//_delay_us(1);
	mPortData &= ~(1<<LCDE);
	i2c_writeByte(mPortData);
	//_delay_us(1);
}

void SetDataPins(uint8_t pData)
{
	if ((pData & 0x10) == 0)
		mPortData &= ~(1<<LCDD4);
	else
		mPortData |= (1<<LCDD4);
	if ((pData & 0x20) == 0)
		mPortData &= ~(1<<LCDD5);
	else
		mPortData |= (1<<LCDD5);
	if ((pData & 0x40) == 0)
		mPortData &= ~(1<<LCDD6);
	else
		mPortData |= (1<<LCDD6);
	if ((pData & 0x80) == 0)
		mPortData &= ~(1<<LCDD7);
	else
		mPortData |= (1<<LCDD7);
}

uint8_t GetDataPins()
{
	uint8_t res = 0;

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

void i2c_init()
{
//#if defined(INTERNAL_I2C_PULLUPS)
	I2C_PULLUPS_ENABLE;
// #else
// I2C_PULLUPS_DISABLE
//..#end	
	TWSR = 0;                                    // no prescaler => prescaler = 1
	TWBR = 0x02;//((F_CPU / I2C_SPEED) - 16) / 2;   // change the I2C clock rate
	TWCR = 1<<TWEN;                              // enable twi module, no interrupt
	//PORTD |= (1<<PD1);
}

unsigned char i2c_readAck(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	mWaitTimeout = 0;
	while(!(TWCR & (1<<TWINT)) && mWaitTimeout < I2CWAITTIMEOUT)
	{
		_delay_us(1);
		mWaitTimeout++;
	}
	//uart_putN(TWDR);
	return TWDR;
}

//********************************************************
//! @brief read I2C byte without acknowledgment
//!
//! @return read byte
//********************************************************
unsigned char i2c_readNak(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN);
	mWaitTimeout = 0;
	while(!(TWCR & (1<<TWINT)) && mWaitTimeout < I2CWAITTIMEOUT)
	{
		_delay_us(1);
		mWaitTimeout++;
	}
	return TWDR;
}

uint8_t  i2c_write(uint8_t data)
{
	uint8_t twst;
	TWDR = data; // send data to the previously addressed device
	TWCR = (1<<TWINT) | (1<<TWEN);
	mWaitTimeout = 0;
	while(!(TWCR & (1<<TWINT)) && mWaitTimeout < I2CWAITTIMEOUT)  // wait until transmission completed
	{
		_delay_us(1);
		mWaitTimeout++;
	}
	twst = TW_STATUS; // check value of TWI Status Register. Mask prescaler bits

	if( twst != TW_MT_DATA_ACK)
	{
		//uart_puts("error write (noack)\r\n");
		return 1;
	}
	else
	return 0;
}

unsigned char i2c_start(unsigned char address)
{
	unsigned char twst;
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send START condition
	mWaitTimeout = 0;
	while(!(TWCR & (1<<TWINT)) && mWaitTimeout < I2CWAITTIMEOUT) // wait until transmission completed
	{
		_delay_us(1);
		mWaitTimeout++;
	}
	twst = TW_STATUS;// check value of TWI Status Register. Mask prescaler bits.
	if ( (twst != TW_START) && (twst != TW_REP_START))
	{
		//uart_puts("Error starting (HW)!\r\n");
		return 1;
	}
	TWDR = address; // send device address
	TWCR = (1<<TWINT) | (1<<TWEN);
	// wait until transmission completed and ACK/NACK has been received
	mWaitTimeout = 0;
	while(!(TWCR & (1<<TWINT)) && mWaitTimeout < I2CWAITTIMEOUT)
	{
		_delay_us(1);
		mWaitTimeout++;
	}
	twst = TW_STATUS;

	// check value of TWI Status Register. Mask prescaler bits.
	if ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK) )
	{
		//uart_puts("Error starting (noack)!\r\n");
		return 1;
	}
	return 0;
}

void i2c_stop(void)
{
	//return;
	// send stop condition
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	// wait until stop condition is executed and bus released
	mWaitTimeout = 0;
	while(TWCR & (1<<TWSTO) && mWaitTimeout < I2CWAITTIMEOUT) //THIS MAKES PROBLEM FOR IS2402
	{
		_delay_us(1);
		mWaitTimeout++;
	}
}

void i2c_writeReg(uint8_t add, uint8_t reg, uint8_t val) {
	i2c_start(add); // I2C write direction
	i2c_write(reg);        // register selection
	i2c_write(val);        // value to write in register
	i2c_stop();
}

void i2c_writeByte(uint8_t pVal) {
	i2c_start(I2C_LCD_ADDRESS_WRITE); // I2C write direction
	i2c_write(pVal);        // value to write in register
	i2c_stop();
}

uint8_t i2c_read()
{
	uint8_t retVal;
	i2c_start(I2C_LCD_ADDRESS_READ); // I2C read direction
	retVal = i2c_readAck();        
	i2c_stop();
	return retVal;
}



