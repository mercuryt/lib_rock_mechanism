#include "hasDigDesignations.h"
#include "area.h"
#include "../blocks/blocks.h"
HasDigDesignationsForFaction::HasDigDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area) :
	m_faction(faction)
{
	for(const Json& pair : data["projects"])
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data.emplace(block, pair[1], deserializationMemo, area);
	}
}
void HasDigDesignationsForFaction::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data["projects"])
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data[block].loadWorkers(pair[1], deserializationMemo);
	}
}
Json HasDigDesignationsForFaction::toJson() const
{
	Json projects;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second->toJson()};
		projects.push_back(jsonPair);
	}
	return {{"projects", projects}};
}
void HasDigDesignationsForFaction::designate(Area& area, const BlockIndex& block, [[maybe_unused]] const BlockFeatureTypeId blockFeatureType)
{
	assert(!m_data.contains(block));
	Blocks& blocks = area.getBlocks();
	blocks.designation_set(block, m_faction, BlockDesignation::Dig);
	// To be called when block is no longer a suitable location, for example if it got dug out already.
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<DigLocationDishonorCallback>(m_faction, area, block);
	[[maybe_unused]] DigProject& project = m_data.emplace(block, m_faction, area, block, blockFeatureType, std::move(locationDishonorCallback));
	assert(static_cast<DigProject*>(blocks.project_get(block, m_faction)) == &project);
}
void HasDigDesignationsForFaction::undesignate(const BlockIndex& block)
{
	assert(m_data.contains(block));
	DigProject& project = m_data[block];
	project.cancel();
}
void HasDigDesignationsForFaction::remove(Area& area, const BlockIndex& block)
{
	assert(m_data.contains(block));
	area.getBlocks().designation_unset(block, m_faction, BlockDesignation::Dig);
	m_data.erase(block);
}
void HasDigDesignationsForFaction::removeIfExists(Area& area, const BlockIndex& block)
{
	if(m_data.contains(block))
		remove(area, block);
}
BlockFeatureTypeId HasDigDesignationsForFaction::getForBlock(const BlockIndex& block) const { return m_data[block].m_blockFeatureType; }
bool HasDigDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void AreaHasDigDesignations::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.emplace(faction, pair[1], deserializationMemo, faction, area);
	}
}
void AreaHasDigDesignations::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data[faction].loadWorkers(pair[1], deserializationMemo);
	}

}
Json AreaHasDigDesignations::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first;
		jsonPair[1] = pair.second->toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void AreaHasDigDesignations::addFaction(const FactionId& faction)
{
	assert(!m_data.contains(faction));
	m_data.emplace(faction, faction);
}
void AreaHasDigDesignations::removeFaction(const FactionId& faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
// If blockFeatureType is null then dig out fully rather then digging out a feature.
void AreaHasDigDesignations::designate(const FactionId& faction, const BlockIndex& block, const BlockFeatureTypeId blockFeatureType)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	m_data[faction].designate(m_area, block, blockFeatureType);
}
void AreaHasDigDesignations::undesignate(const FactionId& faction, const BlockIndex& block)
{
	assert(m_data.contains(faction));
	assert(m_data[faction].m_data.contains(block));
	m_data[faction].undesignate(block);
}
void AreaHasDigDesignations::remove(const FactionId& faction, const BlockIndex& block)
{
	assert(m_data.contains(faction));
	m_data[faction].remove(m_area, block);
}
void AreaHasDigDesignations::clearAll(const BlockIndex& block)
{
	for(auto& pair : m_data)
		pair.second->removeIfExists(m_area, block);
}
void AreaHasDigDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair : pair.second->m_data)
			pair.second->clearReservations();
}
bool AreaHasDigDesignations::areThereAnyForFaction(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return !m_data[faction].empty();
}
DigProject& AreaHasDigDesignations::getForFactionAndBlock(const FactionId& faction, const BlockIndex& block)
{
	assert(m_data.contains(faction));
	assert(m_data[faction].m_data.contains(block));
	return m_data[faction].m_data[block];
}