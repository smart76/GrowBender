/*
 * DHT22.cpp
 *
 * Created: 12.03.2017 14:02:55
 *  Author: smart
 */ 
#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>

#include "DHT22.h"

uint16_t mError_dht;
float mHumidity;
float mCurrentTempIn_dht;
	
void ReadData()
{
	uint16_t data = 0;
	uint16_t tdata = 0;
	DDRB |= (1<<PB3);
	PORTB &= ~(1<<PB3); // signal start to DHT
	_delay_ms(1);   // 1ms required
	_delay_us(100); // +0.1 ms for insurance

	PORTB |= (1<<PB3);
	DDRB &= ~(1<<PB3);
	
	uint8_t waitCounter;

	//wait for low // line goes low // DHT reacts
	for(waitCounter = 200; waitCounter > 0 && (PINB & (1<<PB3)); waitCounter--)
		_delay_us(1);
	if (waitCounter == 0 || waitCounter == 200) // timeout error // 20-40
		mError_dht = 1;

	//wait for high
	for(waitCounter = 200; waitCounter > 0 && !(PINB & (1<<PB3)); waitCounter--) //for(waitCounter = 200; waitCounter > 0; waitCounter--)
		_delay_us(1);

	if (waitCounter == 0 || waitCounter == 200) // timeout error
		mError_dht += 2;
	else if (waitCounter > 110) //80 us means start
		;

	//wait for low
	for(waitCounter = 200; waitCounter > 0 && (PINB & (1<<PB3)); waitCounter--)
		_delay_us(1);
	
	if (waitCounter == 0 || waitCounter == 200) // timeout error
		mError_dht += 4;
	else if (waitCounter > 110) //80 us means start
		;
	
	//reading humidity data
	for (uint8_t b = 0; b<16; b++)
	{
		//wait for high
		for(waitCounter = 200; waitCounter > 0 && !(PINB & (1<<PB3)); waitCounter--)
			_delay_us(1);

		if (waitCounter == 0 || waitCounter == 200) // timeout error
		{
			mError_dht += 8;
			break;
		}
		else if (waitCounter > 140) //50 us means start of bit
		;

		//wait for low
		for(waitCounter = 200; waitCounter > 0 && (PINB & (1<<PB3)); waitCounter--)
			_delay_us(1);

		data<<=1;
		if (waitCounter == 0 || waitCounter == 200) // timeout error
		{
			mError_dht += 16;
			break;
		}
		else if (waitCounter > 170) //26-28 us means 1
			data |= 1;
		else if (waitCounter > 120) //70 us means 0
			;
	}
	
	//reading temperature data
	for (uint8_t b = 0; b<16; b++)
	{
		//wait for high
		for(waitCounter = 200; waitCounter > 0 && !(PINB & (1<<PB3)); waitCounter--)
		_delay_us(1);

		if (waitCounter == 0 || waitCounter == 200) // timeout error
		{
			mError_dht += 32;
			break;
		}
		else if (waitCounter > 140) //50 us means start of bit
		;

		//wait for low
		for(waitCounter = 200; waitCounter > 0 && (PINB & (1<<PB3)); waitCounter--)
			_delay_us(1);

		tdata<<=1;
		if (waitCounter == 0 || waitCounter == 200) // timeout error
		{
			mError_dht += 64;
			break;
		}
		else if (waitCounter > 170) //26-28 us means 1
			tdata |= 1;
		else if (waitCounter > 120) //70 us means 0
			;
	}

	//reading checksum
	for (uint8_t b = 0; b<8; b++)
	{
		//wait for high
		for(waitCounter = 200; waitCounter > 0 && !(PINB & (1<<PB3)); waitCounter--)
			_delay_us(1);

		if (waitCounter == 0 || waitCounter == 200) // timeout error
		{
			mError_dht += 128;
			break;
		}
		else if (waitCounter > 140) //50 us means start of bit
			;
		
		//wait for low
		for(waitCounter = 200; waitCounter > 0 && (PINB & (1<<PB3)); waitCounter--)
			_delay_us(1);

		//	tdata<<=1;
		if (waitCounter == 0 || waitCounter == 200) // timeout error
		{
			mError_dht += 256;
			break;
		}
		else if (waitCounter > 170) //26-28 us means 1
			;//tdata |= 1;
		else if (waitCounter > 120) //70 us means 0
			;
	}

	//wait for high (line release)
	for(waitCounter = 200; waitCounter > 0 && !(PINB & (1<<PB3)); waitCounter--)
		_delay_us(1);

	if (waitCounter == 0 || waitCounter == 200) // timeout error
		mError_dht += 512;
	else if (waitCounter > 140) //50 us means end
		;
	// checking for error and assign vars
	
	mHumidity = -data/10.00;
	mCurrentTempIn_dht = -tdata/10.00;

	if (mHumidity <= 0.00 || mHumidity  > 100.0)
		mHumidity = NAN;

	if (mCurrentTempIn_dht < -200.0 || mCurrentTempIn_dht == 0.0 || mCurrentTempIn_dht > 200.0)
		mCurrentTempIn_dht = NAN;
}