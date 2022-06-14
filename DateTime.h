/*
 * DateTime.h
 *
 * Created: 23.07.2017 11:27:08
 *  Author: smart
 */ 


#ifndef DATETIME_H_
#define DATETIME_H_

struct sTime
{
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Second;
};
struct sDate
{
	uint8_t DayOfWeek;
	uint8_t Day;
	uint8_t Month;
	uint8_t Year;
};


#endif /* DATETIME_H_ */