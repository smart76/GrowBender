/*
 * CPPFile1.cpp
 *
 * Created: 16.01.2016 23:11:26
 *  Author: smart
 */ 
#include "UART.h"
#include "ESP.h"
#include "EEPROMAddresses.h"
#include "Buzzer.h"
#include <avr/wdt.h>
#include <avr/eeprom.h>

//typedef enum {csNone, csOk, csError, csSendPrepare, csSendReady, csSendSuccess} CommandState ;
//typedef enum {esNone, esGotIP, esServerActive, esHardwareError} ESPState;
	
volatile CommandState mCommandState;
ESPState mESPState;
uint8_t mConnectionState[4];
volatile uint8_t mConnectionReportIndex[4];

bool mShotdownESPRequestReceived = false;
uint16_t ip1, ip2, ip3, ip4;

char* csATCommand = "AT+C";

bool WaitCommandState(CommandState pState)
{
	if (pState == csNone) return true; // remove when optimize
	
	mCommandState = csNone;

	uint16_t i;
	switch (pState)
	{
		case csReady: 
			i = 1000;
			break;
		default : 
			i = 1000;
			break;
	}
	while(i-- > 0)
	{

		ProcessSerialData();
		if (mCommandState == pState)
		{
			//PORTD &= ~(1<<PD4);
			//uart_puts("!");
			return true;
		}
		else if (mCommandState == csError)
		{

			
			//PORTD &= ~(1<<PD4);
			return false;
		}
		else
			_delay_ms(10);
	}
	//if (pState == csSendReady)
	//{
		//lcd_goto(0,1); 
		//lcd_puts("hw");
	//}
	//mESPState = esHardwareError;
	//uart_puts("HW error\r\n");
	//uart_putb(mUARTBuffer+mUARTTail, mUARTHead-mUARTTail);
	//PORTD &= ~(1<<PD4);
	return false;
}

bool WaitESPState(ESPState pState)
{
	//if (mESPState == pState)
	mESPState = esNone;
	
	for (uint16_t i=0; i<2000; i++)
	{
		ProcessSerialData();
		if (mESPState == pState)
			return true;
		else
			_delay_ms(10);
	}
	return false;
}

bool InitESP()
{
	_delay_ms(500);
	//uart_puts("AT\r\n");
	//mCommandState = csNone;
	//if (!WaitCommandState(csOk))
	//{
		//uart_puts("AT\r\n");
		//mCommandState = csNone;
		//if (!WaitCommandState(csOk))
			//return false;
	//}
	uart_puts("AT+RST\r\n");
	mCommandState = csNone;
	if (!WaitCommandState(csOk))
		return false;
	mCommandState = csNone;
	if (!WaitCommandState(csReady))
	{
		//uart_puts("AT+RST\r\n");
		//mCommandState = csNone;
		//WaitCommandState(csOk);
		//mCommandState = csNone;
		//if (!WaitCommandState(csReady))
			return false;
	}
	FlashBuffer();
	uart_puts("ATE0\r\n");
	mCommandState = csNone;
	if (!WaitCommandState(csOk))
		return false;
	uart_puts("AT+RFPOWER=1\r\n");
	mCommandState = csNone;
	if (!WaitCommandState(csOk))
		return false;
	uart_puts(csATCommand);
	uart_puts("IPMUX=1\r\n");
	mCommandState = csNone;
	
	if (!WaitCommandState(csOk))
		return false;
	uart_puts(csATCommand);
	uart_puts("WMODE_CUR=1\r\n");
	mCommandState = csNone;
	return WaitCommandState(csOk);
	//mUARTHead = mUARTTail = 0; ///////////////////////////////////
}

void DisconnectAP()
{
	uart_puts(csATCommand);
	uart_puts("WQAP\r\n");
	WaitCommandState(csOk);
}

bool JoinAP(char* pAPName, char* pPassword)
{
// 	if (mCommandState != csNone)
// 		DisconnectAP();

		
	uart_puts(csATCommand);
	uart_puts("WJAP_CUR=\"");
	uart_puts(pAPName);
	uart_puts("\",\"");
	uart_puts(pPassword);
	uart_puts("\"\r\n");
	mCommandState = csNone;
	return WaitCommandState(csOk);
}

void ReadIPStart()
{
	uart_puts(csATCommand);
	uart_puts("IFSR\r\n");
}

bool SetMUX(uint8_t pMUX)
{
	if (pMUX == 0)
	{
		uart_puts(csATCommand);
		uart_puts("IPMUX=0");
	}
	else
	{
		uart_puts(csATCommand);
		uart_puts("IPMUX=1");
	}
	return WaitCommandState(csOk);
}

bool StartServer(uint16_t pPort)
{
	uart_puts(csATCommand);
	uart_puts("IPSERVER=1,");
	uart_putNLn(pPort);
	mCommandState = csNone;
	if (!WaitCommandState(csOk))
	{
		mESPState = esServerActive;
		return true;
	}
	else
	{
		mESPState = esNone;
		return false;
	}
}

bool StopServer()
{
	uart_puts(csATCommand);
	uart_puts("IPSERVER=0\r\n");
	mCommandState = csNone;
	return WaitCommandState(csOk);
}

bool TCPConnect(uint8_t pConnID, char* pAddress, int pPort)
{
	uart_puts(csATCommand);
	uart_puts("IPSTART=");
	uart_putN(pConnID);
	uart_puts(",\"TCP\",\"");
	uart_puts(pAddress);
	uart_puts("\",");
	uart_putNLn(pPort);
	return WaitCommandState(csOk);
}

bool TCPDisconnect(uint8_t pConnID)
{
	uart_puts(csATCommand);
	uart_puts("IPCLOSE=");
	uart_putNLn(pConnID);
	return WaitCommandState(csOk);
}

bool TCPSendDataStr(uint8_t pConnID, char* pStrData)
{
	uart_puts(csATCommand);
	uart_puts("IPSEND=");
	uart_putN(pConnID);
	uart_putc(',');
	uart_putNLn(strlen(pStrData));
	mCommandState = csSendPrepare;
// 	if (WaitCommandState(csSendReady))
// 	{
		uart_puts(pStrData);
		return WaitCommandState(csSendSuccess);
//	}
// 	else
// 		return false;
}

uint16_t ProcessApplicationData(uint8_t pConnectionIndex, char* pData, uint16_t pDataLen)
{
	mNeedMenuRedraw = true;
	if (pDataLen >= 8 && memcmp(pData, "#setb:", 6)==0) //#set:<ParmID - 1 byte><parm val - N bytes, depend on ParmID>
	{
		switch(pData[6])
		{
			case 0: 
				mConnectionReportIndex[pConnectionIndex] = pData[7];
				break;
			case EEPROM_ADDRESS_LIGHTS:
				SetLight(pData[7]);
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_LIGHTS, mLigntMode);
				break;
			case EEPROM_ADDRESS_FAN:
				SetFan(pData[7]);
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_FAN, mFanMode);
				break;
			case EEPROM_ADDRESS_VENT:
				SetVentFan(pData[7]);
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_VENT, mVentMode);
				break;
			case EEPROM_ADDRESS_HEATER:
				SetHeater(pData[7]);
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_HEATER, mHeaterMode);
				break;
			case EEPROM_ADDRESS_HUMIDIFIER:
				SetHumidifyer(pData[7]);
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_HUMIDIFIER, mHumidifyerMode);
				break;
			case EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MIN:
				mMaintainTempDayMin = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MIN, mMaintainTempDayMin);
				break;
			case EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MAX:
				mMaintainTempDayMax = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MAX, mMaintainTempDayMax);
				break;
			case EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MIN:
				mMaintainTempNightMin = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MIN, mMaintainTempNightMin);
				break;
			case EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MAX:
				mMaintainTempNightMax = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MAX, mMaintainTempNightMax);
				break;
			case EEPROM_ADDRESS_MAINTAIN_HUMIDITY:
				mMaintainHumidity = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_HUMIDITY, mMaintainHumidity);
				break;
			case EEPROM_ADDRESS_DELTA_TEMP:
				mDeltaTemp = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_DELTA_TEMP, mDeltaTemp);
				break;
			case EEPROM_ADDRESS_AUTO_MODE:
				mAutoMode = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_AUTO_MODE, mAutoMode);
				break;
			case EEPROM_ADDRESS_TEMP_SENSOR:
				mTempSensor = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_TEMP_SENSOR, mTempSensor);
				break;
			case EEPROM_ADDRESS_CRITICAL_TEMP:
				mCriticalTemp = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_CRITICAL_TEMP, mCriticalTemp);
				break;
			case EEPROM_ADDRESS_COOLING_MODE:
				mCoolingMode = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_COOLING_MODE, mCoolingMode);
				break;
			case EEPROM_ADDRESS_NIGHT_START_H:
				mNightStartTime.Hour = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_START_H, mNightStartTime.Hour);
				break;
			case EEPROM_ADDRESS_NIGHT_START_M:
				mNightStartTime.Minute = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_START_M, mNightStartTime.Minute);
				break;
			case EEPROM_ADDRESS_NIGHT_DURATION_START:
				mNightDurationStart = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_START, mNightDurationStart);
				break;
			case EEPROM_ADDRESS_NIGHT_DURATION_END:
				mNightDurationEnd = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_END, mNightDurationEnd);
				break;
			case EEPROM_ADDRESS_NIGHT_DURATION_DELTA:
				mNightDurationDelta = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_DELTA, mNightDurationDelta);
				break;
			case EEPROM_ADDRESS_NIGHT_MODE:
				mNightMode = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_MODE, mNightMode);
				break;
			case EEPROM_ADDRESS_GROW_START_DAY:
				mGrowStartDate.Day = pData[8];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_GROW_START_DAY, mGrowStartDate.Day);
				break;
			case EEPROM_ADDRESS_GROW_START_MONTH:
				mGrowStartDate.Month = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_GROW_START_MONTH, mGrowStartDate.Month);
				break;
			case EEPROM_ADDRESS_GROW_START_YEAR:
				mGrowStartDate.Year = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_GROW_START_YEAR, mGrowStartDate.Year);
				break;
			case EEPROM_ADDRESS_GROW_DURATION:
				mGrowDuration = pData[7];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_GROW_DURATION, mGrowDuration);
				break;
			case CMD_ADDRESS_CLOCK:
			case CMD_ADDRESS_CLOCK + 1:
			case CMD_ADDRESS_CLOCK + 2:
			case CMD_ADDRESS_CLOCK + 3:
			case CMD_ADDRESS_CLOCK + 4:
			case CMD_ADDRESS_CLOCK + 5:
				SetClock(pData[6], pData[7]);
				GetDate(mCurrentDate);
				break;				
		}
		return 8;
	}
	else if (pDataLen >= 9 && memcmp(pData, "#setw:", 6)==0) //#set:<ParmID - 1 byte><parm val - N bytes, depend on ParmID>
	{
		switch(pData[6])
		{
			case EEPROM_ADDRESS_NIGHT_DURATION_CURR: // uint16
				mNightDurationCurr = ((uint16_t*)(pData+7))[0];
				eeprom_write_word((uint16_t*)EEPROM_ADDRESS_NIGHT_DURATION_CURR, mNightDurationCurr);
		
				break;
		}
		return 9;
	}
	else if (pDataLen >= 8 && memcmp(pData, "#sets:", 6)==0)
	{
		if (pDataLen >= pData[7]+8) // ensure whole string received
		{
			switch(pData[6])
			{
				case 0: // access point name
					eeprom_write_block(pData+7, (void*)EEPROM_ADDRESS_AP_NAME, pData[7]+1); // extra byte for len
				break;
				case 1: // access point name
					eeprom_write_block(pData+7, (void*)EEPROM_ADDRESS_AP_PASSWORD, pData[7]+1); 
				break;
			}
		}
		return 8 + pData[7];
	}
	else if (pDataLen >= 8 && memcmp(pData, "#restart", 8)==0)
	{
		wdt_enable(WDTO_15MS);
		return 8;
	}
	else
		return 0;
}
uint8_t tmOkCount = 0;
void ProcessSerialData()
{
	volatile uint16_t pos = mUARTTail;
	int dataLen;
	//cli();

	while ((dataLen = (int)mUARTHead - pos) >= MINPACKETLEN)
	{
		if  ((mUARTBuffer[pos] == '\r' && mUARTBuffer[pos + 1] == '\n') || ((mUARTBuffer[pos] >= '0' && mUARTBuffer[pos] < '9') && mUARTBuffer[pos + 1] == ',') || (mUARTBuffer[pos] == '+' && mUARTBuffer[pos + 1] == 'C'))
		{
			if(dataLen >= 4 && memcmp(mUARTBuffer+pos+2, "OK", 2)==0)
			{
				tmOkCount++;
				mCommandState = csOk;
				mUARTTail = pos += 4;
			}
			else if(dataLen >= 7 && memcmp(mUARTBuffer+pos+2, "ready", 5)==0)
			{
				mCommandState = csReady;
				mUARTTail = pos += 7;
			}			
			else if(dataLen >= 11 && memcmp(mUARTBuffer+pos+2, "SEND OK\r\n", 9)==0)
			{
				mCommandState = csSendSuccess;
				mUARTTail = pos += 11;
			}
			else if(dataLen >= 13 && memcmp(mUARTBuffer+pos+2, "SEND FAIL\r\n", 11)==0)
			{
				mCommandState = csSendFail;
				mUARTTail = pos += 13;
			}
			else if(/*mCommandState == csSendPrepare && */dataLen >= 1 && memcmp(mUARTBuffer+pos+2, ">", 1)==0)
			{
				mCommandState = csSendReady;
				mUARTTail = pos += 1;
			}
			else if(dataLen >= 7 && memcmp(mUARTBuffer+pos+2, "ERROR", 5)==0)
			{
				mCommandState = csError;
				mUARTTail = pos += 7;
			}
			else if(dataLen >= 8 && memcmp(mUARTBuffer+pos+2, "busy p", 6)==0)
			{
				mCommandState = csBusyp;
				mUARTTail = pos += 8;
			}

			else if(dataLen >= 6 && memcmp(mUARTBuffer+pos+2, "FAIL", 4)==0)
			{
				mCommandState = csError;
				mUARTTail = pos += 4;
			}
			else if(dataLen >= 10 && memcmp(mUARTBuffer+pos+1, ",CLOSED", 7)==0) // skipping conn id
			{
				mConnectionState[mUARTBuffer[pos] - '0'] = 0;
				mUARTTail = pos += 10;
			}
			else if(dataLen >= 14 /*pos is + 1 in tis case*/ && memcmp(mUARTBuffer+pos+1, ",CONNECT FAIL", 13)==0) // skipping conn id
			{
				mConnectionState[mUARTBuffer[pos] - '0'] = 0;
				mUARTTail = pos += 16; // including \r\n
			}				
			else if(dataLen >= 13 && memcmp(mUARTBuffer+pos+2, "WIFI GOT IP", 11) == 0) 
			{
				mESPState = esGotIP;
				mNeedToReedIP = 1;
				mUARTTail = pos += 13;
                uart_puts("got ip\r\n");
			}
			else if(dataLen >= 16 && memcmp(mUARTBuffer+pos+2, "+CIFSR:STAIP", 12) == 0)
			{
				sscanf((char*)(mUARTBuffer+pos+16), " %d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
				//sscanf("11.22.33.44", "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
				mUARTTail = pos += 18; // need to calculate but too lazy since it is one time operation //strchr((char*)mUARTBuffer+pos+18, '\r') - (char*)mUARTBuffer + pos;
                                uart_puts("got ip\r\n");
			}
			else if(dataLen >= 11 && memcmp(mUARTBuffer+pos+1, ",CONNECT", 8)==0) // skipping conn id
			{
				mConnectionState[mUARTBuffer[pos] - '0'] = 1;
				mUARTTail = pos += 11;
			}
			// rn+IPD,0,26:
			else if(dataLen >= 7 && memcmp(mUARTBuffer+pos+2, "+IPD,", 5)==0)
			{
				char* colChar = strchr((char*)mUARTBuffer+pos+7, ':');
				*colChar = 0; //mUARTBuffer[pos+7+idx] = 0; // make string zero-terminated at the end of length
				int recvDataLen = atoi((char*)mUARTBuffer+pos+9); // see comment below where var discards
				*colChar = ':';

				if (dataLen >= recvDataLen + colChar - (char*)mUARTBuffer - pos )
				{
				//lcd_goto(0,0); lcd_putc('7');
					uint8_t byteCount = 0;
					uint8_t dataPos = 0;
					uint16_t w = 0;
 					do 
 					{
						byteCount = ProcessApplicationData(mUARTBuffer[pos+7] - '0', colChar + 1 + dataPos, recvDataLen - dataPos);
						dataPos += byteCount;

					} while (byteCount != 0);
					
					pos += recvDataLen + 11; // expecting only whole processable packets
					if (recvDataLen > 10) // skipping lehgth additional positions
						pos++;
					if (recvDataLen > 100) // skipping lehgth additional positions (are not expecting more than 1000 bytes per msg
						pos++;
					if (pos >= mUARTHead)
						pos = mUARTHead;
					// should use length to skip unknown data
					mUARTTail = pos;
				}
				else
				{
					//char str[5];
					//itoa(mUARTHead, str, 10);
//
					//lcd_goto(0,0);
//
					//lcd_puts(str);
					//lcd_putc(' ');
//
					//itoa(mUARTTail, str, 10);
					//lcd_puts(str);
					//lcd_putc(' ');
					//
					//itoa((colChar - (char*)mUARTBuffer), str, 10);
					//lcd_puts(str);
					//lcd_putc(' ');
					//lcd_goto(0,1);
					pos = mUARTHead;
				}
			}
			else
				pos++;
				
			if (mUARTTail >= mUARTHead)
			{
				mUARTHead = mUARTTail = 0;
				break;
			}
		}
		else
			pos++;
				
	
	}			
	sei();	
	/*if (mUARTTail != mUARTHead)
	{
		char str[5];
		itoa(mUARTHead, str, 10);

		lcd_goto(0,0);

		lcd_puts(str);
		lcd_putc(' ');

		itoa(mUARTTail, str, 10);
		lcd_puts(str);
		lcd_putc(' ');
		
		itoa(pos, str, 10);
		lcd_puts(str);
		lcd_putc(' ');	
		lcd_goto(0,1);
		for (int i =  mUARTTail; i<mUARTHead && i - mUARTTail < 20; i++)
			lcd_putc(mUARTBuffer[i]);
		if (mUARTHead - mUARTTail > 20)
		{
			lcd_goto(0,2);
			for (int i =  mUARTTail + 20; i<mUARTHead && i - mUARTTail < 40; i++)
				lcd_putc(mUARTBuffer[i]);

			if (mUARTHead - mUARTTail > 40)
			{
				lcd_goto(0,3);
				for (int i =  mUARTTail + 40; i<mUARTHead && i - mUARTTail < 60; i++)
				lcd_putc(mUARTBuffer[i]);
			}
		}
		lcd_putc('!');	
	}*/

} 

bool SendData(uint8_t pConnID, char* pData, uint16_t pLength)
{
	mCommandState = csSendPrepare;
	uart_puts(csATCommand);
	uart_puts("IPSEND=");
	uart_putN(pConnID);
	uart_putc(',');
	uart_putNLn(pLength);
 //	if (
	WaitCommandState(csSendReady);
//	 )
// 	{
		//_delay_ms(100);
	uart_putb((uint8_t*)pData, pLength);
	return WaitCommandState(csSendSuccess);
// 	}
// 	else
// 		return false;
}


