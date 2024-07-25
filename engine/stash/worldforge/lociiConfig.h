#pragma once
#include "../config.h"
#include <cstdint>
struct LociiConfig final
{
	uint32_t numberOfLocationsPerLocii;
	float minimumIntensity;
	float maximumIntensity;
	float minimumSustain;
	float maximumSustain;
	float minimumDecay;
	float maximumDecay;
	float minimumExponent;
	float maximumExponent;
	uint32_t minimumPositions;
	uint32_t maximumPositions;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(LociiConfig, numberOfLocationsPerLocii, minimumIntensity, maximumIntensity, minimumSustain, maximumSustain, minimumDecay, maximumDecay, minimumExponent, maximumExponent, minimumPositions, maximumPositions);
};
