#include "river.h"
#include "world.h"
#include "worldLocation.h"
#include <list>
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
	std::list<RouteNode*> open;
	open.emplace_back(&headWater, nullptr);
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
			open.emplace_back(adjacent, &node);
		}
		if(open.empty())
		{
			// No route found, make lake.
			world.setLake(*node->location);
		}
	}
	
}
void River::setLocations(std::vector<WorldLocation*> locations)
{
	m_locations = locations;
	for(auto* location : locations)
		location->rivers.push_back(this);
}
