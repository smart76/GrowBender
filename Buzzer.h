/*
 * Buzzer.h
 *
 * Created: 08.07.2017 15:20:06
 *  Author: smart
 */ 


#ifndef BUZZER_H_
#define BUZZER_H_

#include <stdio.h>
#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>

extern uint8_t mBuzzerMode;

void InitBuzzer();

void SetBuzzer(uint8_t pMode);
void ProcessBuzzer();


#endif /* BUZZER_H_ */