#pragma once
#include "../config.h"
#include <cstdint>
struct LociiConfig final
{
	uint32_t numberOfLocationsPerLocii;
	float maximumIntensity;
	float minimumIntensity;
	float maximumSustain;
	float minimumSustain;
	float maximumDecay;
	float minimumDecay;
	float maximumExponent;
	float minimumExponent;
	uint32_t maximumPositions;
	uint32_t minimumPositions;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(LociiConfig, numberOfLocationsPerLocii, maximumIntensity, minimumIntensity, maximumSustain, minimumSustain, maximumDecay, minimumDecay, maximumExponent, minimumExponent, maximumPositions, minimumPositions);
};
