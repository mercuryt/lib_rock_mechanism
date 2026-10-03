#pragma once

#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include <cmath>
#include <cstdint>
namespace Config::World
{
	inline Distance defaultAreaSize;
	inline Distance maxHumidityEffectDistance;
	inline float tropicalBandStart;
	inline float subtropicalBandStart;
	inline float midLatitudeBandStart;
	inline float polarBandStart;
	inline float equitorialRainModifierHigh;
	inline float equitorialRainModifierLow;
	inline float tropicalRainModifierHigh;
	inline float tropicalRainModifierLow;
	inline float subtropicalRainModifierHigh;
	inline float subtropicalRainModifierLow;
	inline float midlatitudeRainModifierHigh;
	inline float midlatitudeRainModifierLow;
	inline float polarRainModifierHigh;
	inline float polarRainModifierLow;
	inline Temperature defaultHighAmbiantTemperature;
	inline Temperature defaultLowAmbiantTemperature;
	inline float temperatureLossFractionPerUnitAltitude;
	inline TemperatureDelta maxAmbiantSwing;
	inline float ratioOfSurfaceToTotalAreaHeight;
	inline Distance distanceAreaLandIsWarpedBySlopeOnWorldMap;
	inline float fractionOfHeightmapToGenerateBroadPhase;
	inline Percent minimumHumidityForDirt;
	inline Distance minBedrockStartDepth;
	inline Distance maxBedrockStartDepth;
	inline Distance lavaDepth;
	inline Distance spaceToClearAboveRiver;
	inline float smoothingFactor;
	inline Distance minSmallLakeDepth;
	inline Distance maxSmallLakeDepth;
	inline int minSmallLakeBeds;
	inline int maxSmallLakeBeds;
	inline float maxRatioOfLongToShortDimensionForLakes;
	inline float maxRatioOfSmallLakeSizeToAreaSize;
	inline float minRatioOfSmallLakeSizeToAreaSize;
	inline float ratioOfAnimalMassToFoliageMass;
	inline float ratioOfCarnivorMassToTotalAnimalMass;
	inline Distance maxDepthOfSmallCove;
	inline int maxSmallCoveCuboids;
	inline int minSmallCoveCuboids;
	inline Distance shoreSandMax;
	inline Distance shoreSandMin;
	inline int terrainFuzzIntensity;
	inline int maxChaosAttractorCountArea;
	inline int maxChaosAttractorDeltaArea;
	inline int64_t riverDefaultFlowRate;
	inline Distance maxRangeOfRiverToOcean;
	inline int riverCanyonCost;
	inline Temperature idealTemperatureForPlants;
	inline float fractionOfFoliageMassToLosePerPointTemperatureFromIdeal;
	inline int maxFoliageMassForArea;
	inline Distance minDistanceBetweenWorldAttractors;
	inline float areaToCoverBroadPhaseWorldGen;
	inline Distance maxSizeOfBroadPhaseCuboid;
	inline Percent minimumHumidityForTrees;
	inline int minimumTreesPerPercentHumidity;
	inline int maximumTreesPerPercentHumidity;
	inline Temperature treesMaxDeltaFromIdealTemperature;
	inline float seaLevelAttractorWeight;
	inline float ratioOfAnimalMassFlying;
	inline int depthOfTransitZone;
	inline MoveCost baseMoveCost;
	inline float fractionToIncreaseCostByPerZLevelUp;
	inline float fractionToDecreaseCostByPerZLevelDown;
	inline Percent humidityPercentPerFluidBlock;
	void load();
}