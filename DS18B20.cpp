/*
 * DS18B20.cpp
 *
 * Created: 10.09.2016 0:58:12
 *  Author: smart
 */ 

// INTRODUCTION
// A  host  CPU  can  easily  generate  1-Wire  timing
// signals  itself  if  a  true  bus  master  is  not  present.
// This  is  an  example,  written  in  C,  of  how  to
// perform  the  communication.  There  are  several
// system  requirements  for  proper  operation  of  the
// code:
// 1.
// The communication port is bi-directional, its
// output  is  open  drain,  and  there  is  a  weak
// pull-up  on  the  line.  This  is  a  requirement  of
// any 1-Wire bus.
// 2.
// The   system   is   capable   of   generating   an
// accurate and repeatable 1us delay.
// This  code  relies  on  the  two  C  functions  “outp”
// and “inp” to write and read bytes of data to port
// locations.  They  are  located  in  the  <conio.h>
// standard library. Usage:
// int outp(unsigned port, int byte);
// int inp(unsigned port);
// The  constant  PORTADDRESS  in  the  code  is
// defined  as  the  location  of  the  communication
// port.  The  code  assumes  bit  0  of  this  location  is
// the 1-Wire bus.
// The  function  “Waitx”  in  this  example  is  a  user-
// generated  routine  to  wait  a  variable  number  of
// microseconds.  This  function  will  vary  for  each
// unique hardware platform running this code so it
// cannot possibly be described here. Usage:
// //Pause for exactly x microseconds
// void Waitx(int microseconds){}
// FUNCTIONS
// The four basic operations of a 1-Wire bus (Reset,
// Write  1,  Write  0,  and  Read  Bit)  are  listed  next.
// They   will   be   used   to   build   more   complex
// functions later.
// // Generate a 1-Wire reset, return 1 if no
// // presence detect was found, return 0 otherwise.
#include "DS18B20.h"
void InitTempSensor()
{
	Reset();
	WriteByte(0xCC);   //Skip ROM
	WriteByte(0x4E);   //write scratchpad
	WriteByte(0x00);   //TL
	WriteByte(0x00);   //TH
	WriteByte(0b01111111); //CONFIG 12bit sampling
	
	Reset();
	WriteByte(0xCC);   //Skip ROM
	WriteByte(0x44);   //convert
	WaitConversion();
}

bool WaitConversion()
{ 
	uint16_t delayCount = 0;
	while(Readx() && ++delayCount < WAIT_CONVERSION_TIMEOUT)
		_delay_ms(1);
	if (delayCount == WAIT_CONVERSION_TIMEOUT)	
		return false;
	else
		return true;
}
// shared vars for 2 methods
//uint16_t a, b;
double retVal;
float GetTemperature1()
{
	Reset();
	WriteByte(0x55);   //match ROM
	WriteByte(SCODE10);
	WriteByte(SCODE11);
	WriteByte(SCODE12);
	WriteByte(SCODE13);
	WriteByte(SCODE14);
	WriteByte(SCODE15);
	WriteByte(SCODE16);
	WriteByte(SCODE17);
	// 		WriteByte(0x33);   //Read ROM
	WriteByte(0x44);   //convert
	if ( WaitConversion())
	{
		Reset();
		WriteByte(0x55);   //match ROM
		WriteByte(SCODE10);
		WriteByte(SCODE11);
		WriteByte(SCODE12);
		WriteByte(SCODE13);
		WriteByte(SCODE14);
		WriteByte(SCODE15);
		WriteByte(SCODE16);
		WriteByte(SCODE17);
		WriteByte(0xBE);   //Read memory
		//WriteByte(0x00);   //Page 1
		//following code reads 2 bytes in sequence and convert it to double decimal
		//a = ReadByte();
		//b = ReadByte();
		retVal = (ReadByte() + ReadByte() * 256) * 0.0625;
		Write1(); // skipping
		if (retVal > 200 || retVal < -100 || retVal == 85.0)
			return NAN;
		else
			return retVal;
	}
	else
		return NAN;
}

float GetTemperature2()
{
	Reset();
	WriteByte(0x55);   //match ROM
	WriteByte(SCODE20);
	WriteByte(SCODE21);
	WriteByte(SCODE22);
	WriteByte(SCODE23);
	WriteByte(SCODE24);
	WriteByte(SCODE25);
	WriteByte(SCODE26);
	WriteByte(SCODE27);
	// 		WriteByte(0x33);   //Read ROM
	WriteByte(0x44);   //convert
	if (WaitConversion())
	{
		Reset();
		WriteByte(0x55);   //match ROM
		WriteByte(SCODE20);
		WriteByte(SCODE21);
		WriteByte(SCODE22);
		WriteByte(SCODE23);
		WriteByte(SCODE24);
		WriteByte(SCODE25);
		WriteByte(SCODE26);
		WriteByte(SCODE27);
		WriteByte(0xBE);   //Read memory
		//WriteByte(0x00);   //Page 1

		//a = ReadByte();
		//b = ReadByte();
		retVal = (ReadByte() + ReadByte() * 256) * 0.0625;
		Write1(); // skipping
		if (retVal > 200 || retVal < -100 || retVal == 85.0)
			return NAN;
		else
			return retVal;
	}
	else
		return NAN;
}

uint8_t Reset(void)
{
	uint8_t result;
	SENSORPORTDIR |= (1<<SENSORPIN); PORTC &= ~(1<<SENSORPIN); //Drives DQ low
	_delay_us(480);
	
	SENSORPORTDIR  &= ~((1<<SENSORPIN)); PORTC |= (1<<SENSORPIN); //Releases the bus
	
	_delay_us(120);
	//Sample and return the Presence Detect
	result = PINC & (1<<SENSORPIN);
	_delay_us(360);
	return result;
}

void WriteByte(uint8_t pByte)
{
	for (int i =0; i<8; i++)
	{
		if (pByte & 0x01)
			Write1();	
		else
			Write0();
		pByte >>= 1;
	}
}

void Write1(void)
{
	SENSORPORTDIR  |= (1<<SENSORPIN); PORTC &= ~(1<<SENSORPIN); //Drives DQ low
	_delay_us(1);
	SENSORPORTDIR  &= ~((1<<SENSORPIN)); PORTC |= (1<<SENSORPIN); //Releases the bus
	_delay_us(59);
}
//Generate a Write0. I'm giving a 5us recovery
//time in case the rise time of your system is
//slower than 1us. This will not affect system
//performance.
void Write0(void)
{
	SENSORPORTDIR  |= (1<<SENSORPIN); PORTC &= ~(1<<SENSORPIN); //Drives DQ low
	_delay_us(55);
	SENSORPORTDIR  &= ~((1<<SENSORPIN)); PORTC |= (1<<SENSORPIN); //Releases the bus
	_delay_us(5);
}

// Read data byte and return it
uint8_t ReadByte(void)
{
	uint8_t result=0;
	for(uint8_t loop=0; loop<8; loop++)
	{
		result >>= 1;
		if (Readx())
			result |= 0x80;
	}
	return result;
}

// Read 1 bit from the bus and return it
uint8_t Readx(void)
{
	uint8_t result;
	SENSORPORTDIR  |= (1<<SENSORPIN); PORTC &= ~(1<<SENSORPIN); //Drives DQ low
	_delay_us(4);
	SENSORPORTDIR  &= ~((1<<SENSORPIN)); PORTC |= (1<<SENSORPIN); //Releases the bus
	_delay_us(14);
	//Sample after 15us
	result = PINC & (1<<SENSORPIN);
	_delay_us(45);
	//uart_putN(result);
	return result;
}


//uint64_t ReadSensorId()
//{
	//
//}
/*
//This is all that is required to do bit-wise manipulation of the bus, however the above routines can be built
//upon to create byte-wise manipulator functions.
// Write data byte
void WriteByte(int Data)
{
	int loop;
	//Do 1 complete byte
	for(loop=0; loop<8; loop++)
	{
		//0x01,0x02,0x04,0x08,0x10,ect.
		if(Data & (0x01<<loop))
		Write1();
		else
		Write0();
	}
}

// These six functions plus the user’s “Waitx” function are all that are required for control of the 1-Wire bus
// at byte level. The following example shows how a user would bring those functions together by reading
// the ICA register of a DS2437.
//Read the Integrated Current Accumulator
int ReadICA(void)
{
	// Recall Page 1
	if(Reset()) return 0;  //No devices found
	WriteByte(0xCC);   //Skip ROM
	WriteByte(0xB8);   //Recall memory
	WriteByte(0x01);   //Page 1
	// Read Page 1
	Reset();
	WriteByte(0xCC);   //Skip ROM
	WriteByte(0xBE);   //Read memory
	WriteByte(0x01);   //Page 1
	ReadByte();        //Ignore first 4 bytes
	ReadByte();
	ReadByte();
	ReadByte();
	return ReadByte(); // The ICA register
}*/