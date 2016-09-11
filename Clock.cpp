#include "Clock.h"
#include "Wire.h"		// I2C = clock
#include <inttypes.h>

#define DS1307_ADDRESS 0x68		// RTC
const uint8_t zero = 0x00;

// ctor
Clock::Clock()
{
}

Clock::~Clock()
{
}

void Clock::GetDateTime()
{
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(zero);
	Wire.endTransmission();

	Wire.requestFrom(DS1307_ADDRESS, 7);

	this->second = bcdToDec(Wire.read());
	this->minute = bcdToDec(Wire.read());
	this->hour = bcdToDec(Wire.read() & 0b111111);	//24 hour time
	this->weekDay = bcdToDec(Wire.read());			//0-6 -> sunday - saturday
	this->monthDay = bcdToDec(Wire.read());
	this->month = bcdToDec(Wire.read());
	this->year = bcdToDec(Wire.read());
}

void Clock::SetDateTime(uint8_t sec, uint8_t min, uint8_t hr, uint8_t wkDay, uint8_t moDay, uint8_t mo, uint8_t yr)
{
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(zero);

	Wire.write(decToBcd(sec));
	Wire.write(decToBcd(min));
	Wire.write(decToBcd(hr));
	Wire.write(decToBcd(wkDay));
	Wire.write(decToBcd(moDay));
	Wire.write(decToBcd(mo));
	Wire.write(decToBcd(yr));

	Wire.write(zero); //start
	Wire.endTransmission();
}

uint8_t Clock::decToBcd(uint8_t val)
{
	// Convert normal decimal numbers to binary coded decimal
	return ((val / 10 * 16) + (val % 10));
}

uint8_t Clock::bcdToDec(uint8_t val)
{
	// Convert binary coded decimal to normal decimal numbers
	return ((val / 16 * 10) + (val % 16));
}
