#pragma once
#include "../util.h"
#include "../config.h"
#include "worldLocation.h"
#include <string>
#include <sys/types.h>
#include <vector>
#include <algorithm>
class World;
class River
{
public:
	std::wstring m_name;
	std::vector<WorldLocation*> m_locations;
	uint8_t m_rate;
	River(std::wstring n, uint8_t rate, WorldLocation& headWater, World& world);
	River(const Json& data, DeserializationMemo& deserializationMemo, World& world);
	void setLocations(std::vector<WorldLocation*> locations);
	static Meters valleyDepth(Meters inital)
	{
		// Reduce heigh by either flat quantity or percentage, whichever is less.
		return std::max(
				util::scaleByPercent(inital, Config::percentHeightCarvedByRivers),
				inital - Config::metersHeightCarvedByRivers
		);
	}
	WorldLocation* getUpStream(const WorldLocation& location)
	{
		auto iterator = std::ranges::find(m_locations, &location);
		assert(iterator != m_locations.end());
		if(iterator == m_locations.begin())
			return nullptr;
		return *(--iterator);
	}
	WorldLocation* getDownStream(const WorldLocation& location)
	{
		auto iterator = std::ranges::find(m_locations, &location);
		assert(iterator != m_locations.end());
		if(iterator + 1 == m_locations.end())
			return nullptr;
		return *(++iterator);
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(River, m_name, m_locations);
};
