#pragma once

#include "../types.h"
#include "../random.h"
#include "locii.h"
#include "worldLocation.h"
#include "lake.h"
#include "river.h"
#include "lociiConfig.h"
#include "biome.h"

#include <cstddef>
#include <string>
#include <vector>
class Simulation;
class Area;
class DeserializationMemo;
struct WorldConfig final
{
	LociiConfig elevation;
	std::unordered_map<const Biome*, LociiConfig> biomeConfigs;
	size_t equatorSize;
	uint32_t areaSizeX;
	uint32_t areaSizeY;
	uint32_t areaSizeZ;
	size_t averageLandHeightBlocks;
};
class World final
{
	Simulation& m_simulation;
	WorldConfig m_config;
	// The number of locations in a band around the equator.
	const size_t m_equatorSize;
	// The number of locations in a band around the pole.
	const size_t m_poleSizeHorizontal;
	// The number of locations in a line between the poles.
	const size_t m_poleDistance;
	// TODO: calculate this.
	Kilometers m_kilometersPerDegreeLongitude = 111;
	std::vector<Locii> m_elevationLocii;
	std::unordered_map<const Biome*, std::vector<Locii>> m_biomeLocii;
	std::vector<River> m_rivers;
	std::vector<Lake> m_lakes;
	std::vector<WorldLocation> m_locations;
	
	void setElevationForLocation(WorldLocation& location);
	void setAdjacentFor(WorldLocation& location, size_t horizontal, size_t horizontalSize, size_t previousHorizontalSize, size_t nextHorizontalSize, WorldLocation* previousHorizontalStart, WorldLocation* nextHorizontalStart);
	size_t horizontalSizeAtVerticalBand(size_t vertical);
	void generateRivers();
	void generateBiomeLocii();
	void assignBiomes();
	Area& makeArea(WorldLocation& location);
	void assignAreaHeights(WorldLocation& location, Area& area);
	void assignAreaBiome(WorldLocation& location, Area& area);
	void makeAreaRiversAndLakes(WorldLocation& location, Area& area);
public:
	Random m_random;
	World(WorldConfig config, Simulation& simulation);
	World(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void setLake(WorldLocation& location);
	LatLng getRandomCoordinates();
	LatLng getRandomCoordinatesInRangeOf(LatLng location, Kilometers range);
	Area& getAreaForLocation(WorldLocation& location);
	WorldLocation& getLocationByNormalizedLatLng(LatLng location);
};
