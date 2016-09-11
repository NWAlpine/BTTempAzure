#pragma once
#include <inttypes.h>
class Clock
{

public:
	Clock();
	~Clock();

	void GetDateTime();
	void SetDateTime(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
	uint8_t second;		//0-59
	uint8_t minute;		//0-59
	uint8_t hour;		//0-23
	uint8_t weekDay;	//0-6 -> sunday - saturday
	uint8_t monthDay;	//1-31
	uint8_t month;		//1-12
	uint8_t year;		//0-99

private:
	uint8_t decToBcd(uint8_t);
	uint8_t bcdToDec(uint8_t);

};
