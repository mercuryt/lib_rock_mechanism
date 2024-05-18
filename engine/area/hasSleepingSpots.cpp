#include "hasSleepingSpots.h"
#include "../deserializationMemo.h"
#include "../designations.h"
#include "../simulation.h"
#include "../block.h"
void AreaHasSleepingSpots::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& block : data["unassigned"])
		m_unassigned.insert(&deserializationMemo.m_simulation.getBlockForJsonQuery(block));
}
Json AreaHasSleepingSpots::toJson() const
{
	Json data{{"unassigned", Json::array()}};
	for(Block* block : m_unassigned)
		data["unassigned"].push_back(block);
	return data;
}
void AreaHasSleepingSpots::designate(Faction& faction, Block& block)
{
	m_unassigned.insert(&block);
	block.setDesignation(faction, BlockDesignation::Sleep);
}
void AreaHasSleepingSpots::undesignate(Faction& faction, Block& block)
{
	m_unassigned.erase(&block);
	block.unsetDesignation(faction, BlockDesignation::Sleep);
}
