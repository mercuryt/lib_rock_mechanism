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
	inline Step phaseChangeFrequency;
	inline float unitsOfFluidToMeltPerPointVolume;
	inline float unitsOfFluidToFreezePerPointVolume;
	inline CollisionVolume volumeOfRubbleToGenerateWhenSolidBurns;
	inline CollisionVolume volumeOfRubbleToGenerateWhenFeatureBurns;
	void load();
}