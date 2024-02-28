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
		if(hour == 0 || day == 0 || year == 0)
			assert(hour == day && day == year);
		assert(hour <= Config::hoursPerDay);
		assert(day <= Config::daysPerYear);
	}
	DateTime() : hour(0), day(0), year(0) { }
	[[nodiscard]] explicit operator bool() const { return hour || day || year; }
	static DateTime fromPastBySteps(Step steps)
	{
		DateTime output;
		output.year = steps / Config::stepsPerYear;
		steps -= output.year * Config::stepsPerYear;
		output.day = steps / Config::stepsPerDay;
		steps -= output.day * Config::stepsPerDay;
		output.hour = steps / Config::stepsPerHour;
		return output;
	}
	uint8_t getSeason() { return day / ((float)Config::daysPerYear / 4.f); }
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
