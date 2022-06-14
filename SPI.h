#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>
#include "SPI.h"
#include "UART.h"
//#include "General.h"

#define CSCLK PB5 //PC0
#define MOSI PC0 //PC1
#define MISO PC1 //PC2
#define SCK PC2 //PB5

extern volatile char mSPIData[4];

void InitSPI();
void SPI_AI_write(char byte);
void SPI_write1(char address, char pValue);
uint8_t SPI_read1(char address);
void ExecCommand(char address);
