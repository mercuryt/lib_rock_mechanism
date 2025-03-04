#include "dig.h"
#include "designations.h"
#include "area/area.h"
#include "actors/actors.h"
#include "blockFeature.h"
#include "deserializationMemo.h"
#include "deserializeDishonorCallbacks.h"
#include "path/pathRequest.h"
#include "random.h"
#include "reference.h"
#include "reservable.h"
#include "skill.h"
#include "path/terrainFacade.h"
#include "types.h"
#include "util.h"
#include "simulation/simulation.h"
#include "itemType.h"
#include "objectives/dig.h"
#include "hasShapes.hpp"
#include <memory>
#include <sys/types.h>
/*
// Input.
void DesignateDigInputAction::execute()
{
	BlockIndex block = *m_cuboid.begin();
	auto& digDesginations = block.m_area->m_hasDigDesignations;
	FactionId faction = *(**m_actors.begin()).getFaction();
	for(BlockIndex block : m_cuboid)
		digDesginations.designate(faction, block, m_blockFeatureType);
};
void UndesignateDigInputAction::execute()
{
	BlockIndex block = *m_cuboid.begin();
	auto& digDesginations = block.m_area->m_hasDigDesignations;
	FactionId faction = *(**m_actors.begin()).getFaction();
	for(BlockIndex block : m_cuboid)
		digDesginations.undesignate(faction, block);
}
*/
DigProject::DigProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) : Project(data, deserializationMemo, area),
	m_blockFeatureType(data.contains("blockFeatureType") ? &BlockFeatureType::byName(data["blockFeatureType"].get<std::wstring>()) : nullptr) { }
Json DigProject::toJson() const
{
	Json data = Project::toJson();
	if(m_blockFeatureType)
		data["blockFeatureType"] = m_blockFeatureType->name;
	return data;
}
std::vector<std::pair<ItemQuery, Quantity>> DigProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, Quantity>> DigProject::getUnconsumed() const
{
	static ItemTypeId pick = ItemType::byName(L"pick");
	return {{ItemQuery::create(pick), Quantity::create(1)}};
}
std::vector<ActorReference> DigProject::getActors() const { return {}; }
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> DigProject::getByproducts() const
{
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> output;
	Random& random = m_area.m_simulation.m_random;
	auto& blocks = m_area.getBlocks();
	MaterialTypeId materialType = blocks.solid_is(m_location) ? blocks.solid_get(m_location) : blocks.blockFeature_getMaterialType(m_location);
	if(materialType.exists())
	{
		for(SpoilsDataTypeId spoilDataId : MaterialType::getSpoilData(materialType))
		{
			if(!random.percentChance(SpoilData::getChance(spoilDataId)))
				continue;
			//TODO: reduce yield for block features.
			Quantity quantity = Quantity::create(random.getInRange(SpoilData::getMin(spoilDataId).get(), SpoilData::getMax(spoilDataId).get()));
			output.emplace_back(SpoilData::getItemType(spoilDataId), SpoilData::getMaterialType(spoilDataId), quantity);
		}
	}
	return output;
}
// Static.
uint32_t DigProject::getWorkerDigScore(Area& area, ActorIndex actor)
{
	static SkillTypeId digType = SkillType::byName(L"dig");
	Actors& actors = area.getActors();
	uint32_t strength = actors.getStrength(actor).get();
	uint32_t skillLevel = actors.skill_getLevel(actor, digType).get();
	return (strength * Config::digStrengthModifier) + (skillLevel * Config::digSkillModifier);
}
void DigProject::onComplete()
{
	auto& blocks = m_area.getBlocks();
	if(!blocks.solid_is(m_location))
	{
		assert(!m_blockFeatureType);
		blocks.blockFeature_removeAll(m_location);
	}
	else
	{
		if(m_blockFeatureType == nullptr)
			blocks.solid_setNot(m_location);
		else
			blocks.blockFeature_hew(m_location, *m_blockFeatureType);
	}
	// Remove designations for other factions as well as owning faction.
	auto workers = std::move(m_workers);
	Actors& actors = m_area.getActors();
	m_area.m_hasDigDesignations.clearAll(m_location);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
void DigProject::onCancel()
{
	Actors& actors = m_area.getActors();
	Area& area = m_area;
	auto workers = getWorkersAndCandidates();
	m_area.m_hasDigDesignations.remove(m_faction, m_location);
	for(ActorIndex actor : workers)
	{
		auto& current = actors.objective_getCurrent<DigObjective&>(actor);
		current.m_project = nullptr;
		current.reset(area, actor);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
void DigProject::onDelay()
{
	m_area.m_blockDesignations.getForFaction(m_faction).maybeUnset(m_location, BlockDesignation::Dig);
}
void DigProject::offDelay()
{
	m_area.m_blockDesignations.getForFaction(m_faction).set(m_location, BlockDesignation::Dig);
}
// What would the total delay time be if we started from scratch now with current workers?
Step DigProject::getDuration() const
{
	uint32_t totalScore = 0u;
	Actors& actors = m_area.getActors();
	for(auto& pair : m_workers)
		totalScore += getWorkerDigScore(m_area, pair.first.getIndex(actors.m_referenceData));
	return std::max(Step::create(1), Config::digMaxSteps / totalScore);
}
DigLocationDishonorCallback::DigLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_faction(data["faction"].get<FactionId>()),
	m_area(deserializationMemo.area(data["area"])),
	m_location(data["location"].get<BlockIndex>()) { }
Json DigLocationDishonorCallback::toJson() const { return Json({{"type", "DigLocationDishonorCallback"}, {"faction", m_faction}, {"location", m_location}, {"area", m_area}}); }
void DigLocationDishonorCallback::execute([[maybe_unused]] const Quantity& oldCount, [[maybe_unused]] const Quantity& newCount)
{
	m_area.m_hasDigDesignations.undesignate(m_faction, m_location);
}
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
void HasDigDesignationsForFaction::designate(Area& area, const BlockIndex& block, [[maybe_unused]] const BlockFeatureType* blockFeatureType)
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
const BlockFeatureType* HasDigDesignationsForFaction::getForBlock(const BlockIndex& block) const { return m_data[block].m_blockFeatureType; }
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
void AreaHasDigDesignations::designate(const FactionId& faction, const BlockIndex& block, const BlockFeatureType* blockFeatureType)
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
