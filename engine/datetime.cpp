#include "datetime.h"
#include "config/config.h"
#include <cstdint>
#include <cassert>
DateTime::DateTime(Step step)
{
	year = (step / Config::stepsPerYear).get();
	step -= Config::stepsPerYear * year;
	day = (step / Config::stepsPerDay).get();
	step -= Config::stepsPerDay * day;
	hour = (step / Config::stepsPerHour).get() + 1;
	++day;
	assert(day <= Config::daysPerYear);
	assert(hour <= Config::hoursPerDay);
}
DateTime::DateTime(int8_t h, int16_t d, int16_t y) : hour(h), day(d), year(y)
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
Step DateTime::toSteps(int8_t hour, int16_t day, int16_t year)
{
	assert(day);
	assert(hour);
	return (Config::stepsPerYear * year) + (Config::stepsPerDay * --day) + (Config::stepsPerHour * --hour);
}
int8_t DateTime::toSeason(Step step)
{
	DateTime dateTime(step);
	int8_t output = float(dateTime.day - 1) / ((float)Config::daysPerYear / 4.f);
	assert(output >= 0);
	assert(output < 4);
	return output;
}
