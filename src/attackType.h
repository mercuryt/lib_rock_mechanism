#pragma once

#include "woundType.h"

#include <string>
struct AttackType
{
	std::string name;
	uint32_t area;
	uint32_t baseForce;
	uint32_t range;
	const WoundType& woundType;
};
