#include "river.h"
#include "deserializationMemo.h"
#include "world.h"
#include "worldLocation.h"
#include <list>
#include <string>
#include <unordered_set>

struct WorldLocation;
struct RouteNode
{
	WorldLocation* location;
	RouteNode* previous;
};

River::River(std::wstring n, uint8_t rate, WorldLocation& headWater, World& world) : m_name(n), m_rate(rate)
{
	std::list<RouteNode> nodes;
	std::unordered_set<WorldLocation*> closed;
	nodes.emplace_back(&headWater, nullptr);
	std::list<RouteNode*> open{&nodes.back()};
	closed.insert(&headWater);
	while(true)
	{
		RouteNode* node = open.front();
		open.pop_front();
		for(auto* adjacent : node->location->adjacent)
		{
			// water can't flow uphill.
			if(adjacent->landHeightAboveSeaLevel > node->location->landHeightAboveSeaLevel)
				continue;
			if(adjacent->rivers.size() || adjacent->landHeightAboveSeaLevel < 0)
			{
				// done.
				std::vector<WorldLocation*> locations{node->location};
				while(node->previous)
				{
					node = node->previous;
					locations.push_back(node->location);
				}
				std::reverse(locations.begin(), locations.end());
				setLocations(locations);
			}
			if(closed.contains(adjacent))
				continue;
			closed.insert(adjacent);
			nodes.emplace_back(adjacent, node);
			open.push_back(&nodes.back());
		}
		if(open.empty())
		{
			// No route found, make lake.
			world.setLake(*node->location);
		}
	}
	
}
River::River(const Json& data, DeserializationMemo& deserializationMemo, World& world) : River(
		data["name"].get<std::wstring>(),
		data["rate"].get<uint8_t>(),
		deserializationMemo.getLocationByNormalizedLatLng(data["headWater"]),
		world) {}
void River::setLocations(std::vector<WorldLocation*> locations)
{
	m_locations = locations;
	for(auto* location : locations)
		location->rivers.push_back(this);
}
