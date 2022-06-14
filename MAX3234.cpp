/*
 * MAX3234.cpp
 *
 * Created: 08.07.2017 14:52:59
 *  Author: smart
 */ 

#include "MAX3234.h"
#include "SPI.h"
#include "Buzzer.h"

void InitClock()
{
	InitSPI();
}

void GetTime(sTime &pTime)
{
	uint8_t sec, min, hrs;
	sec = SPI_read1(0x0);
	min = SPI_read1(0x1);
	hrs = SPI_read1(0x2);
	pTime.Second = (((sec&0xF0)>>4) * 10) + (sec&0x0F);
	pTime.Minute = (((min&0x70)>>4) * 10) + (min&0x0F);
	pTime.Hour = (((hrs&0x30)>>4) * 10) + (hrs&0x0F);
	
	if (pTime.Second > 59) // crutch. must be removed
	pTime.Second = 0;
	if (pTime.Minute > 59) // crutch. must be removed
	pTime.Minute = 0;
	if (pTime.Hour > 24) // crutch. must be removed
	{
		pTime.Hour = 0;
		SetBuzzer(3);
	}
}

void GetDate(sDate &pDate)
{
	uint8_t day, month, year;
	pDate.DayOfWeek = SPI_read1(0x03);
	day = SPI_read1(0x4);
	month = SPI_read1(0x5);
	year = SPI_read1(0x6);
	pDate.Day = (((day>>4)&0x03) * 10) + (day&0x0F);
	pDate.Month = (((month>>4)&0x01) * 10) + (month&0x0F);
	pDate.Year = (((year>>4)&0x0F) * 10) + (year&0x0F);

	if (pDate.DayOfWeek > 7 || pDate.DayOfWeek == 0) // crutch. must be removed
	pDate.DayOfWeek = 1;
	if (pDate.Day > 31 || pDate.Day == 0) // crutch. must be removed
	pDate.Day = 1;
	if (pDate.Month > 12 || pDate.Month == 0) // crutch. must be removed
	pDate.Month = 1;
	if (pDate.Year > 99 ) // crutch. must be removed
	pDate.Year = 0;
}

void SetClock(uint8_t pDateTimeUnit, uint8_t pValue)
{
	 switch (pDateTimeUnit)
	 {
		 case 0x81 : 
			SPI_write1(pDateTimeUnit, (((pValue / 10))<<4) + (pValue % 10));	 
		 break;
		 case 0x82 :
			SPI_write1(pDateTimeUnit, (((pValue/ 10))<<4) + (pValue % 10));
		 break;
		 case 0x83 :
			SPI_write1(pDateTimeUnit, pValue);
		 break;
		 case 0x84 :
			SPI_write1(pDateTimeUnit, (((pValue / 10))<<4) + (pValue % 10));
		 break;
		 case 0x85 :
			SPI_write1(pDateTimeUnit, (((pValue / 10))<<4) + (pValue % 10));
		 break;
		 case 0x86 :
			SPI_write1(pDateTimeUnit, (((pValue / 10))<<4) + (pValue % 10));
		 break;
		
	 }
	 
}