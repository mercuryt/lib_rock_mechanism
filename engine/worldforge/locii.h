#pragma once

#include "../types.h"
#include "lociiConfig.h"
#include <algorithm>
#include <cstdint>
#include <vector>

class World;

struct Locii final
{
	const std::vector<LatLng> positions;
	const float intensity;
	// How quickly intesity is lost over distance after sustain.
	const float decay;
	// How far to propigate influence without applying decay.
	const Kilometers sustain;
	// To be applied to distance to create a rounded slope.
	const float exponent;
	const Kilometers maxRange;
	Locii(std::vector<LatLng>& p, float i, float d, Kilometers s, float e) : positions(p), intensity(i), decay(d), sustain(s), exponent(e),
		maxRange(sustain + (intensity * decay)) { }
	float intensityAtDistance(Kilometers distance) const
	{
		if(sustain >= distance)
			return intensity;
		auto modifiedDistance = distance - sustain;
		float intensityModifier = modifiedDistance * decay;
		return intensityModifier > 1.f ? 0.f : intensity * (1.f-intensityModifier);
	};
	Kilometers distanceTo(Latitude latitude, Longitude longitude)
	{
		bool found = false;
		Kilometers output = 0;
		for(const LatLng& position : positions)
		{
			Kilometers distance = position.distanceTo(latitude, longitude);
			if(!found || output > distance)
			{
				found = true;
				output = distance;
			}
		}
		if(!found)
			return UINT32_MAX;
		return output;
	}
	static Locii make(LociiConfig& config, World& world);
};
