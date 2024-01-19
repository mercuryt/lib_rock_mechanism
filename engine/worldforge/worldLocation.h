#pragma once
#include "../types.h"
#include "../config.h"
#include "locii.h"
#include "biome.h"
#include <vector>
class River;
class Lake;
struct WorldLocation
{
	Latitude latitude;
	Longitude longitude;
	Meters landHeightAboveSeaLevel;
	Percent rainFall;
	AreaId area;
	std::vector<WorldLocation*> adjacent;
	WorldLocation* east;
	WorldLocation* west;
	std::vector<WorldLocation*> north;
	std::vector<WorldLocation*> south;
	std::vector<River*> rivers;
	Lake* lake;
	std::unordered_map<const Biome*, uint32_t> biomeInfluence;
	const Biome* primaryBiome;
	// No need to store adjacent, 
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(WorldLocation, latitude, longitude, landHeightAboveSeaLevel, rainFall, area);
};
inline void to_json(Json& data, WorldLocation* const& location){ data = std::make_pair(location->latitude, location->longitude); }
