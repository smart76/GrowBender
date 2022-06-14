/*
 * MAX3234.h
 *
 * Created: 08.07.2017 14:53:18
 *  Author: smart
 */ 


#ifndef MAX3234_H_
#define MAX3234_H_
#include <stdio.h>
#include "DateTime.h"

void InitClock();
void SetClock(uint8_t pDateTimeUnit, uint8_t pValue);
void GetTime(sTime &pTime);
void GetDate(sDate &pDate);


#endif /* MAX3234_H_ */