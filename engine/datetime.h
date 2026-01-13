#pragma once
#include "numericTypes/types.h"
struct DateTime
{
	int8_t hour;
	int16_t day;
	int16_t year;
	DateTime(Step step);
	DateTime(int8_t h, int16_t d, int16_t y);
	[[nodiscard]] Step toSteps();
	[[nodiscard]] static Step toSteps(int8_t hour, int16_t day, int16_t year);
	[[nodiscard]] static int8_t toSeason(Step step);
};
