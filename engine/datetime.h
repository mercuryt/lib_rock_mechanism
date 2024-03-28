#pragma once
#include "types.h"
struct DateTime
{
	uint8_t hour;
	uint16_t day;
	uint16_t year;
	DateTime(Step step);
	DateTime(uint8_t h, uint16_t d, uint16_t y);
	Step toSteps();
	static Step toSteps(uint8_t hour, uint16_t day, uint16_t year);
	static uint8_t toSeason(Step step);
};
