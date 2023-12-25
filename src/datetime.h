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
	DateTime(const Json& data) : hour(data["hour"].get<uint8_t>()), day(data["day"].get<uint16_t>()), year(data["year"].get<uint16_t>()) { }
	Json toJson() const
	{
		Json output;
		output["hour"] = hour;
		output["day"] = day;
		output["year"] = year;
		return output;
	}
};
