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
	m_blockFeatureType(data.contains("blockFeatureType") ? &BlockFeatureType::byName(data["blockFeatureType"].get<std::string>()) : nullptr) { }
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
	static ItemTypeId pick = ItemType::byName("pick");
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
	static SkillTypeId digType = SkillType::byName("dig");
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