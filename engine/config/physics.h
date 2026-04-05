#pragma once
#include "../numericTypes/types.h"
namespace Config::Physics
{
	inline float radiantHeatDisipatesAtDistanceExponent;
	inline TemperatureDelta minimumHeatDeltaToTrackAffectedArea;
	inline CollisionVolume volumeOfFluidToGenerateWhenAPointFeatureMelts;
	inline Distance rangeOfInfluenceForPortalsBetweenOutsideAndInside;
	inline Distance maxVolumeOfInfluenceForPortalsBetweenOutsideAndInside;
	inline float ambiantTemperatureDeltaDecay;
	void load();
}