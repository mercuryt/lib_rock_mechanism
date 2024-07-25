#include "world.h"
#include "../config.h"
#include "../util.h"
#include "../deserializationMemo.h"
#include "../area.h"
#include "../areaBuilderUtil.h"
#include "biome.h"
#include "worldLocation.h"
#include <bits/fs_fwd.h>
#include <cstddef>
#include <list>
#include <unordered_set>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WorldConfig, elevation, biomeConfigs, equatorSize, areaSizeX, areaSizeY, averageLandHeightBlocks);
World::World(WorldConfig config, Simulation& simulation) : m_simulation(simulation), m_config(config), m_equatorSize(config.equatorSize), m_poleSizeHorizontal(m_equatorSize / 2), m_poleDistance(((float)m_equatorSize * 3.f) / 4)
{
	// This is reserving extra, as if the map were a rectangle.
	m_locations.reserve(m_equatorSize * m_poleDistance);
	for(size_t vertical = 0; vertical < m_poleDistance; ++vertical)
	{
		size_t verticalDistanceToEquator = abs((int)(m_poleDistance / 2) - (int)vertical);
		size_t horizontalSize = horizontalSizeAtVerticalBand(vertical);
		for(size_t horizontal= 0; horizontal < horizontalSize; ++horizontal)
		{
			Latitude latitude = util::scaleByFraction(90, verticalDistanceToEquator, m_poleDistance / 2);
			if(verticalDistanceToEquator > m_poleDistance / 2)
				latitude *= -1;
			size_t horizontalDistanceToPrimeMeridian = abs((int)(horizontalSize / 2) - (int)horizontal);
			Longitude longitude = util::scaleByFraction(90, horizontalDistanceToPrimeMeridian, horizontalSize / 2);
			if( horizontalSize > horizontalSize / 2)
				longitude *= -1;
			m_locations.emplace_back(latitude, longitude);
			WorldLocation& location = m_locations.back();
			setElevationForLocation(location);
			size_t previousHorizontalSize;
		       	size_t nextHorizontalSize;
			WorldLocation* previousHorizontalStart;
		       	WorldLocation* nextHorizontalStart;
			if(vertical)
			{
				previousHorizontalSize = horizontalSizeAtVerticalBand(vertical - 1);
				previousHorizontalStart = &location - horizontal - previousHorizontalSize - 1;
			}
			else 
			{
				previousHorizontalSize = 0; 
				previousHorizontalStart = nullptr;
			}
			if(vertical != m_poleDistance - 1)
			{
				nextHorizontalSize = horizontalSizeAtVerticalBand(vertical + 1);
				nextHorizontalStart = &location - horizontal + horizontalSize + 1;
			}
			else 
			{
				nextHorizontalSize = 0; 
				nextHorizontalStart = nullptr;
			}
			setAdjacentFor(location, horizontal, horizontalSize, previousHorizontalSize, nextHorizontalSize, previousHorizontalStart, nextHorizontalStart);
		}
	}
	generateRivers();
	generateBiomeLocii();
	assignBiomes();
}
World::World(const Json& data, DeserializationMemo& deserializationMemo) :
       	m_simulation(deserializationMemo.m_simulation), m_config(data["config"]), m_equatorSize(data["equatorSize"].get<size_t>()), 
	m_poleSizeHorizontal(m_equatorSize / 2), m_poleDistance(((float)m_equatorSize * 3.f) / 4), m_kilometersPerDegreeLongitude(data["kilometersPerDegreeLongitude"])
{
	for(const Json& locationData : data["locations"])
		m_locations.emplace_back(locationData);
	for(const Json& elevationLociiData : data["elevationLocii"])
		m_elevationLocii.emplace_back(elevationLociiData, deserializationMemo);
	for(const Json& biomeLociiData : data["biomeLocii"])
		m_elevationLocii.emplace_back(biomeLociiData, deserializationMemo);
	for(const Json& riversData : data["rivers"])
		m_rivers.emplace_back(riversData, deserializationMemo, *this);
	for(const Json& lakeData : data["lakes"])
		m_lakes.emplace_back(lakeData, deserializationMemo);
}
Json World::toJson() const
{
	Json data{{"config", m_config}, {"equatorSize", m_equatorSize}, {"kilometersPerDegreeLongitude", m_kilometersPerDegreeLongitude},
		{"locations", Json::array()}, {"elevationLocii", Json::array()}, {"biomeLocii", Json::array()}, {"rivers", Json::array()}, {"lakes", Json::array()}};
	for(const Locii& locii : m_elevationLocii)
		data["elevationLocii"].push_back(locii.toJson());
	return data;
}
size_t World::horizontalSizeAtVerticalBand(size_t vertical)
{
	size_t verticalDistanceToEquator = abs((int)(m_poleDistance / 2) - (int)vertical);
	return  util::scaleByFraction(m_equatorSize, verticalDistanceToEquator, (m_poleDistance / 2));
}
void World::setElevationForLocation(WorldLocation& location)
{
	for(auto locii : m_elevationLocii)
	{
		auto distance = locii.distanceTo(location.latitude, location.longitude);
		if(distance > locii.maxRange)
			continue;
		location.landHeightAboveSeaLevel += locii.intensityAtDistance(distance) * Config::metersPerUnitElevationLociiIntensity;
	}
}
void World::setAdjacentFor(WorldLocation& location, size_t horizontal, size_t horizontalSize, size_t previousHorizontalSize, size_t nextHorizontalSize, WorldLocation* previousHorizontalStart, WorldLocation* nextHorizontalStart)
{
	// North
	if(location.latitude != -90)
	{
		float blocksOnThisLevelPerBlockOnPreviousLevel = (float)horizontalSize / (float)previousHorizontalSize;
		float startFloat = horizontal * blocksOnThisLevelPerBlockOnPreviousLevel;
		size_t end = startFloat + blocksOnThisLevelPerBlockOnPreviousLevel;
		size_t start = startFloat;
		for(size_t i = start; i < end; ++i)
		{
			location.adjacent.push_back(previousHorizontalStart + i);
			location.north.push_back(location.adjacent.back());
		}
	}
	// West
	if(location.longitude == -90)
		location.adjacent.push_back(&location + horizontalSize);
	else 
		location.adjacent.push_back(&location + 1);
	location.west = location.adjacent.back();
	// East
	if(location.longitude == 90)
		location.adjacent.push_back(&location - horizontalSize);
	else 
		location.adjacent.push_back(&location - 1);
	location.east = location.adjacent.back();
	// South
	if(location.latitude != 90)
	{
		float blocksOnThisLevelPerBlockOnNextLevel = (float)horizontalSize / (float)nextHorizontalSize;
		float startFloat = horizontal * blocksOnThisLevelPerBlockOnNextLevel;
		size_t end = startFloat + blocksOnThisLevelPerBlockOnNextLevel;
		size_t start = startFloat;
		for(size_t i = start; i < end; ++i)
		{
			location.adjacent.push_back(nextHorizontalStart + i);
			location.south.push_back(location.adjacent.back());
		}
	}
}
void World::generateRivers()
{
	std::list<std::unordered_set<WorldLocation*>> sets;
	std::unordered_map<WorldLocation*, std::unordered_set<WorldLocation*>*> setsByLocation;
	// Find candidates and consolidate into sets.
	for(WorldLocation& location : m_locations)
		if(location.landHeightAboveSeaLevel >= Config::minimumAltitudeForHeadwaterFormation)
		{
			assert(!setsByLocation.contains(&location));
			std::unordered_set<std::unordered_set<WorldLocation*>*> adjacentSets;
			for(WorldLocation* adjacent : location.adjacent)
				if(setsByLocation.contains(adjacent))
					adjacentSets.insert(setsByLocation.at(adjacent));
			if(setsByLocation.empty())
			{
				sets.push_back({});
				setsByLocation[&location] = &sets.back();
			}
			else
			{
				auto* set = *adjacentSets.begin();
				set->insert(&location);
				setsByLocation[&location] = set;
				for(auto* otherSet : adjacentSets)
				{
					if(otherSet == set)
						continue;
					for(WorldLocation* otherLocation : *otherSet)
						setsByLocation[otherLocation] = set;
					set->insert(otherSet->begin(), otherSet->end());
					sets.remove(*otherSet);
				}

			}

		}
	
	for(auto& set : sets)
	{
		// Reduce candidates.
		uint32_t maxNumber = set.size() * Config::averageNumberOfRiversPerCandidate;
		uint32_t numberOfRivers = m_random.getInRange(1u, maxNumber);
		std::vector<WorldLocation*> vector(set.begin(), set.end());
		// TODO: prevent generating adjacent.
		auto headwaterLocations = m_random.getMultipleInVector(vector, numberOfRivers);
		// Path.
		for(WorldLocation* location : headwaterLocations)
		{
			std::wstring name = L"river";
			// TODO: calculate or generate rate somehow.
			uint8_t rate = 3;
			m_rivers.emplace_back(name, rate, *location, *this);
		}
		// Make vallies.
		for(River& river : m_rivers)
			for(WorldLocation* location : river.m_locations)
				location->landHeightAboveSeaLevel = River::valleyDepth(location->landHeightAboveSeaLevel);
	}
}
void World::setLake(WorldLocation& location)
{
	assert(!location.lake);
	for(WorldLocation* adjacent : location.adjacent)
		if(adjacent->lake)
		{
			adjacent->lake->locations.push_back(&location);
			location.lake = adjacent->lake;
		}
		else
		{
			uint8_t size = 3;
			m_lakes.emplace_back(L"lake", size);
			m_lakes.back().locations.push_back(&location);
		}
}
void World::generateBiomeLocii()
{
	for(auto& [biomeType, lociiConfig] : m_config.biomeConfigs)
	{
		uint lociiCount = lociiConfig.numberOfLocationsPerLocii * m_locations.size();
		for(uint i = 0; i < lociiCount; ++i)
			m_biomeLocii[biomeType].push_back(Locii::make(lociiConfig, *this));
	}
}
void World::assignBiomes()
{
	for(WorldLocation& location : m_locations)
	{
		// collect influence scaled by distance.
		for(auto& [biomeType, lociiList] : m_biomeLocii)
		{
			for(auto locii : lociiList)
			{
				auto distance = locii.distanceTo(location.latitude, location.longitude);
				if(distance > locii.maxRange)
					continue;
				location.biomeInfluence[biomeType] += locii.intensityAtDistance(distance);
			}
		}
		// find primary biome
		float highest = 0;
		const Biome* primary = nullptr;
		for(auto& [biome, influence] : location.biomeInfluence)
			if(influence > highest)
			{
				highest = influence;
				primary = biome;
			}
		assert(primary);
		location.primaryBiome = primary;
	}
}
Area& World::getAreaForLocation(WorldLocation& location)
{
	// TODO: unhibernate area.
	if(!location.area)
		location.area = makeArea(location).m_id;
	return m_simulation.getAreaById(location.area);
}
LatLng World::getRandomCoordinates()
{
	return {
		m_random.getInRange(-90.0, 90.0),
		m_random.getInRange(-90.0, 90.0),
	};
}
LatLng World::getRandomCoordinatesInRangeOf(LatLng location, Kilometers range)
{
	double maxDegreesOffset = (double)range / (double)m_kilometersPerDegreeLongitude;
	LatLng candidate {
		m_random.getInRange(location.latitude - maxDegreesOffset, location.latitude + maxDegreesOffset),
		m_random.getInRange(location.longitude - maxDegreesOffset, location.longitude + maxDegreesOffset)
	};
	while(candidate.distanceTo(location) > range)
		candidate =  {
			m_random.getInRange(location.latitude - maxDegreesOffset, location.latitude + maxDegreesOffset),
			m_random.getInRange(location.longitude - maxDegreesOffset, location.longitude + maxDegreesOffset)
		};
	return candidate;
}

WorldLocation& World::getLocationByNormalizedLatLng(LatLng location)
{
	auto iter =  std::ranges::find_if(m_locations, [&](const WorldLocation& wl){ return location.latitude == wl.latitude && location.longitude == wl.longitude; });
	assert(iter != m_locations.end());
	return *iter;
}
