#pragma once
#include "config.h"
#include <cstdint>
#include <cassert>
struct DateTime
{
	//TODO: Add Minute and second?
	uint8_t hour;
	uint16_t day;
	uint16_t year;
	DateTime(uint8_t h, uint16_t d, uint16_t y) : hour(h), day(d), year(y)
	{
		assert(hour != 0);
		assert(hour <= Config::hoursPerDay);
		assert(day != 0);
		assert(day <= Config::daysPerYear);
		assert(year != 0);
	}
	DateTime() : hour(0), day(0), year(0) { }
};
inline void from_json(const Json& data, DateTime& dateTime)
{
	dateTime.hour = data["hour"].get<uint8_t>();
	dateTime.day = data["day"].get<uint16_t>();
	dateTime.year = data["year"].get<uint16_t>();
}
inline void to_json(Json& data, const DateTime& dateTime)
{
	data["hour"] = dateTime.hour;
	data["day"] = dateTime.day;
	data["year"] = dateTime.year;
}
