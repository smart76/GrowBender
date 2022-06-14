/*
 * DS18B20.h
 *
 * Created: 10.09.2016 1:00:44
 *  Author: smart
 */ 


#ifndef DS18B20_H_
#define DS18B20_H_

#include <avr/io.h>
#include <util/delay.h>

#define SENSORPORTDIR _SFR_IO8(0x07) // DDRC    
#define SENSORPORT PORTC
#define SENSORPIN PC3

#define WAIT_CONVERSION_TIMEOUT 100

#if DEVICE0

/*#define SCODE17 0x8b
#define SCODE16 0x01
#define SCODE15 0x16
#define SCODE14 0x15
#define SCODE13 0x69
#define SCODE12 0x80
#define SCODE11 0xee
#define SCODE10 0x28
*/
//wo knot
#define SCODE17 0x31
#define SCODE16 0x04
#define SCODE15 0x16
#define SCODE14 0xa1
#define SCODE13 0x73
#define SCODE12 0x73
#define SCODE11 0xff
#define SCODE10 0x28

#define SCODE27 0x43
#define SCODE26 0x02
#define SCODE25 0x16
#define SCODE24 0x12
#define SCODE23 0x92
#define SCODE22 0x6f
#define SCODE21 0xee
#define SCODE20 0x28
#endif

#if DEVICE1
/*#define SCODE17 0x53
#define SCODE16 0x01
#define SCODE15 0x16
#define SCODE14 0x15
#define SCODE13 0x93
#define SCODE12 0x5d
#define SCODE11 0xee
#define SCODE10 0x28*/

// w fnot
#define SCODE17 0x05
#define SCODE16 0x04
#define SCODE15 0x16
#define SCODE14 0xa1
#define SCODE13 0x6e
#define SCODE12 0xdd
#define SCODE11 0xff
#define SCODE10 0x28

#define SCODE27 0x05
#define SCODE26 0x02
#define SCODE25 0x16
#define SCODE24 0x12
#define SCODE23 0x94
#define SCODE22 0xdf
#define SCODE21 0xee
#define SCODE20 0x28
#endif


void InitTempSensor();
uint8_t Reset(void);
void WriteByte(uint8_t pByte);
void Write1(void);
void Write0(void);
uint8_t ReadByte(void);
uint8_t Readx(void);
float GetTemperature1();
float GetTemperature2();
bool WaitConversion();

#endif /* DS18B20_H_ */