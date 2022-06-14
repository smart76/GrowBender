/*
 * SPI.cpp
 *
 * Created: 04.11.2013 12:51:08
 *  Author: smart
 */ 
#include "SPI.h"

#include <math.h>

volatile char mSPIData[4];
volatile uint16_t i;
#define SPIIMUDELAY 50
#define SPIWAIT
void wait_ns(uint16_t ns)
{
	_delay_us(SPIIMUDELAY);
	// following doesn't dealy!
	//for(uint16_t i = 0; i <= 60000; i++)
	//	;
}
/*
#define CSCLK PB3
#define MOSI PC5
#define MISO PC4
#define SCK PD2
*/
void InitSPI (void)
{

	DDRB |= (1<<CSCLK);
	DDRC |= (1<<MOSI);
	DDRC &= ~(1<<MISO);
	DDRC |= (1<<SCK);
	PORTB |= (1<<CSCLK);
	PORTC |= (1<<MOSI) | (1<<MISO);
	//TCCR2B = (1<<CS21); //Set Prescaler to 8. CS21=1
}

/*
// 
// void SPI_AI_write(char byte)
// {
// 	unsigned char SPICount;                                       // Counter used to clock out the data
// 
// 	unsigned char SPIData;                                        // Define a data structure for the SPI data
// 	
// 	PORTB |= (1<<CSOSD);                                        		// Make sure we start with active-low CS high
// 	PORTB &= ~(1<<SCK);                                       		// and CK low
// 	
// 	PORTB &= ~(1<<CSOSD);                                           // Set active-low CS low to start the SPI cycle
// 
// 	// Repeat for the Data byte
// 	SPIData = byte;                                            // Preload the data to be sent with Data
// 	
// 	for (SPICount = 0; SPICount < 8; SPICount++)
// 	{
// 		if (SPIData & 0x80) PORTB |= (1<<MOSI);
// 
// 		else PORTB &= ~(1<<MOSI);
// 		
// 		PORTB |= (1<<SCK);
// 	wait_ns(SPIIMUDELAY);
// 		PORTB &= ~(1<<SCK);
// 		
// 		SPIData <<= 1;
// 	}
// 	
// 	PORTB |= (1<<CSOSD);
// 	PORTB &= ~(1<<MOSI);
// }
*/
void SPI_write1(char address, char pValue)
{
	unsigned char SPICount = 0;                                       // Counter used to clock out the data
	
	PORTB |= (1<<CSCLK);                                        	// Make sure we start with active-low CS high
	PORTC &= ~(1<<SCK);                                       		// and CK low
_delay_ms(1);	
	PORTB &= ~(1<<CSCLK);                                           // Set active-low CS low to start the SPI cycle
//		PORTC &= ~(1<<MOSI);
		
	for (SPICount = 0; SPICount < 8; SPICount++)                  // Prepare to clock out the Address byte
	{
		if (address & 0x80) PORTC |= (1<<MOSI);           				// Check for a 1 and set the MOSI line appropriately

		else PORTC &= ~(1<<MOSI);
		
		PORTC |= (1<<SCK);    
_delay_us(SPIIMUDELAY);                                  // Toggle the clock line
		PORTC &= ~(1<<SCK);
		
		address <<= 1;                                              // Rotate to get the next bit
	}                                                             // and loop back to send the next bit
	
_delay_ms(1);

	for (SPICount = 0; SPICount < 8; SPICount++)
	{
		if (pValue & 0x80)
			PORTC |= (1<<MOSI);

		else PORTC &= ~(1<<MOSI);
		
		PORTC |= (1<<SCK);
_delay_us(SPIIMUDELAY);

		PORTC &= ~(1<<SCK);
		
		pValue <<= 1;
	}
	PORTB |= (1<<CSCLK);
	PORTC &= ~(1<<MOSI);
_delay_ms(1);
}

uint8_t SPI_read1(char address)
{
	uint8_t SPICount;                                       // Counter used to clock out the data
	uint8_t res =0; 
	//IsSending = true;
	PORTB |= (1<<CSCLK);                                        	// Make sure we start with active-low CS high
	PORTC &= ~(1<<SCK);                                       		// and CK low
	PORTB &= ~(1<<CSCLK);                                           // Set active-low CS low to start the SPI cycle
	PORTC &= ~(1<<MOSI);

	for (SPICount = 0; SPICount < 8; SPICount++)                  // Prepare to clock out the Address byte
	{
		if (address & 0x80)
			PORTC |= (1<<MOSI);           				// Check for a 1 and set the MOSI line appropriately
		else
			PORTC &= ~(1<<MOSI);
		
		PORTC |= (1<<SCK);
		_delay_us(SPIIMUDELAY);
		PORTC &= ~(1<<SCK);
		address <<= 1;                                              // Rotate to get the next bit
	}                                                             // and loop back to send the next bit
	PORTC &= ~(1<<MOSI);

	for (SPICount = 0; SPICount < 8; SPICount++)                  // Prepare to clock in the data to be read
	{
		res <<=1;

		PORTC |= (1<<SCK);								// Raise the clock to clock the data out
		_delay_us(SPIIMUDELAY);
		if (PINC & (1<<MISO))
			res |= 1;                   // Read the data bit

		// Drop the clock ready for the next bit
		PORTC &= ~(1<<SCK);
	}
	PORTB |= (1<<CSCLK);                                                 // Raise CS
	return res;
}

void ExecCommand(char address)
{
	uint8_t SPICount;                                       // Counter used to clock out the data
	//IsSending = true;
	PORTB |= (1<<CSCLK);                                        	// Make sure we start with active-low CS high
	PORTC &= ~(1<<SCK);                                       		// and CK low
	_delay_us(SPIIMUDELAY);
	PORTB &= ~(1<<CSCLK);                                           // Set active-low CS low to start the SPI cycle
	_delay_us(SPIIMUDELAY);
	PORTC &= ~(1<<MOSI);
	uint8_t dummy = 1;
	for (SPICount = 0; SPICount < 8; SPICount++)                  // Prepare to clock out the Address byte
	{
		if (dummy & 0x80)
		PORTC |= (1<<MOSI);           				// Check for a 1 and set the MOSI line appropriately
		else
		PORTC &= ~(1<<MOSI);
		
		// Toggle the clock line
		wait_ns(SPIIMUDELAY);
		PORTC |= (1<<SCK);
		wait_ns(SPIIMUDELAY);
		PORTC &= ~(1<<SCK);
		
		dummy <<= 1;                                              // Rotate to get the next bit
	}
	_delay_us(SPIIMUDELAY);
	for (SPICount = 0; SPICount < 8; SPICount++)                  // Prepare to clock out the Address byte
	{
		if (address & 0x80)
		PORTC |= (1<<MOSI);           				// Check for a 1 and set the MOSI line appropriately
		else
		PORTC &= ~(1<<MOSI);
		
		// Toggle the clock line
		_delay_us(SPIIMUDELAY);
		PORTC |= (1<<SCK);
		_delay_us(SPIIMUDELAY);
		PORTC &= ~(1<<SCK);
		
		address <<= 1;                                              // Rotate to get the next bit
	}                                                             // and loop back to send the next bit
	PORTC &= ~(1<<MOSI);
	
	_delay_us(SPIIMUDELAY);
	//IsSending = false;
	for(int byteCount = 3; byteCount >= 0; byteCount--)
	{
		mSPIData[byteCount] = 0;
		for (SPICount = 0; SPICount < 8; SPICount++)                  // Prepare to clock in the data to be read
		{
			mSPIData[byteCount] <<=1;

			PORTC |= (1<<SCK);								// Raise the clock to clock the data out
			_delay_us(SPIIMUDELAY);
			if (PINC & (1<<MISO))
			mSPIData[byteCount] |= 1;                   // Read the data bit

			// Drop the clock ready for the next bit
			PORTC &= ~(1<<SCK);
			_delay_us(SPIIMUDELAY);
		}
		//monitor_putN(mSPIData[byteCount]);
	}                                       // and loop back
	PORTC |= (1<<CSCLK);                                                 // Raise CS
}
