#include "datetime.h"
#include "config.h"
#include <cstdint>
#include <cassert>
DateTime::DateTime(Step step)
{
	year = step / Config::stepsPerYear;
	step -= year * Config::stepsPerYear;
	day = (step / Config::stepsPerDay);
	step -= day * Config::stepsPerDay;
	hour = (step / Config::stepsPerHour) + 1;
	++day;
	assert(day <= Config::daysPerYear);
	assert(hour <= Config::hoursPerDay);
}
DateTime::DateTime(uint8_t h, uint16_t d, uint16_t y) : hour(h), day(d), year(y) 
{
	if(!hour)
	{
		assert(!day);
		assert(!year);
	}
}
Step DateTime::toSteps()
{
	return toSteps(hour, day, year);
}
// Static methods.
Step DateTime::toSteps(uint8_t hour, uint16_t day, uint16_t year)
{
	assert(day);
	assert(hour);
	return (year * Config::stepsPerYear) + (--day * Config::stepsPerDay) + (--hour * Config::stepsPerHour);
}
uint8_t DateTime::toSeason(Step step)
{
	DateTime dateTime(step);
	uint8_t output =  float(dateTime.day - 1) / ((float)Config::daysPerYear / 4.f);
	assert(output >= 0);
	assert(output < 4);
	return output;
}
