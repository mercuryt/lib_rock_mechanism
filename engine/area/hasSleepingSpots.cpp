#include "hasSleepingSpots.h"
#include "../deserializationMemo.h"
#include "../designations.h"
#include "../simulation.h"
#include "../blocks/blocks.h"
#include "../area.h"
void AreaHasSleepingSpots::load(const Json& data, DeserializationMemo&)
{
	for(const Json& block : data["unassigned"])
		m_unassigned.add(block.get<BlockIndex>());
}
Json AreaHasSleepingSpots::toJson() const
{
	Json data{{"unassigned", Json::array()}};
	for(BlockIndex block : m_unassigned)
		data["unassigned"].push_back(block);
	return data;
}
void AreaHasSleepingSpots::designate(FactionId faction, BlockIndex block)
{
	m_unassigned.add(block);
	m_area.getBlocks().designation_set(block, faction, BlockDesignation::Sleep);
}
void AreaHasSleepingSpots::undesignate(FactionId faction, BlockIndex block)
{
	m_unassigned.remove(block);
	m_area.getBlocks().designation_unset(block, faction, BlockDesignation::Sleep);
}
