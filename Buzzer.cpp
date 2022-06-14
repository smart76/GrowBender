/*
 * Buzzer.cpp
 *
 * Created: 08.07.2017 15:20:18
 *  Author: smart
 */ 
#include "Buzzer.h"
uint8_t mBuzzerCounter;
/*
0 - off;
1 - short once;
2 - double short once;
3 - long once;
4 - short continous;
5 - double short continous;
6 - long continous;
*/
uint8_t mBuzzerMode;

void InitBuzzer()
{
	DDRB |= (1<<PB4);
	//PORTB |= (1<<PB4);
	
}

void SetBuzzer(uint8_t pMode)
{
	mBuzzerMode = pMode;
	mBuzzerCounter = 0;
}
void ProcessBuzzer()
{
	return;
	mBuzzerCounter++;
	switch(mBuzzerMode)
	{
		case 1:
		case 4:
		case 6:
		if (mBuzzerCounter == 1)
		PORTB |= (1<<PB4);
		break;
		case 2:
		case 5:
		if (mBuzzerCounter == 1 || mBuzzerCounter == 2)
		PORTB |= (1<<PB4);
		break;
		case 3:
		if (mBuzzerCounter == 1)
		PORTB |= (1<<PB4);
		break;
	}
	_delay_ms(50);
	if ((mBuzzerMode != 3 && mBuzzerMode != 6) || mBuzzerCounter == 3) // IF NOT long tone - 3 cycles
	PORTB &= ~(1<<PB4);
	
	if (mBuzzerCounter > 22)
	{
		if (mBuzzerMode < 4)
		mBuzzerMode = 0;
		mBuzzerCounter = 0;
	}
}
