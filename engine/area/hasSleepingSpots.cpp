#include "hasSleepingSpots.h"
#include "../deserializationMemo.h"
#include "../designations.h"
#include "../simulation/simulation.h"
#include "../blocks/blocks.h"
#include "../area/area.h"
void AreaHasSleepingSpots::load(const Json& data, DeserializationMemo&)
{
	for(const Json& block : data["unassigned"])
		m_unassigned.insert(block.get<BlockIndex>());
}
Json AreaHasSleepingSpots::toJson() const
{
	Json data{{"unassigned", Json::array()}};
	for(const BlockIndex& block : m_unassigned)
		data["unassigned"].push_back(block);
	return data;
}
void AreaHasSleepingSpots::designate(const FactionId& faction, const BlockIndex& block)
{
	m_unassigned.insert(block);
	m_area.getBlocks().designation_set(block, faction, BlockDesignation::Sleep);
}
void AreaHasSleepingSpots::undesignate(const FactionId& faction, const BlockIndex& block)
{
	m_unassigned.erase(block);
	m_area.getBlocks().designation_unset(block, faction, BlockDesignation::Sleep);
}
