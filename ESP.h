/*
 * ESP.h
 *
 * Created: 25.01.2016 22:13:15
 *  Author: smart
 */ 

#ifndef ESP_H_
#define ESP_H_
#define MINPACKETLEN 3
#define F_CPU 16000000UL
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "UART.h"
#include "DateTime.h"

typedef enum {csNone, csOk, csReady, csError, csBusyp, csSendPrepare, csSendReady, csSendSuccess, csSendFail} CommandState ;
typedef enum {esNone, esGotIP, esServerActive, esHardwareError} ESPState;
	
volatile extern CommandState mCommandState;
extern ESPState mESPState;
extern uint8_t mConnectionState[4];
volatile extern uint8_t mConnectionReportIndex[4];
extern uint8_t tmOkCount;
extern uint8_t ip1, ip2, ip3, ip4;
/************************************************************************/
/*                    vars from clock (MAX3234.cpp)		                */
extern void SetClock(uint8_t pDateTimeUnit, uint8_t pValue);
extern void GetTime(sTime &pTime);
extern void GetDate(sDate &pDate);
/************************************************************************/
/*                    vars from main module (GrowBender.cpp)            */
extern uint8_t mCoolingMode;
extern uint8_t mLigntMode;
extern uint8_t mFanMode;
extern uint8_t mVentMode;
extern uint8_t mHeaterMode;
extern uint8_t mHumidifyerMode;
extern int8_t mMaintainTempDayMin;
extern int8_t mMaintainTempNightMin;
extern int8_t mMaintainTempDayMax;
extern int8_t mMaintainTempNightMax;
extern uint8_t mMaintainHumidity;
extern int8_t mDeltaTemp;
extern uint8_t mAutoMode; // 0- disabled; 1 - enabled
extern uint8_t mTempSensor; // 0 - temp IN sensor; 1- DHT temp sensor (also must be placed inside); 2 - average over both sensors
extern int8_t mCriticalTemp;
extern sTime mNightStartTime;
extern uint8_t mNightDurationStart;
extern uint8_t mNightDurationEnd;
extern uint16_t mNightDurationCurr;
extern uint8_t mNightDurationDelta;
extern uint8_t mNightMode;
extern sDate mGrowStartDate;
extern uint8_t mGrowDuration;
extern sDate mCurrentDate;
extern bool mNeedMenuRedraw;
extern uint8_t mNeedToReedIP;
void SetLight(bool pSetOn);
void SetFan(uint8_t pMode);
void SetHeater(uint8_t pMode);
void SetHumidifyer(uint8_t pMode);
void SetVentFan(uint8_t pMode);

/*                                                                      */
/************************************************************************/
bool SendData(uint8_t pConnID, char* pData, uint16_t pLength);

bool WaitCommandState(CommandState pState);

bool WaitESPState(ESPState pState);
bool InitESP();
void DisconnectAP();
bool JoinAP(char* pAPName, char* pPassword);
void ReadIPStart();
bool SetMUX(uint8_t pMUX);
bool StartServer(uint16_t pPort);
bool StopServer();
bool TCPConnect(uint8_t pConnID, char* pAddress, int pPort);
bool TCPDisconnect(uint8_t pConnID);
bool TCPSendDataStr(uint8_t pConnID, char* pStrData);
void ProcessSerialData();
void ProcessESPPacket(uint16_t startIndex);
void SendReport();

#endif /* ESP_H_ */