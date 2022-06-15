/*
 * GrowBender.cpp
 *
 * Created: 05.08.2016 21:00:09
 *  Author: smart
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
//#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
//#include <avrfix.h>


#include "UART.h"
#include "LCDI2C.h"
#include "MAX3234.h"
#include "DS18B20.h"
#include "ESP.h"

#include "DHT22.h"
#include "Buzzer.h"
#include "EEPROMAddresses.h"

#ifdef DEVICE0
#define INVERTEDRELAYS
#endif

#define SWITCHPORTDIR _SFR_IO8(0x0A)// DDRD
#define SWITCHPORT PORTD
#define SWITCHPORTPINS PIND

#define BTN1PIN PD2 // !
#define BTN2PIN PD3
#define BTN3PIN PD4
//#define BTN4PIN PC3


#define REL1PIN PB2 //PD2 // ! fan 1
#define REL2PIN PB1 //PD6 // fan 2
#define REL3PIN PB0 //PD7 // vent
#define REL4PIN PD7 //PB0 // humid
#define REL5PIN PD6 //PB1 // lamp
#define REL6PIN PD5 //PB2 // heater
// #define REL7PIN PB3 // 
// #define REL8PIN PB4 // 

#define RELDSB PB0
#define RELCP PB1

#define BUTTONRELAXCYCLECOUNT 3
#define BTNLONGPRESSCYCLECOUNTER 50




#define FAN_AFTER_LAMP_OFF_MINUTES 2 // 5 minutes :)
#define MODECOUNT 5
#define NORMAL_TEMP_AREA 0.5



/// menus
/*
Program			0
-night start h			0
-night start m			1
-duration start			2
-duration end			3
-duration curr			4 {reset only}
-daily delta+			5
-night mode				6 {const start/const end/middle}
-night info				7 
Cooling			1
-Select mode			8
Clock			2
-Day					9
-Month					10
-Year					11
-DayOfWeek				12
-Hours					13
-Minutes				14
Temp			3
-Maintain temp day		15
-Maintain temp day		16
-Maintain temp night	17
-Maintain humidity		18
-Temp delta				19
-Temp auto hold mode	20
-Temp sensor			21
misc			4
-reset temp				22
-grow start d			23
-grow start m			24
-grow start y			25
-grow duration			26
menuLevel:
0 - default screen; btn1-short enters menu
1 - selected menu name; btn1 - next menu; btn2-short enters submenu
2 - selected summenu name and setting; btn1 - next menu; btn2-short enters submenu
*/

#define SM_NIGHT_START_H				0  // menu night start hour
#define SM_NIGHT_START_M				1  // menu night start minute
#define SM_NIGHT_DURATION_START			2  // menu night duration start {0-24}
#define SM_NIGHT_DURATION_END			3  // menu night duration start {0-24}
#define SM_NIGHT_DURATION_CURR			4  // menu night duration current in minutes (reset only)
#define SM_NIGHT_DURATION_DELTA			5  // daily delta     6
#define SM_NIGHT_MODE					6  // night mode     7 {0-2 const start/const end/middle}
#define SM_NIGHT_INFO					7
#define SM_COOLING_MODE					8  // menu cooling mode selected
#define SM_CLOCK_DAY					9  // menu clock-day selected
#define SM_CLOCK_MONTH					10 // menu clock-month selected
#define SM_CLOCK_YEAR					11 // menu clock-year selected
#define SM_CLOCK_DAYOFWEEK				12 // menu clock-day of week selected
#define SM_CLOCK_HOUR					13 // menu clock-hours selected
#define SM_CLOCK_MINUTE					14 // menu clock-minutes selected
#define SM_M_TEMP_DAY_MIN				15 // M Temp-
#define SM_M_TEMP_DAY_MAX				16 // M Temp-
#define SM_M_TEMP_NIGHT_MIN				17 // M Temp-
#define SM_M_TEMP_NIGHT_MAX				18 // M Temp-
#define SM_M_HUMIDITY					19 // M Humidity
#define SM_TM_DELTA						20 // TM Delta-
#define SM_AUTO_MODE					21 // auto temp control mode
#define SM_TEMP_SENSOR					22 // Sensor select (11/2/
#define SM_CRITICAL_TEMP				23 // critical temp (lamp off temp)
#define SM_RESET_TEMP					24 // reset min/max temp
#define SM_GROW_START_DAY				25 // set growing start day
#define SM_GROW_START_MONTH				26 // set growing start month
#define SM_GROW_START_YEAR				27 // set growing start year
#define SM_GROW_DURATION				28 // set growing start year
#define SM_RESET_WIFI					29 // set growing start year

//#define GetVentFan() (mRelays & (1<<VENT_REL))
//#define GetHeater() (mRelays & (1<<HEATER_REL))


sTime mCurrentTime;
sDate mCurrentDate;
static uint8_t sAfterLampOffMinutes;
sDate mGrowStartDate;
uint8_t mGrowDuration;
	
char* mDOWNames[7] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
uint8_t mMonyhDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static uint8_t Btn1RelaxCycleCounter;
volatile uint8_t Btn2RelaxCycleCounter;
volatile uint8_t Btn3RelaxCycleCounter = 0;

uint8_t Btn1LongPressCycleCounter;
uint8_t Btn2LongPressCycleCounter;
uint8_t Btn3LongPressCycleCounter;

uint8_t ButtonLongPressed; // this is used in handler to monitor button long pressing state
uint8_t ButtonStateChanged = 0; // 0-1 button bits
uint8_t ButtonWasLongPressed = 0; // 0-1 button bits
//uint8_t ButtonStateInternal = 0x0; // this is used in handler to monitor button pressing state
volatile static uint8_t ButtonStateCurrent = 0x0; // it must be used in client code to determine state of the button
uint8_t mRelays;


double mCurrentHumidity;
float mCurrentTempIn;
float mCurrentTempOut;


// switching program variables
sTime mNightStartTime;
uint8_t mNightDurationStart;
uint8_t mNightDurationEnd;
uint16_t mNightDurationCurr;
uint8_t mNightDurationDelta;
uint8_t mNightMode;
uint8_t mLastEvenHrs;
uint8_t mLastEvenMin;

// menu variables
uint8_t mMenuIndex = 0;
uint8_t mSubmenuIndex = 0;
uint8_t mMenuLevel = 0; //0 - default; 1- menu; 2 - submenu
//bool mJustEntered;
bool mNeedMenuRedraw;
//bool mNeedSubmenuRedraw;
uint8_t mCurremtFrame = 0;

//these can be optimized in one byte cuz it's in mRelay actually
uint8_t mCoolingMode;
uint8_t mLigntMode;
uint8_t mFanMode;
uint8_t mVentMode;
uint8_t mHeaterMode;
uint8_t mHumidifyerMode;

char mStrBuffer[21];
char mStrBuffer2[11];

//vent controller variables
int8_t mMaintainTempDayMin;
int8_t mMaintainTempDayMax;
int8_t mMaintainTempNightMin;
int8_t mMaintainTempNightMax;
uint8_t mMaintainHumidity;
int8_t mDeltaTemp;
uint8_t mAutoMode; // 0- disabled; 1 - enabled 
uint8_t mTempSensor; // 0 - temp IN sensor; 1- DHT temp sensor (also must be placed inside); 2 - average over both sensors
int8_t mCriticalTemp;
uint8_t mNeedToReedIP = 0;
const char cStrDateTime[] = " %02d-%02d-%02d %s %02d:%02d";
const char* cStrDate = "%02d-%02d-20%02d";
const char* cStrTime  = "%02d:%02d";
const char cCharMarker = '>';
const char* cStrGrow = "Grow ";
const char* cStrMinMaxTime =  " / ";
static uint8_t sEventToggled; 

void InitSwitches();
void InitRelays();
void SetRelays();
void InitTimer();
void ProcessPrograms();
void DrawDefault();
void DrawMenu();
void ProcessButtons();

void SetLight(bool pSetOn);
//void SetFan(uint8_t pMode);
void SetHeater(uint8_t pMode);
void SetVentFan(uint8_t pMode);
void CheckTemperature();
void CheckHumidity();
float GetInSensorTemp();
void ProcessDefaultScreenButtons();

void ProcessMenuButtons();
void ProcessSubmenuButtons();

sTime GetNightStartTime();
sTime GetNightEndTime();

void DrawFrame();
void DrawMenuFrame();

void SetMessage(char* mMessage);
bool ScrollMessage();
void SetNextMessage();

void CheckMinMaxDouble(uint8_t pAddress, double pValue)
{
	double val = eeprom_read_float((float*)(pAddress + 0));

	if (val > pValue || isnan(val))
	{
		eeprom_write_float((float*)(pAddress + 0), pValue);

		eeprom_write_byte((uint8_t*)(pAddress + 4), mCurrentTime.Hour);
		eeprom_write_byte((uint8_t*)(pAddress + 5), mCurrentTime.Minute);
	}

	val = eeprom_read_float((float*)(pAddress + 6));
	if (val < pValue || isnan(val))
	{
		eeprom_write_float((float*)(pAddress + 6), pValue);

		eeprom_write_byte((uint8_t*)(pAddress + 10), mCurrentTime.Hour);
		eeprom_write_byte((uint8_t*)(pAddress + 11), mCurrentTime.Minute);
	}	
}

//uint8_t mcusr_mirror _attribute_ ((section (".noinit")));

void InitWDT()
{
//	mcusr_mirror = MCUSR;
	MCUSR = 0;
	wdt_disable();
}

int main(void)
{
	////DDRC = 255;
	//PORTC |= 255;//(1<<PD2);
	//
	InitWDT();
	InitUART();
	InitRelays();
	InitSwitches();
	InitTempSensor();

	InitTimer();
	InitClock();

	InitLCD();
	//while(1==1);	
	InitBuzzer();
	sei();	
	
 	lcd_clear();
	lcd_goto(6,2);
	lcd_puts("Starting");

	if (InitESP())
	{
		//strcpy(mStrBuffer, "ASUS");
		//strcpy(mStrBuffer2, "r4e3w2q1");
		//eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_AP_NAME, 4);
		//eeprom_write_block(mStrBuffer, (void*)EEPROM_ADDRESS_AP_NAME+1, 4); // extra byte for len
		//eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_AP_PASSWORD, 8);
		//eeprom_write_block(mStrBuffer2, (void*)EEPROM_ADDRESS_AP_PASSWORD+1, 8);
		
		uint8_t strLen;
		strLen = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_AP_NAME);
		eeprom_read_block((void*)mStrBuffer, (void*)EEPROM_ADDRESS_AP_NAME + 1, strLen);
		mStrBuffer[strLen] = 0;
		strLen = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_AP_PASSWORD);
		eeprom_read_block((void*)mStrBuffer2, (void*)EEPROM_ADDRESS_AP_PASSWORD + 1, strLen);
		mStrBuffer2[strLen] = 0;
		
		_delay_ms(2000);
		if (JoinAP(mStrBuffer, mStrBuffer2))
		{
			SetMessage("ESP joined");
			StartServer(5000);
		}
	}
	else if (mCommandState == csBusyp)
		SetMessage("ESP busy");
	else if (mESPState == esHardwareError)
		SetMessage("ESP hw error");
	else if (mCommandState == csError)
		SetMessage("ESP error");
	else
		SetMessage("ESP unkerror");
	
//	uart_puts("started");

#pragma region Reading EEPROM

	uint8_t tempFanMode = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_FAN);
	mLigntMode = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_LIGHTS);
	mCoolingMode = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_COOLING_MODE);
	mMaintainTempDayMin = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MIN);
	mMaintainTempDayMax = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MAX);
	mMaintainTempNightMin = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MIN);
	mMaintainTempNightMax = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MAX);
	mMaintainHumidity = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_HUMIDITY);
	mDeltaTemp = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_DELTA_TEMP);
	mAutoMode = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_AUTO_MODE);
	mTempSensor = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_SENSOR);
	mCriticalTemp = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_CRITICAL_TEMP);
	
	mNightStartTime.Hour = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_START_H);
	mNightStartTime.Minute = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_START_M);
	mNightDurationStart = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_START);
	mNightDurationEnd = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_END);
	mNightDurationCurr = eeprom_read_word((uint16_t*)EEPROM_ADDRESS_NIGHT_DURATION_CURR);
	mNightDurationDelta = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_DELTA);
	mNightMode = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_MODE);
	
	mGrowStartDate.Day = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_GROW_START_DAY);
	mGrowStartDate.Month = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_GROW_START_MONTH);
	mGrowStartDate.Year = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_GROW_START_YEAR);
	mGrowDuration = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_GROW_DURATION);
#pragma endregion Reading EEPROM	

	/*
	mMinTempIn = ReadDoubleFromEEPROM((uint8_t*)EEPROM_ADDRESS_MIN_TEMP_IN);
	
	mMinTempInTime.Hour = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MIN_TEMP_IN+4);
	mMinTempInTime.Minute = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MIN_TEMP_IN+5);

	mMaxTempIn = ReadDoubleFromEEPROM((uint8_t*)EEPROM_ADDRESS_MAX_TEMP_IN);

	mMaxTempInTime.Hour = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MAX_TEMP_IN+4);
	mMaxTempInTime.Minute = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MAX_TEMP_IN+5);

	mMinTempOut = ReadDoubleFromEEPROM((uint8_t*)EEPROM_ADDRESS_MIN_TEMP_OUT);

	mMinTempOutTime.Hour = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MIN_TEMP_OUT+4);
	mMinTempOutTime.Minute = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MIN_TEMP_OUT+5);

	mMaxTempOut = ReadDoubleFromEEPROM((uint8_t*)EEPROM_ADDRESS_MAX_TEMP_OUT);
	
	mMaxTempOutTime.Hour = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MAX_TEMP_OUT+4);
	mMaxTempOutTime.Minute = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_MAX_TEMP_OUT+5);
	*/
	
	SetLight(mLigntMode); // little bit unoptimized cus calling SetFan twice
	SetFan(tempFanMode);
		
	//InitLCD();
 	lcd_clear();
	GetDate(mCurrentDate);
	SetBuzzer(2);
	
	DrawFrame();
	/*//set date and time
	//SPI_write1(0x82, 0b00010000 + 0b00000100);
	//SPI_write1(0x81, 0b01010000 + 0b00000011);
	//SPI_write1(0x83, 0b00000000 + 0b00001101);
	//SPI_write1(0x84, 0b00000001 + 0b00000011);	
	//SPI_write1(0x85, 0b00000000 + 0b00001000);	
	//SPI_write1(0x86, 0b00000001 + 0b00000110);	
	
	//SetMessage("GrowBender v0.9 candidate");*/

	uint8_t cycleCounter = 0;
	uint8_t oldRelays = 0;
/*	
	if (isnan(ReadDoubleFromEEPROM(EEPROM_ADDRESS_TEMP_IN)))
		SaveDoubleToEEPROM(EEPROM_ADDRESS_TEMP_IN, 100);
	if (isnan(ReadDoubleFromEEPROM(EEPROM_ADDRESS_TEMP_IN + 6)))
		SaveDoubleToEEPROM(EEPROM_ADDRESS_TEMP_IN + 6, 100);
	if (isnan(ReadDoubleFromEEPROM(EEPROM_ADDRESS_TEMP_OUT)))
		SaveDoubleToEEPROM(EEPROM_ADDRESS_TEMP_OUT, 100);
	if (isnan(ReadDoubleFromEEPROM(EEPROM_ADDRESS_TEMP_OUT + 6)))

	if (isnan(ReadDoubleFromEEPROM(EEPROM_ADDRESS_TEMP_IN_DHT)))
		SaveDoubleToEEPROM(EEPROM_ADDRESS_TEMP_IN_DHT, 100);
	if (isnan(ReadDoubleFromEEPROM(EEPROM_ADDRESS_TEMP_IN_DHT + 6)))
		SaveDoubleToEEPROM(EEPROM_ADDRESS_TEMP_IN_DHT + 6, 0);
	if (isnan(ReadDoubleFromEEPROM(EEPROM_ADDRESS_HUMIDITY)))
		SaveDoubleToEEPROM(EEPROM_ADDRESS_HUMIDITY, 100);
	if (isnan(ReadDoubleFromEEPROM(EEPROM_ADDRESS_HUMIDITY + 6)))
		SaveDoubleToEEPROM(EEPROM_ADDRESS_HUMIDITY + 6, 0);
*/
	ButtonStateCurrent = 255;	
	ButtonStateChanged = 0;	

	while(1)
    {
		cycleCounter++;
		GetTime(mCurrentTime);		
		if (ButtonStateChanged)
			ProcessButtons();
		ProcessPrograms();
		if (mMenuLevel == 0)
			DrawDefault();
		else
			DrawMenu();

		if (cycleCounter % 10 == 0)
		{
			if (mNeedToReedIP == 1)
			{
				ReadIPStart();
				mNeedToReedIP = 0;
			}
			ReadData(); // reading DHT22
			CheckMinMaxDouble(EEPROM_ADDRESS_HUMIDITY, mHumidity);
			CheckMinMaxDouble(EEPROM_ADDRESS_TEMP_IN_DHT, mCurrentTempIn_dht);
			//
			mCurrentTempIn = GetTemperature1();
			CheckMinMaxDouble(EEPROM_ADDRESS_TEMP_IN, mCurrentTempIn);
				//
			mCurrentTempOut = GetTemperature2();
			CheckMinMaxDouble(EEPROM_ADDRESS_TEMP_OUT, mCurrentTempOut);
//
			CheckTemperature();
			CheckHumidity();
			if (oldRelays != mRelays)
			{
				SetRelays();
				oldRelays = mRelays;
			}
			
			ProcessSerialData();
			SendReport();
		}
		if (mCurremtFrame == 0 && ScrollMessage())
			SetNextMessage();

		ProcessBuzzer(); // takes 50 MS
		_delay_ms(50);
    }
}


void ProcessPrograms()
{
	static uint8_t sLastHour;
	static uint8_t sMinute;
	
	// check date change, dec/inc current night duration 
	if (mNightDurationCurr != mNightDurationEnd)
	{
		int8_t trueDelta = (mNightDurationStart <= mNightDurationEnd ? mNightDurationDelta : -mNightDurationDelta);
		if (sLastHour == 24 && mCurrentTime.Hour == 0)
		{
			mNightDurationCurr += trueDelta;
			if (trueDelta * (int8_t)mNightDurationCurr >= trueDelta * (int8_t)mNightDurationEnd)
				mNightDurationCurr = mNightDurationEnd*60;
			eeprom_write_word((uint16_t*)EEPROM_ADDRESS_NIGHT_DURATION_CURR, mNightDurationCurr);
		}
		sLastHour = mCurrentTime.Hour;
	}
	// processing events
	if (mNightDurationCurr == 0 || mNightDurationCurr == 24)
		return;
	if (mLastEvenHrs != mCurrentTime.Hour || mLastEvenMin != mCurrentTime.Minute)
	{
		sTime nightTime;
		nightTime = GetNightEndTime();
		if (nightTime.Hour == mCurrentTime.Hour
			&& nightTime.Minute == mCurrentTime.Minute)
		{
			mLastEvenHrs = mCurrentTime.Hour;
			mLastEvenMin = mCurrentTime.Minute;
			SetLight(true);
		}
		
		nightTime = GetNightStartTime();
		if (nightTime.Hour == mCurrentTime.Hour
			&& nightTime.Minute == mCurrentTime.Minute)
		{
			mLastEvenHrs = mCurrentTime.Hour;
			mLastEvenMin = mCurrentTime.Minute;
			SetLight(false);
			sAfterLampOffMinutes = FAN_AFTER_LAMP_OFF_MINUTES;
		}
	}
	// checking after-lamp-off fan timout
	if (mCoolingMode == 3 && sAfterLampOffMinutes != 0 && sMinute != mCurrentTime.Minute)
	{
		sMinute = mCurrentTime.Minute;
		if (--sAfterLampOffMinutes == 0)
			SetFan(0);
	}
}
#define BTN2_PRESSED ((ButtonStateChanged & 0x02) && !(ButtonStateCurrent & 0x02))
#define BTN3_PRESSED ((ButtonStateChanged & 0x04) && !(ButtonStateCurrent & 0x04))
void ProcessButtons()
{
	switch (mMenuLevel)
	{
		case 0:
			ProcessDefaultScreenButtons();
		break;
		case 1:
			ProcessMenuButtons();
		break;
		case 2:
			ProcessSubmenuButtons();
		break;
	}
	ButtonStateChanged = 0;
	ButtonLongPressed = 0;
}

void ProcessDefaultScreenButtons()
{
	if ((ButtonStateChanged & 0x01))  // btn1 state changed
	{
		if  (ButtonStateCurrent & 0x01) // btn1 short (on open)
		{
			if (++mCurremtFrame > 3)
			{
				mCurremtFrame = 0;
				InitLCD();
			}
			DrawFrame();
		}
		else if (ButtonLongPressed & 0x01) // btn2 long
		{
			mMenuIndex = 0;
			mMenuLevel = 1;
			//			mJustEntered = true;
			mNeedMenuRedraw = true;
			DrawMenuFrame();
		}
	}
	
	if ((ButtonStateChanged & 0x02))  // btn2 state changed
	{
		if  (ButtonStateCurrent & 0x02) // btn2 short press (on open)
		{
			if (++mFanMode > 2)
				mFanMode = mLigntMode == 1?1:0; // if lamp is on, fan must be on as well
			SetFan(mFanMode);
		}
		else if (ButtonLongPressed & 0x02) // btn2 long
		{
			ButtonLongPressed &= ~0x02;
			SetLight(mLigntMode != 1);
			mMenuLevel = 0;
		}
	}

	if (ButtonStateChanged & 0x04)
	{
		if (ButtonLongPressed & 0x04) // btn3 long
		{
//			SetBuzzer(1);
			SetHeater(mHeaterMode?0:1);
		}
		else if (ButtonStateCurrent & 0x04) // btn3 short (on open)
			SetVentFan(mVentMode?0:1);
	}
}

void SelectDefautlSubmenu()
{
	switch(mMenuIndex )
	{
		case 0:
			mSubmenuIndex = SM_NIGHT_START_H;
		break;
		case 1:
			mSubmenuIndex = SM_COOLING_MODE;
		break;
		case 2:
			mSubmenuIndex = SM_CLOCK_DAY;
		break;
		case 3:
			mSubmenuIndex = SM_M_TEMP_DAY_MIN;
		break;
		case 4:
			mSubmenuIndex = SM_RESET_TEMP;
		break;
	}
}

void SelectNextSubmenu()
{
	switch(mMenuIndex)
	{
		case 0:
			if (++mSubmenuIndex >= SM_COOLING_MODE)
				mSubmenuIndex = SM_NIGHT_START_H;
		break;
		case 1:
			mSubmenuIndex = SM_COOLING_MODE;
		break;
		case 2:
			if (++mSubmenuIndex >= SM_M_TEMP_DAY_MIN) 
				mSubmenuIndex = SM_CLOCK_DAY; 
		break;
		case 3:
			if (++mSubmenuIndex >= SM_RESET_TEMP) 
				mSubmenuIndex = SM_M_TEMP_DAY_MIN;
		break;
		case 4:
			if (++mSubmenuIndex > SM_RESET_WIFI)
				mSubmenuIndex = SM_RESET_TEMP;
		break;
	}
}

void ProcessMenuButtons()
{
	mNeedMenuRedraw = true;
	if (ButtonLongPressed & 0x01) // btn1 long
	{
		mMenuIndex = 0;
		mMenuLevel = 0;
		DrawFrame();
		ButtonLongPressed = 0;
		ProcessDefaultScreenButtons(); // expected processing only of btn1 longpress!
	}
	else if ((ButtonStateChanged & 0x01) && !(ButtonStateCurrent & 0x01)) // btn1 short
	{
		if (++mMenuIndex > 4)
			mMenuIndex = 0;
		mMenuLevel = 1;
	}
	
	if ((ButtonStateChanged & 0x02) && !(ButtonStateCurrent & 0x02)) // btn1 short
	{
		mMenuLevel = 2;
		SelectDefautlSubmenu();
	}
}
void ChangeAndSaveParameter(uint8_t &pParameter, uint8_t pLowLimit, uint8_t pHighLimit, uint16_t pEEPROMAddress, uint8_t pWriteDevice /*0 - eeprom; 1- clock */)
{
	// ugly assignments in IF condition. But alternate is unreadable
	if (ButtonLongPressed & 0x02) 
	{
		if (pHighLimit - pParameter > 10)
			pParameter += 10;
		else
			pParameter = pLowLimit;
	}
	else if (BTN2_PRESSED)
	{
		if (pParameter < pHighLimit)
			pParameter ++;
		else
			pParameter = pLowLimit;
	}
	if (ButtonLongPressed & 0x04) 
	{
		if (pParameter - pLowLimit > 10)
			pParameter -= 10;
		else
			pParameter = pHighLimit;
	}
	else if (BTN3_PRESSED)
	{
		if (pParameter > pLowLimit)
			pParameter --;
		else
			pParameter = pHighLimit;
	}
	if (!pWriteDevice)
		eeprom_write_byte((uint8_t*)(pEEPROMAddress + 0), pParameter);
	else
		SetClock(pEEPROMAddress, pParameter);
	
}

void ChangeAndSaveParameter(int8_t &pParameter,  int8_t pLowLimit, int8_t pHighLimit, uint16_t pEEPROMAddress, uint8_t pWriteDevice /*0 - eeprom; 1- clock */)
{
	// ugly assignments in IF condition. But alternate is unreadable
	if (ButtonLongPressed & 0x02) 
	{	if (abs(pHighLimit - pParameter) > 10)
			pParameter += 10;
		else
			pParameter = pLowLimit;
	}
	else if (BTN2_PRESSED)
	{
			if (pParameter < pHighLimit)
			pParameter ++;
		else
			pParameter = pLowLimit;
	}
	if (ButtonLongPressed & 0x04) 
	{
			if (abs(pParameter - pLowLimit) > 10)
			pParameter -= 10;
		else
			pParameter = pHighLimit;
	}
	else if (BTN3_PRESSED)
	{
		if (pParameter > pLowLimit)
			pParameter --;
		else
			pParameter = pHighLimit;
	}
	if (!pWriteDevice)
		eeprom_write_byte((uint8_t*)pEEPROMAddress, pParameter);
	else
		SetClock(pEEPROMAddress, pParameter);
}
void ProcessSubmenuButtons()
{
	mNeedMenuRedraw = true;
	if (ButtonLongPressed & 0x01) // btn1
	{
		mMenuLevel = 1;
		DrawMenuFrame();
	}
	else if ((ButtonStateChanged & 0x01) && !(ButtonStateCurrent & 0x01)) // btn1
		SelectNextSubmenu();
	else // btn2
		switch (mSubmenuIndex)
		{
			case SM_NIGHT_START_H: // menu night start hour
				ChangeAndSaveParameter(mNightStartTime.Hour, 0, 24, EEPROM_ADDRESS_NIGHT_START_H, 0);
				/*if (BTN2_PRESSED)
				{
					if (++mNightStartTime.Hour >= 24)
						mNightStartTime.Hour = 0;
				}
				else if (BTN3_PRESSED)
					if (--mNightStartTime.Hour >= 24)
						mNightStartTime.Hour = 23;
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_START_H, mNightStartTime.Hour);*/
			break;
			case SM_NIGHT_START_M: // menu night start minute
				ChangeAndSaveParameter(mNightStartTime.Minute, 0, 59, EEPROM_ADDRESS_NIGHT_START_M, 0);
				/*if (BTN2_PRESSED &&++mNightStartTime.Minute >= 60)
					mNightStartTime.Minute = 0;
				else if (BTN3_PRESSED && --mNightStartTime.Minute >= 60)
					mNightStartTime.Minute = 59;
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_START_M, mNightStartTime.Minute);*/
			break;
			case SM_NIGHT_DURATION_START: // menu night duration start {0-24}
				ChangeAndSaveParameter(mNightDurationStart, 0, 24, EEPROM_ADDRESS_NIGHT_DURATION_START, 0);
				/*if (BTN2_PRESSED && ++mNightDurationStart >= 24)
					mNightDurationStart = 0;
				else if (BTN3_PRESSED && --mNightDurationStart >= 24)
					mNightDurationStart = 23;
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_START, mNightDurationStart);*/
			break;
			case SM_NIGHT_DURATION_END: // menu night duration end {0-24}
				ChangeAndSaveParameter(mNightDurationEnd, 0, 24, EEPROM_ADDRESS_NIGHT_DURATION_END, 0);
				/*if (BTN2_PRESSED && ++mNightDurationEnd >= 24)
					mNightDurationEnd = 0;
				else if (BTN3_PRESSED && --mNightDurationEnd >= 24)
					mNightDurationEnd = 23;
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_END, mNightDurationEnd);*/
			break;
			case SM_NIGHT_DURATION_CURR: // menu night duration current in minutes (reset only)
				if (BTN2_PRESSED || BTN3_PRESSED) 
				{
					mNightDurationCurr = mNightDurationStart*60;
					eeprom_write_word((uint16_t*)EEPROM_ADDRESS_NIGHT_DURATION_CURR, mNightDurationCurr);
					//eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_CURR, mNightDurationCurr / 256);
					//eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_CURR+1, mNightDurationCurr % 256);
				}
			break;
			case SM_NIGHT_DURATION_DELTA: //-daily delta     6
				ChangeAndSaveParameter(mNightDurationDelta, 0, 120, EEPROM_ADDRESS_NIGHT_DURATION_DELTA, 0);
				/*if (BTN2_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_DELTA, ++mNightDurationDelta);
				else if (BTN3_PRESSED)
					if (mNightDurationDelta != 0)
						eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_DURATION_DELTA, --mNightDurationDelta);
				*/
			break;
			case SM_NIGHT_MODE: //-night mode     7 {0-2 const start/const end/middle} 
				ChangeAndSaveParameter(mNightMode, 0, 2, EEPROM_ADDRESS_NIGHT_MODE, 0);
				/*if (BTN2_PRESSED && ++mNightMode >= 3)
				mNightMode = 0;
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_NIGHT_MODE, mNightMode);*/
			break;
			case SM_COOLING_MODE: // menu mode selected
				ChangeAndSaveParameter(mCoolingMode, 0, MODECOUNT, EEPROM_ADDRESS_COOLING_MODE, 0);
				/*if (BTN2_PRESSED && ++mCoolingMode >= MODECOUNT) 
					mCoolingMode = 0;
				else if (BTN3_PRESSED && --mCoolingMode >= MODECOUNT) 
					mCoolingMode = MODECOUNT - 1;
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_COOLING_MODE, mCoolingMode);*/
			break;
			case SM_CLOCK_HOUR: // menu clock-hours selected
				ChangeAndSaveParameter(mCurrentTime.Hour, 0, 23, 0x82, 1);
				/*				if (BTN2_PRESSED && ++mCurrentTime.Hour >= 24) 
					mCurrentTime.Hour = 0;
				else if (BTN3_PRESSED && --mCurrentTime.Hour >= 24)
					mCurrentTime.Hour = 23;
				SPI_write1(0x82, (((mCurrentTime.Hour/ 10))<<4) + (mCurrentTime.Hour % 10));*/					 
			break;
			case SM_CLOCK_MINUTE: // menu clock-minutes selected
				ChangeAndSaveParameter(mCurrentTime.Minute, 0, 59, 0x81, 1);
				/*if (BTN2_PRESSED && ++mCurrentTime.Minute >= 60)
					mCurrentTime.Minute = 0;
				
				else if (BTN3_PRESSED && --mCurrentTime.Minute >= 60)
					mCurrentTime.Minute = 59;
				SPI_write1(0x81, (((mCurrentTime.Minute / 10))<<4) + (mCurrentTime.Minute % 10));*/
			break;
			case SM_CLOCK_DAYOFWEEK: // menu clock-minutes selected
				ChangeAndSaveParameter(mCurrentDate.DayOfWeek, 1, 7, 0x83, 1);
				/*if (BTN2_PRESSED && ++mCurrentDate.DayOfWeek >= 8)
					mCurrentDate.DayOfWeek = 1;
				
				else if (BTN3_PRESSED && --mCurrentDate.DayOfWeek == 0)
					mCurrentDate.DayOfWeek = 7;
						
				SPI_write1(0x83, mCurrentDate.DayOfWeek);*/
			break;
						
			case SM_CLOCK_DAY: // menu clock-minutes selected
				ChangeAndSaveParameter(mCurrentDate.Day, 1, mMonyhDays[mCurrentDate.Month - 1], 0x84, 1);
				/*if (BTN2_PRESSED  && ++mCurrentDate.Day > mMonyhDays[mCurrentDate.Month - 1])
					mCurrentDate.Day = 1;
				else if (BTN3_PRESSED  && --mCurrentDate.Day > mMonyhDays[mCurrentDate.Month - 1])
					mCurrentDate.Day = mMonyhDays[mCurrentDate.Month - 1];
				SPI_write1(0x84, (((mCurrentDate.Day / 10))<<4) + (mCurrentDate.Day % 10));*/
			break;
									
			case SM_CLOCK_MONTH: // menu clock-minutes selected
				ChangeAndSaveParameter(mCurrentDate.Month, 1, 12, 0x85, 1);
				/*if (BTN2_PRESSED && ++mCurrentDate.Month > 12)
					mCurrentDate.Month = 1;
				
				else if (BTN3_PRESSED && --mCurrentDate.Month > 12)
					mCurrentDate.Month = 12;
												
				SPI_write1(0x85, (((mCurrentDate.Month / 10))<<4) + (mCurrentDate.Month % 10));*/
			break;								
			case SM_CLOCK_YEAR: // menu clock-minutes selected
				ChangeAndSaveParameter(mCurrentDate.Year, 1, 99, 0x86, 1);
				/*if (BTN2_PRESSED && ++mCurrentDate.Year > 99)
					mCurrentDate.Year = 0;
				
				else if (BTN3_PRESSED && --mCurrentDate.Year >99)
					mCurrentDate.Year = 0;
															
				SPI_write1(0x86, (((mCurrentDate.Year / 10))<<4) + (mCurrentDate.Year % 10));*/
			break;
			case SM_M_TEMP_DAY_MIN: // M Temp+
				ChangeAndSaveParameter(mMaintainTempDayMin, -20, 50, EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MIN, 0);	
				/*if (BTN2_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MIN, ++mMaintainTempDayMin);
				else if (BTN3_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MIN, --mMaintainTempDayMin);*/
			break;
			case SM_M_TEMP_DAY_MAX: // M Temp+
				ChangeAndSaveParameter(mMaintainTempDayMax, -20, 50, EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MAX, 0);
				/*if (BTN2_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MAX, ++mMaintainTempDayMax);
				else if (BTN3_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_DAY_MAX, --mMaintainTempDayMax);*/
			break;
			case SM_M_TEMP_NIGHT_MIN: // M Temp+
				ChangeAndSaveParameter(mMaintainTempNightMin, -20, 50, EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MIN, 0);
				/*if (BTN2_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MIN, ++mMaintainTempNightMin);
				else if (BTN3_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MIN, --mMaintainTempNightMin);*/
			break;
			case SM_M_TEMP_NIGHT_MAX: // M Temp+
				ChangeAndSaveParameter(mMaintainTempNightMax, -20, 50, EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MAX, 0);
				/*if (BTN2_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MAX, ++mMaintainTempNightMax);
				else if (BTN3_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_TEMP_NIGHT_MAX, --mMaintainTempNightMax);*/
			break;
			case SM_M_HUMIDITY: // humidity
				ChangeAndSaveParameter(mMaintainHumidity, 0, 100, EEPROM_ADDRESS_MAINTAIN_HUMIDITY, 0);
				/*if (BTN2_PRESSED && mMaintainHumidity < 100)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_HUMIDITY, ++mMaintainHumidity);
				else if (BTN3_PRESSED && mMaintainHumidity > 0)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_MAINTAIN_HUMIDITY, --mMaintainHumidity);*/
			break;
			case SM_TM_DELTA: // TM Delta-
				ChangeAndSaveParameter(mDeltaTemp, 0, 200, EEPROM_ADDRESS_DELTA_TEMP, 0);
				/*if (BTN2_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_DELTA_TEMP, ++mDeltaTemp);
				else if (BTN3_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_DELTA_TEMP, --mDeltaTemp);*/
			break;
			case SM_AUTO_MODE: // heater enabled/disabled
				ChangeAndSaveParameter(mAutoMode, 0, 1, EEPROM_ADDRESS_AUTO_MODE, 0);
				/*if (BTN2_PRESSED && ++mAutoMode > 1)
					mAutoMode = 0;
				else if (BTN3_PRESSED && --mAutoMode > 1)
					mAutoMode = 1;
				sEventToggled = 4; //signal temperature recheck
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_AUTO_MODE, mAutoMode);*/
			break;
			case SM_TEMP_SENSOR: // select temp sensor
				ChangeAndSaveParameter(mTempSensor, 0, 2, EEPROM_ADDRESS_TEMP_SENSOR, 0);
				/*if (BTN2_PRESSED && ++mTempSensor > 2) 
					mTempSensor = 0;
				else if (BTN3_PRESSED && --mTempSensor > 2)
					mTempSensor = 0;
									
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_TEMP_SENSOR, mTempSensor);*/
			break;
			
			case SM_CRITICAL_TEMP: // select temp sensor
				ChangeAndSaveParameter(mCriticalTemp, 35, 65, EEPROM_ADDRESS_CRITICAL_TEMP, 0);
				/*if (BTN2_PRESSED && ++mTempSensor > 2) 
					mTempSensor = 0;
				else if (BTN3_PRESSED && --mTempSensor > 2)
					mTempSensor = 0;
									
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_TEMP_SENSOR, mTempSensor);*/
			break;
			
			case SM_RESET_TEMP: // temp min/max reset
				if (BTN2_PRESSED)
				{
					eeprom_write_float((float*)(EEPROM_ADDRESS_TEMP_OUT + 0), 100);
					eeprom_write_float((float*)(EEPROM_ADDRESS_TEMP_OUT + 6), 0);
					eeprom_write_float((float*)(EEPROM_ADDRESS_TEMP_IN + 0), 100);
					eeprom_write_float((float*)(EEPROM_ADDRESS_TEMP_IN + 6), 0);
					eeprom_write_float((float*)(EEPROM_ADDRESS_TEMP_IN_DHT + 0), 100);
					eeprom_write_float((float*)(EEPROM_ADDRESS_TEMP_IN_DHT + 6), 0);
					eeprom_write_float((float*)(EEPROM_ADDRESS_HUMIDITY + 0), 100);
					eeprom_write_float((float*)(EEPROM_ADDRESS_HUMIDITY + 6), 0);
					lcd_puts("OK");
				}
			break;						
			case SM_GROW_START_DAY: // heater enabled/disabled
				ChangeAndSaveParameter(mGrowStartDate.Day, 1, mMonyhDays[mGrowStartDate.Month - 1], EEPROM_ADDRESS_GROW_START_DAY, 0);
				/*if (BTN2_PRESSED)
				{
					if (++mGrowStartDate.Day > mMonyhDays[mGrowStartDate.Month - 1])
						mGrowStartDate.Day = 1;
				}
				else if (BTN3_PRESSED)
					if (--mGrowStartDate.Day > mMonyhDays[mGrowStartDate.Month - 1])
						mGrowStartDate.Day = mMonyhDays[mGrowStartDate.Month - 1];
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_GROW_START_DAY, mGrowStartDate.Day);*/
			break;
			case SM_GROW_START_MONTH: // grow start day
				ChangeAndSaveParameter(mGrowStartDate.Month, 1, 12, EEPROM_ADDRESS_GROW_START_MONTH, 0);
				/*if (BTN2_PRESSED)
				{
					if (++mGrowStartDate.Month > 12)
						mGrowStartDate.Month = 1;
				}
				else if (BTN3_PRESSED)
					if (--mGrowStartDate.Month > 12)
						mGrowStartDate.Month = 12;
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_GROW_START_MONTH, mGrowStartDate.Month);*/
			break;
			case SM_GROW_START_YEAR: // set grow start date
				ChangeAndSaveParameter(mGrowStartDate.Year, 1, 99, EEPROM_ADDRESS_GROW_START_YEAR, 0);
				/*if (BTN2_PRESSED)
				{
					if (++mGrowStartDate.Year > 99)
						mGrowStartDate.Year = 0;
				}
				else if (BTN3_PRESSED)
					if (--mGrowStartDate.Year >99)
						mGrowStartDate.Year = 0;
				eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_GROW_START_YEAR, mGrowStartDate.Year);*/
			break;
			case SM_GROW_DURATION: // set grow duration in days
				ChangeAndSaveParameter(mGrowDuration, 10, 200, EEPROM_ADDRESS_GROW_DURATION, 0);
				/*if (BTN2_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_GROW_DURATION, ++mGrowDuration);
				else if (BTN3_PRESSED)
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_GROW_DURATION, --mGrowDuration);*/
			break;			
			case SM_RESET_WIFI:
				if (BTN2_PRESSED)
				{
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_AP_NAME, 4);
					eeprom_write_block("ASUS", (void*)EEPROM_ADDRESS_AP_NAME + 1, 4); // extra byte for len
					eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_AP_PASSWORD, 8);
					eeprom_write_block("r4e3w2q1", (void*)EEPROM_ADDRESS_AP_PASSWORD + 1, 8);
					lcd_puts("OK");
				}
			break;
		}
}

void SetLight(bool pSetOn)
{
	if (pSetOn)
	{
		mLigntMode = 1;
		mRelays |= (0x10); // 5th relay
		eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_LIGHTS, 1);
		if (mCoolingMode == 0 || mCoolingMode == 1)
			SetFan(2);
		else
			SetFan(1);
	}
	else
	{
		mLigntMode = 0;
		mRelays &= ~(0x10); // 5th relay
		eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_LIGHTS, 0);
		if (mCoolingMode == 0)
			SetFan(2);
		else
			SetFan(1);
	}
}

void SetFan(uint8_t pMode)
{
	if (mLigntMode == 1 && pMode == 0)
		pMode = 1;
	eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_FAN, pMode);
	mFanMode = pMode;
	mRelays &= 0b11111100;

	mRelays |= pMode;
}

void SetVentFan(uint8_t pMode)
{
	mVentMode = pMode;
	//eeprom_write_byte((uint8_t*)EEPROM_ADDRESS_VENTFAN, pMode);
	if (pMode)
		mRelays |= (1<<VENT_REL);
	else
		mRelays &= ~(1<<VENT_REL);
}

void SetHeater(uint8_t pMode)
{
	mHeaterMode = pMode;
	if (pMode)
	{

		mRelays |= (1<<HEATER_REL);
	}
	else
		mRelays &= ~(1<<HEATER_REL);
}

void SetHumidifyer(uint8_t pMode)
{
	mHumidifyerMode = pMode;
	if (pMode)
		mRelays |= (1<<HUMIDIFYER_REL);
	else
		mRelays &= ~(1<<HUMIDIFYER_REL);
}


void DrawMenuFrame()
{
	lcd_clear();
	lcd_goto(0,0);// optimize! should draw only on menu enter
	lcd_puts("    Settings    ");
}


void DrawMenu()
{

	if (!mNeedMenuRedraw)
		return;
	mNeedMenuRedraw = false;
	if (mMenuLevel == 1)
	{
// 		if (mMenuIndex == 3) // crutch for glitching LCD
// 			InitLCD();
					
		lcd_goto(0,1);
		lcd_putc(cCharMarker);
		switch(mMenuIndex)
		{
			case 0: //prog
				lcd_puts("Night options");
			break;
			case 1: // Safety
				lcd_puts("Cooling mode ");
			break;
			case 2: //Clock
				lcd_puts("Clock        ");
			break;
			case 3: //Temp
				lcd_puts("Temperature  ");
			break;
			case 4: //Temp
				lcd_puts("Miscelaneous ");
			break;
		}
	}
	else if (mMenuLevel == 2)
	{
		lcd_goto(0,1);
		lcd_putc(' ');

		lcd_goto(0,2);
		switch(mSubmenuIndex)
		{
			case SM_NIGHT_START_H: // menu night start hour
				sprintf(mStrBuffer, " Start:    >%02d:%02d", mNightStartTime.Hour, mNightStartTime.Minute);
				lcd_puts(mStrBuffer);
			break;
			case SM_NIGHT_START_M:
				sprintf(mStrBuffer, " Start:     %02d>%02d", mNightStartTime.Hour, mNightStartTime.Minute);
				lcd_puts(mStrBuffer);
			break;
			case SM_NIGHT_DURATION_START:
				sprintf(mStrBuffer, "%3d hrs", mNightDurationStart);
				lcd_puts(">ND start:");
				lcd_puts(mStrBuffer);
			break;
			case SM_NIGHT_DURATION_END:
				sprintf(mStrBuffer, "%3d hrs", mNightDurationEnd);
				lcd_puts(">ND end:  ");
				lcd_puts(mStrBuffer);
			break;
			case SM_NIGHT_DURATION_CURR:
				sprintf(mStrBuffer, "%4d mi", mNightDurationCurr);
				lcd_puts(">ND curr: ");
				lcd_puts(mStrBuffer);
			break;
			case SM_NIGHT_DURATION_DELTA:
				sprintf(mStrBuffer, "%04d mi", mNightDurationDelta);
				lcd_puts(">ND delta :");
				lcd_puts(mStrBuffer);
			break;
			case SM_NIGHT_MODE:
				lcd_puts(">Align:");
				switch (mNightMode)
				{
					case 0:
						lcd_puts(" START     "); // start = const
					break;
					case 1:
						lcd_puts(" END       "); // end = const
					break;
					case 2:
						lcd_puts(" MIDDLE    "); // start and end inc and dec by delta/2
					break;
				}
			break;
			case SM_NIGHT_INFO:
				sprintf(mStrBuffer, "NIGHT %02d:%02d-%02d:%02d", GetNightStartTime().Hour, GetNightStartTime().Minute, GetNightEndTime().Hour, GetNightEndTime().Minute);
				lcd_puts(mStrBuffer);
			break;
			case SM_COOLING_MODE: // menu cooling mode selected
				lcd_putc(cCharMarker);
				switch (mCoolingMode)
				{
					case 0:
						lcd_puts("FULL  "); // always full; cannot be turned off when lamp is on
					break;
					case 1:
						lcd_puts("HIGH"); // full when lamp is on; half when lamp is off; cannot be turned off when lamp is on
					break;
					case 2: 
						lcd_puts("MEDIUM"); // always half; cannot be turned off when lamp is on
					break;
					case 3: 
						lcd_puts("LOW   "); // half when lamp is on; half when lamp is off for 5 minutes then off; cannot be turned off when lamp is on
					break;
					case 4: 
						lcd_puts("MANUAL"); //independent fan from lamp; can be turned off when lamp is on
				}
			break;
			case SM_CLOCK_DAYOFWEEK: //Clock DOW
				sprintf(mStrBuffer, cStrDateTime, mCurrentDate.Day, mCurrentDate.Month, mCurrentDate.Year, mDOWNames[mCurrentDate.DayOfWeek-1], mCurrentTime.Hour, mCurrentTime.Minute);
				lcd_puts(mStrBuffer);
				lcd_goto(9,2);
				lcd_putc(cCharMarker);
			break;
			case SM_CLOCK_DAY: //Clock day
				sprintf(mStrBuffer, cStrDateTime, mCurrentDate.Day, mCurrentDate.Month, mCurrentDate.Year, mDOWNames[mCurrentDate.DayOfWeek-1], mCurrentTime.Hour, mCurrentTime.Minute);
				lcd_puts(mStrBuffer);
				lcd_goto(0,2);
				lcd_putc(cCharMarker);
			break;
			case SM_CLOCK_MONTH: //Clock month
				sprintf(mStrBuffer, cStrDateTime, mCurrentDate.Day, mCurrentDate.Month, mCurrentDate.Year, mDOWNames[mCurrentDate.DayOfWeek-1], mCurrentTime.Hour, mCurrentTime.Minute);
				lcd_puts(mStrBuffer);
				lcd_goto(3,2);
				lcd_putc(cCharMarker);
			break;
			case SM_CLOCK_YEAR: //Clock year
				sprintf(mStrBuffer, cStrDateTime, mCurrentDate.Day, mCurrentDate.Month, mCurrentDate.Year, mDOWNames[mCurrentDate.DayOfWeek-1], mCurrentTime.Hour, mCurrentTime.Minute);
				lcd_puts(mStrBuffer);
				lcd_goto(6,2);
				lcd_putc(cCharMarker);
			break;
			case SM_CLOCK_HOUR: //Clock hours
				sprintf(mStrBuffer, cStrDateTime, mCurrentDate.Day, mCurrentDate.Month, mCurrentDate.Year, mDOWNames[mCurrentDate.DayOfWeek-1], mCurrentTime.Hour, mCurrentTime.Minute);
				lcd_puts(mStrBuffer);
				lcd_goto(12,2);
				lcd_putc(cCharMarker);
			break;
			case SM_CLOCK_MINUTE: //Clock minutes
				sprintf(mStrBuffer, cStrDateTime, mCurrentDate.Day, mCurrentDate.Month, mCurrentDate.Year, mDOWNames[mCurrentDate.DayOfWeek-1], mCurrentTime.Hour, mCurrentTime.Minute);
				lcd_puts(mStrBuffer);
				lcd_goto(15,2);
				lcd_putc(cCharMarker);
			break;
			case SM_M_TEMP_DAY_MIN: //Temp maintain min at day
				dtostrf(mMaintainTempDayMin, 2, 2, mStrBuffer);
				lcd_puts(">D Temp min:");
				lcd_puts(mStrBuffer);
				lcd_puts("    ");
			break;
			case SM_M_TEMP_DAY_MAX: //Temp maintain max at day
				dtostrf(mMaintainTempDayMax, 2, 2, mStrBuffer);
				lcd_puts(">D Temp max:");
				lcd_puts(mStrBuffer);
				lcd_puts("    ");
			break;
			case SM_M_TEMP_NIGHT_MIN: //Temp maintain min at night
				dtostrf(mMaintainTempNightMin, 2, 2, mStrBuffer);
				lcd_puts(">N Temp min:");
				lcd_puts(mStrBuffer);
				lcd_puts("    ");
			break;
			case SM_M_TEMP_NIGHT_MAX: //Temp maintain max at night
				dtostrf(mMaintainTempNightMax, 2, 2, mStrBuffer);
				lcd_puts(">N Temp max:");
				lcd_puts(mStrBuffer);
				lcd_puts("    ");
			break;
			case SM_M_HUMIDITY: //maintain Humidity
				dtostrf(mMaintainHumidity, 2, 2, mStrBuffer);
				lcd_puts(">Humidity  :");
				lcd_puts(mStrBuffer);
				lcd_puts("    ");
			break;
			case SM_TM_DELTA: //Temp delta +
				sprintf(mStrBuffer, "%3d", mDeltaTemp);
				lcd_puts(">MT Delta  :");
				lcd_puts(mStrBuffer);
				lcd_puts("    ");
			break;
			case SM_AUTO_MODE: // heater mode
				lcd_putc(cCharMarker);
				lcd_puts("Auto: ");
				switch (mAutoMode)
				{
					case 0:
						lcd_puts("OFF "); // disabled
					break;
					case 1:
						lcd_puts("ON  "); // always enabled
					break;
				}
				lcd_puts("    ");
			break;
			case SM_TEMP_SENSOR: // heater mode
				lcd_putc(cCharMarker);
				lcd_puts("Sensor: ");
				switch (mTempSensor)
				{
					case 0:
						lcd_puts("in1"); // disabled
					break;
					case 1:
						lcd_puts("dht"); // always enabled
					break;
					case 2:
						lcd_puts("AVG"); // always enabled
					break;
				}
				lcd_puts("     ");
			break;
			
			case SM_CRITICAL_TEMP: 
				sprintf(mStrBuffer, "%3d", mCriticalTemp);
				lcd_puts(">Critical temp:");
				lcd_puts(mStrBuffer);
				lcd_puts("  ");
			break;
			
			case SM_RESET_TEMP: // reset min/max temp
				lcd_puts("Reset temp  :>");
			break;
			case SM_GROW_START_DAY: // grow date set
				lcd_puts(cStrGrow);
				sprintf(mStrBuffer, "st:>%02d-%02d-20%02d", mGrowStartDate.Day, mGrowStartDate.Month, mGrowStartDate.Year);
				lcd_puts(mStrBuffer);
			break;
			case SM_GROW_START_MONTH: //Clock hours
				lcd_puts(cStrGrow);
				sprintf(mStrBuffer, "st: %02d>%02d-20%02d", mGrowStartDate.Day, mGrowStartDate.Month, mGrowStartDate.Year);
				lcd_puts(mStrBuffer);
			break;
			case SM_GROW_START_YEAR: //Clock hours
				lcd_puts(cStrGrow);
				sprintf(mStrBuffer, "st: %02d-%02d>20%02d", mGrowStartDate.Day, mGrowStartDate.Month, mGrowStartDate.Year);
				lcd_puts(mStrBuffer);
			break;
			case SM_GROW_DURATION: // growing duration on days
				lcd_puts(cStrGrow);
				sprintf(mStrBuffer, "days:>%3d      ", mGrowDuration);
				lcd_puts(mStrBuffer);
			break;
			case SM_RESET_WIFI:
				lcd_puts("Reset WIFI  :>");
			break;
		}
	}
}
/*
--------------------
LAMP:OFF  till 22:12
FAN :FULL  VENT: OFF
DATE:22-02-17  22:31
{                  }
--------------------
Tout:22.5  21.0/23.3
Tin1:23.5  22.0/25.3
Tin2:24.4  24.0/25.2
Hum%:22.5  10.0/99.3 

--------------------
Tout:m22:55 - M11:34
Tin1:m23:13 - M21:22
Tin2:m20:45 - M12:54
Hum%:m23:35 - M13:12
--------------------
WiFi

IP   :
State:
*/
void DrawFrame()
{
	lcd_clear();
	lcd_goto(0,0);
	if (mCurremtFrame == 0)
	{
		lcd_puts("LAMP:");
		lcd_goto(9,0);
		lcd_puts("till");
		lcd_goto(0,1);
		lcd_puts("FAN :");

		lcd_goto(0,2);
		lcd_puts("DATE:");
		lcd_goto(0,3);
		lcd_putc('{');
		lcd_goto(19,3);
		lcd_putc('}');	
	}
	else if (mCurremtFrame < 3)
	{
		lcd_puts("Tin1:");
		lcd_goto(0,1);
		lcd_puts("Tin2:");

		lcd_goto(0,2);
		lcd_puts("Tout:");
		lcd_goto(0,3);
		lcd_puts("Hmd%:");
	}
	else if (mCurremtFrame == 3)
	{
	/*	lcd_puts("WiFi");
		lcd_goto(0,2);
		lcd_puts("IP:");*/
		//lcd_goto(0,3);
		//lcd_puts("State:");
	}
}

void DrawDefault()
{
//	static uint8_t oldMenu;
	sTime evtTime;

	lcd_goto(5,0);
	if (mCurremtFrame == 0)
	{
		if (mLigntMode)
		{
			evtTime = GetNightStartTime();
			lcd_puts("ON ");
			lcd_goto(15,0);
			sprintf(mStrBuffer, "%02d:%02d", evtTime.Hour, evtTime.Minute);
			lcd_puts(mStrBuffer);
		}	
		else
		{
			evtTime = GetNightEndTime();
			lcd_puts("OFF");
			lcd_goto(15,0);
			sprintf(mStrBuffer, "%02d:%02d", evtTime.Hour, evtTime.Minute);
			lcd_puts(mStrBuffer);
		}
	

		lcd_goto(5,1);
		switch(mFanMode)
		{
			case 0:
				lcd_puts("OFF ");
				break;
			case 1:
				lcd_puts("HALF");
				break;
			case 2:
				lcd_puts("FULL");
			break;
		}
	
		lcd_goto(12,1);
		if ((mCurrentTime.Second % 4)>=2)
		{
			lcd_puts("VENT:");
			if (mVentMode)
				lcd_puts("ON ");
			else
				lcd_puts("OFF");
		}
		else
		{
			lcd_puts("HEAT:");
			if (mHeaterMode)
				lcd_puts("ON ");
			else
				lcd_puts("OFF");
		}

		lcd_goto(5,2);
		sprintf(mStrBuffer, cStrDateTime+1, mCurrentDate.Day, mCurrentDate.Month, mCurrentDate.Year, "", mCurrentTime.Hour, mCurrentTime.Minute);
		if ((mCurrentTime.Second % 2)==1)
			mStrBuffer[12] = ':';
		else
			mStrBuffer[12] = ' ';
	
		lcd_puts(mStrBuffer);
	}
	else if(mCurremtFrame == 1)
	{
		lcd_goto(5,2);
		dtostrf(mCurrentTempOut, 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
		lcd_puts("  ");
		dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_OUT + 0)), 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
		lcd_goto(15,2);
		lcd_putc('/');
		dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_OUT + 6)), 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);

		lcd_goto(5,0);
		dtostrf(mCurrentTempIn, 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
		lcd_puts("  ");
		dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN + 0)), 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
		lcd_goto(15,0);
		lcd_putc('/');
		dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN + 6)), 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
	
		lcd_goto(5,1);
		dtostrf(mCurrentTempIn_dht, 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
		lcd_puts("  ");
		dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN_DHT + 0)), 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
		lcd_goto(15,1);
		lcd_putc('/');
		dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN_DHT + 6)), 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
		
		lcd_goto(5,3);
		dtostrf(mHumidity, 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
		lcd_puts("  ");
		dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_HUMIDITY + 0)), 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
		lcd_goto(15,3);
		lcd_putc('/');
		dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_HUMIDITY + 6)), 2, 1, mStrBuffer);
		lcd_puts(mStrBuffer);
	}
	else if(mCurremtFrame == 2)
	{
		lcd_goto(6,2);
		sprintf(mStrBuffer, cStrTime, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_OUT + 4), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_OUT + 5));
		lcd_puts(mStrBuffer);
		lcd_puts(cStrMinMaxTime);
		sprintf(mStrBuffer, cStrTime, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_OUT + 10), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_OUT + 11));
		lcd_puts(mStrBuffer);

		lcd_goto(6,0);
		sprintf(mStrBuffer, cStrTime, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN + 4), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN + 5));
		lcd_puts(mStrBuffer);
		lcd_puts(cStrMinMaxTime);
		sprintf(mStrBuffer, cStrTime, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN + 10), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN + 11));
		lcd_puts(mStrBuffer);
		
		lcd_goto(6,1);
		sprintf(mStrBuffer, cStrTime, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN_DHT + 4), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN_DHT + 5));
		lcd_puts(mStrBuffer);
		lcd_puts(cStrMinMaxTime);
		sprintf(mStrBuffer, cStrTime, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN_DHT + 10), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN_DHT + 11));
		lcd_puts(mStrBuffer);
				
		lcd_goto(6,3);
		sprintf(mStrBuffer, cStrTime, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_HUMIDITY + 4), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_HUMIDITY + 5));
		lcd_puts(mStrBuffer);
		lcd_puts(cStrMinMaxTime);
		sprintf(mStrBuffer, cStrTime, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_HUMIDITY + 10), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_HUMIDITY + 11));
		lcd_puts(mStrBuffer);
	}
	else if(mCurremtFrame == 3) 
	{
		/*lcd_goto(4, 2);
		sprintf(mStrBuffer, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
		lcd_puts(mStrBuffer);
		
		lcd_goto(5, 0);
		if (mConnectionState[0] != 0 || mConnectionState[1] != 0 || mConnectionState[2] != 0 || mConnectionState[3] != 0)
			lcd_puts("Connected");
		else if (mESPState == esServerActive || mESPState == esGotIP)
			lcd_puts("Ready    ");
		else if (mESPState == esNone)
			lcd_puts("Not ready");
		else if (mESPState == esHardwareError)
			lcd_puts("Error    ");*/
		if (mUARTTail != mUARTHead)
		{
			lcd_clear();
			char str[5];
			itoa(mUARTTail, str, 10);

			lcd_goto(0,0);

			lcd_puts(str);
			lcd_putc(' ');

			itoa(mUARTHead, str, 10);
			lcd_puts(str);
			lcd_putc(' ');
			lcd_goto(0,1);
			static uint16_t qqq;
			if (qqq >= 20 + mUARTHead)
				qqq = 0;
			else 
				qqq++;
			for (int i = qqq; i<mUARTHead && i < 20; i++)
				lcd_putc(mUARTBuffer[i]);
			if (mUARTHead - mUARTTail > 20)
			{
				lcd_goto(0,2);
				for (int i =  20; i<mUARTHead && i < 40; i++)
					lcd_putc(mUARTBuffer[i]);

				if (mUARTHead > 40)
				{
					lcd_goto(0,3);
					for (int i =  40; i<mUARTHead && i < 60; i++)
						lcd_putc(mUARTBuffer[i]);
				}
			}
		}

	}
}

void SetRelays()
{
//#define RELSHIFTERENABLED
#ifdef RELSHIFTERENABLED
	uint8_t tempRel = mRelays;
	for (int i =0; i<8; i++)
	{
		RELAYPORT &= ~(1<<RELCP);
		if (tempRel & 0x80)	
			RELAYPORT &= ~(1<<RELDSB);
		else
			RELAYPORT |= (1<<RELDSB);
		_delay_us(5);
		RELAYPORT |= (1<<RELCP);
		_delay_us(5);
		tempRel <<= 1;
	}
#else // RELSHIFTERENABLED
#ifdef INVERTEDRELAYS
	if (mRelays & 0x01) // inverted (relay activated on by 0)
		PORTB &= ~(1<<REL1PIN);
	else
		PORTB |= (1<<REL1PIN);

	if (mRelays & 0x02) // inverted (relay activated on by 0)
		PORTB &= ~(1<<REL2PIN);
	else
		PORTB |= (1<<REL2PIN);

	if (mRelays & 0x04) // inverted (relay activated on by 0)
		PORTB &= ~(1<<REL3PIN);
	else
		PORTB |= (1<<REL3PIN);

	if (mRelays & 0x08) // inverted (relay activated on by 0)
		PORTD &= ~(1<<REL4PIN);
	else
		PORTD |= (1<<REL4PIN);
#else 
	if (mRelays & 0x01) // noninverted (relay activated on by 1)
		PORTB |= (1<<REL1PIN);
	else
		PORTB &= ~(1<<REL1PIN);

	if (mRelays & 0x02) // noninverted (relay activated on by 1)
		PORTB |= (1<<REL2PIN);
	else
		PORTB &= ~(1<<REL2PIN);

	if (mRelays & 0x04) // noninverted (relay activated on by 1)
		PORTB |= (1<<REL3PIN);
	else
		PORTB &= ~(1<<REL3PIN);

	if (mRelays & 0x08) // noninverted (relay activated on by 1)
		PORTD |= (1<<REL4PIN);
	else
		PORTD &= ~(1<<REL4PIN);
#endif 

	if (mRelays & 0x10) 
		PORTD |= (1<<REL5PIN);
	else
		PORTD &= ~(1<<REL5PIN);

	if (mRelays & 0x20) 
		PORTD |= (1<<REL6PIN);
	else
		PORTD &= ~(1<<REL6PIN);

// 	if (mRelays & 0x02)
// 		PORTB &= ~(1<<REL7PIN);
// 	else
// 		PORTB |= (1<<REL7PIN);
// 
// 	if (mRelays & 0x01)
// 		PORTB &= ~(1<<REL8PIN);
// 	else
// 		PORTB |= (1<<REL8PIN);		

#endif 
}
			
void InitTimer()
{
	TIMSK0 |= (1<<TOIE0);
	TCCR0B |= (1<<CS00) | (1<<CS02);				
}

void InitSwitches()
{
	//SWITCHPORTDIR &= ~((1<<BTN1PIN) | (1<<BTN2PIN) | (1<<BTN3PIN));
	DDRD &= ~((1<<BTN1PIN) | (1<<BTN2PIN) | (1<<BTN3PIN));
	PORTD |= (1<<BTN1PIN) | (1<<BTN2PIN) | (1<<BTN3PIN);
	PCICR |= (1<<PCIE2);
	PCMSK2 |= (1<<PCINT18) | (1<<PCINT19) | (1<<PCINT20);
}

void InitRelays()
{
	DDRB |= (1<<REL1PIN) | (1<<REL2PIN) | (1<<REL3PIN);
	DDRD |= (1<<REL4PIN) | (1<<REL5PIN) | (1<<REL6PIN);// | (1<<REL7PIN) | (1<<REL8PIN);
	
// 	mRelays = 0xff;
// 	SetRelays();
// 	_delay_ms(10000);
// 	mRelays = 0x00;
// 	SetRelays();
}



//  sEventToggled
//  0 - none (all normal);
// 	1 - high temp
// 	2 - low with high outside
// 	3 - low with low outside
#define MAINTTEMPMIN (mLigntMode == 1?mMaintainTempDayMin:mMaintainTempNightMin)
#define MAINTTEMPMAX (mLigntMode == 1?mMaintainTempDayMax:mMaintainTempNightMax)
uint8_t GetTempStatus()
{
	float currentTempIn = GetInSensorTemp(); // it is cache for temp. hate this
	
	if (currentTempIn > mCriticalTemp)
		return 5;
	if (currentTempIn > MAINTTEMPMAX + (double)mDeltaTemp/10.00)				// high temp
		return 1;
	else if (currentTempIn < MAINTTEMPMIN - (double)mDeltaTemp/10.00)			// low temp and ...
	{
		if (mCurrentTempOut > MAINTTEMPMIN + (double)mDeltaTemp/10.00)			// ... warmer outside
			return 2;
		else																	// ... colder outside
			return 3;
	}
	else if (currentTempIn <= MAINTTEMPMAX && currentTempIn >= MAINTTEMPMIN)	// normal
		return 0;
	else // in delta transit 
		return 4;
}
// 
// 				Program Memory Usage 	:	18342 bytes   56,0 % Full
// 				Data Memory Usage 		:	1514 bytes   73,9 % Full
float GetInSensorTemp()
{
	switch(mTempSensor)
	{
		case 0:
			return mCurrentTempIn;
		case 1:
			return mCurrentTempIn_dht;
		default:
			return (mCurrentTempIn + mCurrentTempIn_dht) / 2.0;
	}
}

void CheckHumidity()
{
	if (isnan(mHumidity))
		return;
	if (mHumidity < mMaintainHumidity)
	{
		mRelays |= (1<<HUMIDIFYER_REL);
	}
	else
	{
		mRelays &= ~(1<<HUMIDIFYER_REL);
	}
}

void CheckTemperature()
{
	switch(GetTempStatus())
	{
		case 1:
			if (sEventToggled != 1) // high temp 
			{
				sEventToggled = 1;				
				if (mBuzzerMode != 4)
					SetBuzzer(4);
				if (mAutoMode == 1 && mCurrentTempOut < GetInSensorTemp())
				{
					if (mHeaterMode)
						SetHeater(0);
					if (mVentMode != 1)
						SetVentFan(1);
				}
			}
		break;
		case 2:
			if (sEventToggled != 2) // warmer outside
			{
				if (mLigntMode == 0 && mFanMode == 0) // lamp is off and fan is off
				{
					sEventToggled = 2;
					if (mBuzzerMode != 5)
						SetBuzzer(5);
					if (mAutoMode == 1)
						SetFan(1);
				}
			}
		break;
		case 3:
			if (sEventToggled != 3) // cold outside so using heater
			{
				sEventToggled = 3;
				if (mBuzzerMode != 5)
					SetBuzzer(5);
				if ((mAutoMode == 1) && !mHeaterMode) 
				{
					if (mVentMode)
						SetVentFan(0);
					SetHeater(1);
				}
			}	
		break;
		case 0:
			if (sEventToggled == 1) // normal temp after high
			{
				if (mAutoMode == 1 && mVentMode)
					SetVentFan(0);
				if (mBuzzerMode != 0)
					SetBuzzer(0);
				sEventToggled = 0;
			} 
			else if (sEventToggled >= 2) // normal temp after low
			{ 	
				if (mAutoMode == 1 && mHeaterMode) 
				{
					SetHeater(0);
				}
				if (mAutoMode == 1 && mLigntMode == 0 && mCoolingMode >= 3 && sAfterLampOffMinutes == 0 && mFanMode > 0)
					SetFan(0);	
				if (mBuzzerMode != 0)
					SetBuzzer(0);
				sEventToggled = 0;
			}
		case 4: // in transit (delta)
		break;
		case 5: // critical temp
			if (sEventToggled != 5) // critical temp
			{
				sEventToggled = 5;
				if (mLigntMode != 0)
					SetLight(false);
				if (mHeaterMode != 0)
					SetHeater(0);
				if (mVentMode != 0)
					SetVentFan(0);
				if (mFanMode != 0)
					SetFan(0);
				if (mBuzzerMode != 4)
					SetBuzzer(6);
 			}
		break;
	}
}

ISR(TIMER0_OVF_vect)
{
	if (Btn1RelaxCycleCounter != 0) 
	{
		Btn1RelaxCycleCounter--;
		if (Btn1RelaxCycleCounter == 0) // restoring state without rising state change
		{
			if (SWITCHPORTPINS & (1<<BTN1PIN))
				ButtonStateCurrent |= 0x01;
			else // 0 = pressed
				ButtonStateCurrent &= ~0x01;
		}
	}

	if (Btn2RelaxCycleCounter != 0)
	{
		Btn2RelaxCycleCounter--;
		if (Btn2RelaxCycleCounter == 0) // restoring state without rising state change
		{
			if (SWITCHPORTPINS & (1<<BTN2PIN))
				ButtonStateCurrent |= 0x02;
			else // 0 = pressed
				ButtonStateCurrent &= ~0x02;
		}
	}
	
	if (Btn3RelaxCycleCounter != 0)
	{
		Btn3RelaxCycleCounter--;
		if (Btn3RelaxCycleCounter == 0) // restoring state without rising state change
		{
//  			if ((ButtonStateCurrent & 0x04) ^ (SWITCHPORTPINS & (1<<BTN3PIN)))
//  				ButtonStateChanged |= 0x04;
			if (SWITCHPORTPINS & (1<<BTN3PIN))
			{
				ButtonStateCurrent |= 0x04;
				ButtonWasLongPressed &= ~0x04;
			}
			else // 0 = pressed
				ButtonStateCurrent &= ~0x04;
		}
	}

	if (!(SWITCHPORTPINS & (1<<BTN1PIN)))
	{
		if (++Btn1LongPressCycleCounter == BTNLONGPRESSCYCLECOUNTER)
		{
			ButtonLongPressed |= 0x01;
			ButtonWasLongPressed |= 0x01;
			ButtonStateChanged |= 0x01;
			Btn1LongPressCycleCounter = 0;
			//Btn1RelaxCycleCounter = BTNLONGPRESSCYCLECOUNTER;
		}
	}
	else
		Btn1LongPressCycleCounter = 0;
		
	if (!(SWITCHPORTPINS & (1<<BTN2PIN)))
	{
		if (++Btn2LongPressCycleCounter == BTNLONGPRESSCYCLECOUNTER)
		{
			ButtonLongPressed |= 0x02;
			ButtonWasLongPressed |= 0x02;
			ButtonStateChanged |= 0x02;
			Btn2LongPressCycleCounter = 0;
//			Btn2RelaxCycleCounter = BTNLONGPRESSCYCLECOUNTER;
		}
	}
	else
		Btn2LongPressCycleCounter = 0;
	
	if (!(SWITCHPORTPINS & (1<<BTN3PIN)))
	{
		if (++Btn3LongPressCycleCounter == BTNLONGPRESSCYCLECOUNTER)
		{
			ButtonLongPressed |= 0x04;
			ButtonWasLongPressed |= 0x04;
			ButtonStateChanged |= 0x04;
			Btn3LongPressCycleCounter = 0;
//			Btn3RelaxCycleCounter = BTNLONGPRESSCYCLECOUNTER;
		}
	}
	else
		Btn3LongPressCycleCounter = 0;
}
//uint8_t mDiscardNextEvent;
ISR(PCINT2_vect)
{
	//ButtonStateChanged |= 0x01;
	//uart_puts("Btn pressed\r\n");
	if ((Btn1RelaxCycleCounter == 0) && (((ButtonStateCurrent) & 0x01) ^ ((SWITCHPORTPINS >> BTN1PIN) & 0x01))) //btn1 state changed?
	{
		if (ButtonWasLongPressed & 0x01)
			ButtonWasLongPressed &= ~0x01;
		else
			ButtonStateChanged |= 0x01;
		
		if (SWITCHPORTPINS & (1<<BTN1PIN))
			ButtonStateCurrent |= 0x01;
		else // 0 = pressed
			ButtonStateCurrent &= ~0x01;

		Btn1RelaxCycleCounter = BUTTONRELAXCYCLECOUNT;
	}
	
	if ((Btn2RelaxCycleCounter == 0) && (((ButtonStateCurrent >> 0x1) & 0x01) ^ ((SWITCHPORTPINS >> BTN2PIN) & 0x01))) //btn2 state changed?
	{
		if (ButtonWasLongPressed & 0x02)
			ButtonWasLongPressed &= ~0x02;
		else
			ButtonStateChanged |= 0x02;
			
		if (SWITCHPORTPINS & (1<<BTN2PIN))
			ButtonStateCurrent |= 0x02;
		else // 0 = pressed
			ButtonStateCurrent &= ~0x02;

		Btn2RelaxCycleCounter = BUTTONRELAXCYCLECOUNT;
	}
	
	if ((Btn3RelaxCycleCounter == 0) && (((ButtonStateCurrent >> 0x2) & 0x01) ^ ((SWITCHPORTPINS >> BTN3PIN) & 0x01))) //btn3 state changed?
	{
		if (ButtonWasLongPressed & 0x04)
			ButtonWasLongPressed &= ~0x04;
		else
			ButtonStateChanged |= 0x04;

		if (SWITCHPORTPINS & (1<<BTN3PIN))
			ButtonStateCurrent |= 0x04;
		else // 0 = pressed
			ButtonStateCurrent &= ~0x04;

		Btn3RelaxCycleCounter = BUTTONRELAXCYCLECOUNT;
	}
	
	//_delay_ms(100);
	//PCIFR &= (1<<PCINT8);
}

sTime GetNightStartTime()
{
	sTime time;
	switch(mNightMode)
	{
		case 0:
			time.Hour = mNightStartTime.Hour;
			time.Minute = mNightStartTime.Minute;
		break;
		case 1:
			time.Hour = mNightStartTime.Hour + (mNightDurationStart - mNightDurationCurr / 60);
			time.Minute = mNightStartTime.Minute + ((mNightDurationStart * 60 - mNightDurationCurr) % 60);
		break;
		case 2:
			time.Hour = mNightStartTime.Hour + (mNightDurationStart - mNightDurationCurr / 60) / 2;
			time.Minute = mNightStartTime.Minute + ((mNightDurationStart * 60 - mNightDurationCurr) % 60) / 2;
		break;
	}
// 	char str[20];
// 	mNightDurationCurr = 330;
// 	sprintf(str, "%02d %02d", mNightStartTime.Minute, (mNightDurationStart * 60 - mNightDurationCurr) % 60);
// 	lcd_goto(0,3);
// 	lcd_puts(str);
	// overflow check
	if (time.Minute > 59)
	{
		time.Hour++;
		time.Minute -= 60;
	}
	if (time.Hour > 24)
		time.Hour -= 24;
		
	return time;
}

sTime GetNightEndTime()
{
	sTime time;
	
	time.Hour = 12;
	time.Minute = 12;
	
	switch(mNightMode)
	{
		case 0:
			time.Hour = mNightStartTime.Hour + mNightDurationCurr / 60;
			time.Minute = mNightStartTime.Minute + mNightDurationCurr % 60;
		break;
		case 1:
			time.Hour = mNightStartTime.Hour + mNightDurationStart;
			time.Minute = mNightStartTime.Minute;
		break;
		case 2:
			time.Hour = (mNightStartTime.Hour + mNightDurationStart) - (mNightDurationStart - mNightDurationCurr / 60) / 2;
			time.Minute = mNightStartTime.Minute + ((mNightDurationStart * 60 - mNightDurationCurr) % 60) / 2;
		break;
	}
	
	// overflow check
	if (time.Minute > 59)
	{
		time.Hour++;
		time.Minute -= 60;
	}
	if (time.Hour > 24)
		time.Hour -= 24;
	
	return time;
}
char* mMessage;
static int8_t mPos;
uint8_t msgIdx = 0;
char strMsg[31];
char strDbl[6];

int16_t GetDateDiff(sDate d1, sDate d2)
{
	int16_t diff;
	diff = (d2.Year - d1.Year) * 365;
	if (d2.Month >= d1.Month) /// optimize this !!!
	{
		for(uint8_t i = d1.Month; i<d2.Month; i++)
			diff += mMonyhDays[i];
		diff = diff - d1.Day + d2.Day;
	}
	else
	{
		for(uint8_t i = d2.Month; i<d1.Month; i++)
			diff -= mMonyhDays[i];
		diff = diff + d2.Day - d2.Day;
	}
	return diff;
}

void SetNextMessage()
{
	if (++msgIdx > 5)
		msgIdx = 0;
	
	switch(msgIdx)
	{
		case 0:
			GetDate(mCurrentDate);
			sprintf(strMsg, "%02d-%02d-20%02d %2s", mCurrentDate.Day, mCurrentDate.Month, mCurrentDate.Year, mDOWNames[mCurrentDate.DayOfWeek-1]);

			mMessage = strMsg;
		break;
		case 1: 
			
			sprintf(strMsg, "Day %d of %d ", GetDateDiff(mGrowStartDate, mCurrentDate),  mGrowDuration);
			mMessage = strMsg;
		break;
		case 2: 
			dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN + 0)), 2, 1, strDbl);
			sprintf(strMsg, "Min temp in :%s at %02d:%02d", strDbl, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN + 4), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN + 5));
			mMessage = strMsg;
		break;
		case 3:
			dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN + 6)), 2, 1, strDbl);
			sprintf(strMsg, "Max temp in :%s at %02d:%02d", strDbl, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN + 10), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_IN + 11));
			mMessage = strMsg;
		break;
		case 4:
			dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_OUT + 0)), 2, 1, strDbl);
			sprintf(strMsg, "Min temp out:%s at %02d:%02d", strDbl, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_OUT + 4), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_OUT + 5));
			mMessage = strMsg;
		break;
		case 5:
			dtostrf(eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_OUT + 6)), 2, 1, strDbl);
			sprintf(strMsg, "Max temp out:%s at %02d:%02d", strDbl, eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_OUT + 10), eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_TEMP_OUT + 11));
			mMessage = strMsg;
		break;

	}
	mPos = -18;
}

void SetMessage(char* pMessage)
{
	mMessage = pMessage;
	mPos = -20;
}

void SendReport()
{
	//return;
	for(int i = 0; i< 4; i++)
	if (mConnectionState[i] != 0 )
	{
		char msg[100];
		switch(mConnectionReportIndex[i])
		{
			case 0:
				sprintf(msg, "#main:");
				msg[6] = mLigntMode;
				msg[7] = mFanMode;
				msg[8] = mRelays & (1<<VENT_REL);
				msg[9] = mRelays & (1<<HEATER_REL);
				msg[10] = mRelays & (1<<HUMIDIFYER_REL);
				msg[11] = sEventToggled;
				msg[12] = mHumidity < mMaintainHumidity;
				msg[13] = mCurrentDate.Year;
				msg[14] = mCurrentDate.Month;
				msg[15] = mCurrentDate.Day;
				msg[16] = mCurrentDate.DayOfWeek;
				msg[17] = mCurrentTime.Hour;
				msg[18] = mCurrentTime.Minute;
				msg[19] = mCurrentTime.Second;
				msg[20] = mGrowStartDate.Year;
				msg[21] = mGrowStartDate.Month;
				msg[22] = mGrowStartDate.Day;
				SendData(i, msg, 23);
				break;
				mConnectionReportIndex[i] = 10;
			case 1:
				sprintf(msg, "#sens:");
				((float*)(msg + 6))[0] = mCurrentTempIn;
				((float*)(msg + 10))[0] = eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN + 0));
				((int16_t*)(msg + 14))[0] = eeprom_read_word((uint16_t*)(EEPROM_ADDRESS_TEMP_IN + 4));
				((float*)(msg + 16))[0] = eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN + 6));
				((int16_t*)(msg + 20))[0] = eeprom_read_word((uint16_t*)(EEPROM_ADDRESS_TEMP_IN + 10));

				((float*)(msg + 22))[0] = mCurrentTempIn_dht;
				((float*)(msg + 26))[0] = eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN_DHT + 0));
				((int16_t*)(msg + 30))[0] = eeprom_read_word((uint16_t*)(EEPROM_ADDRESS_TEMP_IN_DHT + 4));
				((float*)(msg + 32))[0] = eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_IN_DHT + 6));
				((int16_t*)(msg + 36))[0] = eeprom_read_word((uint16_t*)(EEPROM_ADDRESS_TEMP_IN_DHT + 10));
				
				((float*)(msg + 38))[0] = mCurrentTempOut;
				((float*)(msg + 42))[0] = eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_OUT + 0));
				((int16_t*)(msg + 46))[0] = eeprom_read_word((uint16_t*)(EEPROM_ADDRESS_TEMP_OUT + 4));
				((float*)(msg + 48))[0] = eeprom_read_float((float*)(EEPROM_ADDRESS_TEMP_OUT + 6));
				((int16_t*)(msg + 52))[0] = eeprom_read_word((uint16_t*)(EEPROM_ADDRESS_TEMP_OUT + 10));
	
				((float*)(msg + 54))[0] = mHumidity;
				((float*)(msg + 58))[0] = eeprom_read_float((float*)(EEPROM_ADDRESS_HUMIDITY + 0));
				((int16_t*)(msg + 62))[0] = eeprom_read_word((uint16_t*)(EEPROM_ADDRESS_HUMIDITY + 4));
				((float*)(msg + 64))[0] = eeprom_read_float((float*)(EEPROM_ADDRESS_HUMIDITY + 6));
				((int16_t*)(msg + 68))[0] = eeprom_read_word((uint16_t*)(EEPROM_ADDRESS_HUMIDITY + 10));
				SendData(i, msg, 69);
				mConnectionReportIndex[i] = 10;
				break;
			case 2:
				sprintf(msg, "#sett:");
				msg[6] = mNightStartTime.Hour;
				msg[7] = mNightStartTime.Minute;
				msg[8] = mNightDurationStart;
				msg[9] = mNightDurationEnd;
				msg[10] = mNightDurationDelta;
				((uint16_t*)(msg + 11))[0] = mNightDurationCurr;
				msg[13] = mNightMode;
				msg[14] = mMaintainTempNightMin;
				msg[15] = mMaintainTempNightMax;
				msg[16] = mMaintainTempDayMin;
				msg[17] = mMaintainTempDayMax;
				msg[18] = mMaintainHumidity;
				msg[19] = mDeltaTemp;
				msg[20] = mTempSensor;
				msg[21] = mCriticalTemp;
				msg[22] = mCoolingMode;
				msg[23] = mAutoMode;
				
				SendData(i, msg, 24);
				mConnectionReportIndex[i] = 10;
				break;
			case 3: //#netw:<1byte-len><apname><1byte-len><appass>
				uint8_t apNameLen = 4;//eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_AP_NAME);
				uint8_t apPassLen = 8;//eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_AP_PASSWORD);
				
				sprintf(msg, "#netw:");
				msg[6] = apNameLen;		
				eeprom_read_block(msg + 7, (void*)(EEPROM_ADDRESS_AP_NAME + 1), apNameLen);
				msg[6 + apNameLen + 1] = eeprom_read_byte((uint8_t*)EEPROM_ADDRESS_AP_PASSWORD);
				eeprom_read_block(msg + 6 + apNameLen + 2, (void*)(EEPROM_ADDRESS_AP_PASSWORD + 1), apPassLen);
				
				//msg[21] = 0;
				//lcd_goto(0,0); 
				//lcd_puts(msg);
				SendData(i, msg, 7 + apNameLen + 1 + apPassLen);
				mConnectionReportIndex[i] = 10;
				break;				
		}
		
	}
}

static uint8_t cnt;
//static uint8_t oldPos;
bool ScrollMessage()
{
	if (mMenuLevel != 0)
		return false;
	bool retVal = false;
	if (mPos > (int8_t)strlen(mMessage)) // optimize call of strlen()
	{
		retVal = true;
		mPos = -18;
	}

	if (((mPos == 1 || (mPos > 1 && strlen(mMessage) - mPos == 17)) && cnt < 20) || (mPos > 0 && cnt <2 && (strlen(mMessage) - mPos > 17 )))
		cnt++;
	else
	{
		if (mPos <= 0)
			lcd_goto(-mPos+1,3);
		else
			lcd_goto(1,3);

		uint8_t charPos = (mPos<0?0:mPos);
		if (strlen(mMessage) - mPos >= 18)
			for(uint8_t i = charPos; i<18+mPos;i++)
				lcd_putc(mMessage[i]);
		else
		{
			lcd_puts(mMessage+charPos);
			if (strlen(mMessage) - charPos != 18)
				lcd_putc(' ');
		}
		++mPos;
		cnt = 0;
	}
	//++mPos;
	return retVal;
}