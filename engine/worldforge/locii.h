#pragma once

#include "../types.h"
#include "lociiConfig.h"
#include "gps.h"
#include <algorithm>
#include <cstdint>
#include <vector>

class World;
struct DeserializationMemo;

struct Locii final
{
	std::vector<LatLng> positions;
	const float intensity;
	// How far to propigate influence without applying decay.
	const Kilometers sustain;
	// How quickly intesity is lost over distance after sustain.
	const float decay;
	// To be applied to distance to create a rounded slope.
	const float exponent;
	const Kilometers maxRange;
	Locii(std::vector<LatLng>& p, float i, Kilometers s, float d, float e) : positions(p), intensity(i), sustain(s), decay(d), exponent(e),
		maxRange(sustain + (intensity * decay)) { }
	Locii(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	float intensityAtDistance(Kilometers distance) const;
	Kilometers distanceTo(Latitude latitude, Longitude longitude) const;
	static Locii make(LociiConfig& config, World& world);
};
