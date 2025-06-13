#include "hasConstructionDesignations.h"
#include "hasBlockDesignations.h"
#include "area.h"
#include "../blocks/blocks.h"

HasConstructionDesignationsForFaction::HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area) :
	m_faction(faction)
{
	for(const Json& pair : data["projects"])
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data.emplace(block, pair[1], deserializationMemo, area);
	}
}
void HasConstructionDesignationsForFaction::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data["projects"])
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data[block].loadWorkers(pair[1], deserializationMemo);
	}
}
Json HasConstructionDesignationsForFaction::toJson() const
{
	Json projects;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second->toJson()};
		projects.push_back(jsonPair);
	}
	return {{"projects", projects}};
}
// If blockFeatureType is null then construct a wall rather then a feature.
void HasConstructionDesignationsForFaction::designate(Area& area, const BlockIndex& block, const BlockFeatureTypeId blockFeatureType, const MaterialTypeId& materialType)
{
	assert(!contains(block));
	Blocks& blocks = area.getBlocks();
	blocks.designation_set(block, m_faction, BlockDesignation::Construct);
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<ConstructionLocationDishonorCallback>(m_faction, area, block);
	m_data.emplace(block, m_faction, area, block, blockFeatureType, materialType, std::move(locationDishonorCallback));
}
void HasConstructionDesignationsForFaction::undesignate(const BlockIndex& block)
{
	assert(m_data.contains(block));
	ConstructProject& project = m_data[block];
	project.cancel();
}
void HasConstructionDesignationsForFaction::remove(Area& area, const BlockIndex& block)
{
	assert(contains(block));
	area.getBlocks().designation_unset(block, m_faction, BlockDesignation::Construct);
	m_data.erase(block);
}
void AreaHasConstructionDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second->m_data)
			pair2.second->clearReservations();
}
void HasConstructionDesignationsForFaction::removeIfExists(Area& area, const BlockIndex& block)
{
	if(m_data.contains(block))
		remove(area, block);
}
bool HasConstructionDesignationsForFaction::contains(const BlockIndex& block) const { return m_data.contains(block); }
BlockFeatureTypeId HasConstructionDesignationsForFaction::getForBlock(const BlockIndex& block) const { return m_data[block].m_blockFeatureType; }
bool HasConstructionDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void AreaHasConstructionDesignations::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.emplace(faction, pair[1], deserializationMemo, faction, area);
	}
}
void AreaHasConstructionDesignations::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data[faction].loadWorkers(pair[1], deserializationMemo);
	}

}
Json AreaHasConstructionDesignations::toJson() const
{
	Json data = Json::array();
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first;
		jsonPair[1] = pair.second->toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void AreaHasConstructionDesignations::addFaction(const FactionId& faction)
{
	assert(!m_data.contains(faction));
	m_data.emplace(faction, faction);
}
void AreaHasConstructionDesignations::removeFaction(const FactionId& faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
void AreaHasConstructionDesignations::designate(const FactionId& faction, const BlockIndex& block, const BlockFeatureTypeId blockFeatureType, const MaterialTypeId& materialType)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	m_data[faction].designate(m_area, block, blockFeatureType, materialType);
}
void AreaHasConstructionDesignations::undesignate(const FactionId& faction, const BlockIndex& block)
{
	m_data[faction].undesignate(block);
}
void AreaHasConstructionDesignations::remove(const FactionId& faction, const BlockIndex& block)
{
	assert(m_data.contains(faction));
	m_data[faction].remove(m_area, block);
}
void AreaHasConstructionDesignations::clearAll(const BlockIndex& block)
{
	for(auto& pair : m_data)
		pair.second->removeIfExists(m_area, block);
}
bool AreaHasConstructionDesignations::areThereAnyForFaction(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return !m_data[faction].empty();
}
bool AreaHasConstructionDesignations::contains(const FactionId& faction, const BlockIndex& block) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].contains(block);
}
ConstructProject& AreaHasConstructionDesignations::getProject(const FactionId& faction, const BlockIndex& block) { return m_data[faction].m_data[block]; }