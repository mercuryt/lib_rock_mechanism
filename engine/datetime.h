#pragma once
#include "numericTypes/types.h"
struct DateTime
{
	int hour;
	int day;
	int year;
	DateTime(Step step);
	DateTime(int h, int d, int y);
	[[nodiscard]] Step toSteps() const;
	[[nodiscard]] static Step toSteps(int hour, int day, int year);
	[[nodiscard]] static int toSeason(Step step);
};
