#include "woodcutting.h"
#include "area.h"
#include "blockFeature.h"
#include "deserializationMemo.h"
#include "random.h"
#include "reservable.h"
#include "terrainFacade.h"
#include "types.h"
#include "util.h"
#include "simulation.h"
#include "itemType.h"
#include <memory>
#include <sys/types.h>
/*
// Input.
void DesignateWoodCuttingInputAction::execute()
{
	BlockIndex block = *m_blocks.begin();
	auto& woodCuttingDesginations = m_area->m_hasWoodCuttingDesignations;
	Faction& faction = *(**m_actors.begin()).getFaction();
	for(BlockIndex block : m_blocks)
		woodCuttingDesginations.designate(faction, block);
};
void UndesignateWoodCuttingInputAction::execute()
{
	BlockIndex block = *m_blocks.begin();
	auto& woodCuttingDesginations = block.m_area->m_hasWoodCuttingDesignations;
	Faction& faction = *(**m_actors.begin()).getFaction();
	for(BlockIndex block : m_blocks)
		woodCuttingDesginations.undesignate(faction, block);
}
*/
WoodCuttingPathRequest::WoodCuttingPathRequest(Area& area, WoodCuttingObjective& woodCuttingObjective) :
	m_woodCuttingObjective(woodCuttingObjective)
{
	std::function<bool(BlockIndex)> predicate = [this, &area](BlockIndex block)
	{
		return m_woodCuttingObjective.getJoinableProjectAt(area, block) != nullptr;
	};
	DistanceInBlocks maxRange = Config::maxRangeToSearchForWoodCuttingDesignations;
	bool unreserved = true;
	createGoAdjacentToCondition(area, m_woodCuttingObjective.m_actor, predicate, m_woodCuttingObjective.m_detour, unreserved, maxRange, BLOCK_INDEX_MAX);
}
void WoodCuttingPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(m_woodCuttingObjective.m_actor, m_woodCuttingObjective);
	else
	{
		if(result.useCurrentPosition)
		{
			if(!actors.move_tryToReserveOccupied(m_woodCuttingObjective.m_actor))
			{
				actors.objective_canNotCompleteSubobjective(m_woodCuttingObjective.m_actor);
				return;
			}
		}
		else if(!actors.move_tryToReserveProposedDestination(m_woodCuttingObjective.m_actor, result.path))
		{
			actors.objective_canNotCompleteSubobjective(m_woodCuttingObjective.m_actor);
			return;
		}
		BlockIndex target = result.blockThatPassedPredicate;
		WoodCuttingProject& project = area.m_hasWoodCuttingDesignations.at(*actors.getFaction(m_woodCuttingObjective.m_actor), target);
		if(project.canAddWorker(m_woodCuttingObjective.m_actor))
			m_woodCuttingObjective.joinProject(project);
		else
		{
			// Project can no longer accept this worker, try again.
			actors.objective_canNotCompleteSubobjective(m_woodCuttingObjective.m_actor);
			return;
		}
	}
}
WoodCuttingObjective::WoodCuttingObjective(ActorIndex a) :
	Objective(a, Config::woodCuttingObjectivePriority) { }
/*
WoodCuttingObjective::WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_woodCuttingPathRequest(m_actor.m_area->m_simulation.m_threadedTaskEngine), 
	m_project(data.contains("project") ? static_cast<WoodCuttingProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr)
{
	if(data.contains("threadedTask"))
		m_woodCuttingPathRequest.create(*this);
}
Json WoodCuttingObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project;
	if(m_woodCuttingPathRequest.exists())
		data["threadedTask"] = true;
	return data;
}
*/
void WoodCuttingObjective::execute(Area& area)
{
	if(m_project != nullptr)
		m_project->commandWorker(m_actor);
	else
	{
		Actors& actors = area.getActors();
		WoodCuttingProject* project = nullptr;
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) 
		{ 
			if(!getJoinableProjectAt(area, block))
				return false;
			project = &area.m_hasWoodCuttingDesignations.at(*actors.getFaction(m_actor), block);
			if(project->canAddWorker(m_actor))
				return true;
			return false;
		};
		[[maybe_unused]] BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(m_actor, predicate);
		if(project != nullptr)
		{
			assert(adjacent != BLOCK_INDEX_MAX);
			joinProject(*project);
			return;
		}
		actors.move_pathRequestRecord(m_actor, std::make_unique<WoodCuttingPathRequest>(area, *this));
	}
}
void WoodCuttingObjective::cancel(Area& area)
{
	if(m_project != nullptr)
		m_project->removeWorker(m_actor);
	area.getActors().move_pathRequestMaybeCancel(m_actor);
}
void WoodCuttingObjective::delay(Area& area)
{
	cancel(area);
	m_project = nullptr;
	area.getActors().project_unset(m_actor);
}
void WoodCuttingObjective::reset(Area& area) 
{ 
	Actors& actors = area.getActors();
	if(m_project)
	{
		assert(!m_project->getWorkers().contains(m_actor));
		m_project = nullptr; 
		area.getActors().project_unset(m_actor);
	}
	else 
		assert(!actors.project_exists(m_actor));
	actors.move_pathRequestMaybeCancel(m_actor);
	actors.canReserve_clearAll(m_actor);
}
void WoodCuttingObjective::onProjectCannotReserve(Area&)
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void WoodCuttingObjective::joinProject(WoodCuttingProject& project)
{
	assert(m_project == nullptr);
	m_project = &project;
	project.addWorkerCandidate(m_actor, *this);
}
WoodCuttingProject* WoodCuttingObjective::getJoinableProjectAt(Area& area, BlockIndex block)
{
	Actors& actors = area.getActors();
	Faction& faction = *actors.getFaction(m_actor);
	if(!area.getBlocks().designation_has(block, faction, BlockDesignation::WoodCutting))
		return nullptr;
	WoodCuttingProject& output = area.m_hasWoodCuttingDesignations.at(faction, block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return nullptr;
	if(!output.canAddWorker(m_actor))
		return nullptr;
	return &output;
}
bool WoodCuttingObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	//TODO: check for any axes?
	return area.m_hasWoodCuttingDesignations.areThereAnyForFaction(*area.getActors().getFaction(actor));
}
std::unique_ptr<Objective> WoodCuttingObjectiveType::makeFor(Area&, ActorIndex actor) const
{
	std::unique_ptr<Objective> objective = std::make_unique<WoodCuttingObjective>(actor);
	return objective;
}
WoodCuttingProject::WoodCuttingProject(const Json& data, DeserializationMemo& deserializationMemo) : Project(data, deserializationMemo) { }
std::vector<std::pair<ItemQuery, uint32_t>> WoodCuttingProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, uint32_t>> WoodCuttingProject::getUnconsumed() const { return {{ItemType::byName("axe"), 1}}; }
std::vector<std::pair<ActorQuery, uint32_t>> WoodCuttingProject::getActors() const { return {}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> WoodCuttingProject::getByproducts() const
{
	PlantIndex plant = m_area.getBlocks().plant_get(m_location);
	Plants& plants = m_area.getPlants();
	Percent percentGrown = plants.getPercentGrown(plant);
	const PlantSpecies& species = plants.getSpecies(plant);
	uint32_t unitsLogsGenerated = util::scaleByPercent(species.logsGeneratedByFellingWhenFullGrown, percentGrown);
	uint32_t unitsBranchesGenerated = util::scaleByPercent(species.branchesGeneratedByFellingWhenFullGrown, percentGrown);
	assert(unitsLogsGenerated);
	assert(unitsBranchesGenerated);
	auto& woodType = species.woodType;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> output{
		
		{&ItemType::byName("branch"), woodType, unitsBranchesGenerated},
		{&ItemType::byName("log"), woodType, unitsLogsGenerated}
	};
	return output;
}
// Static.
uint32_t WoodCuttingProject::getWorkerWoodCuttingScore(Area& area, ActorIndex actor)
{
	static const SkillType& woodCuttingType = SkillType::byName("wood cutting");
	Actors& actors = area.getActors();
	uint32_t strength = actors.getStrength(actor);
	uint32_t skill = actors.skill_getLevel(actor, woodCuttingType);
	return (strength * Config::woodCuttingStrengthModifier) + (skill * Config::woodCuttingSkillModifier);
}
void WoodCuttingProject::onComplete()
{
	if(m_area.getBlocks().plant_exists(m_location))
	{
		PlantIndex plant = m_area.getBlocks().plant_get(m_location);
		m_area.getPlants().die(plant);
	}
	// Remove designations for other factions as well as owning faction.
	auto workers = std::move(m_workers);
	m_area.m_hasWoodCuttingDesignations.clearAll(m_location);
	Actors& actors = m_area.getActors();
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor, projectWorker.objective);
}
void WoodCuttingProject::onCancel()
{
	m_area.m_hasWoodCuttingDesignations.remove(m_faction, m_location);
	Actors& actors = m_area.getActors();
	for(ActorIndex actor : getWorkersAndCandidates())
	{
		// TODO: These two lines seem redundant with the third.
		actors.project_unset(actor);
		actors.objective_getCurrent<WoodCuttingObjective>(actor).reset(m_area);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
void WoodCuttingProject::onDelay()
{
	m_area.getBlocks().designation_maybeUnset(m_location, m_faction, BlockDesignation::WoodCutting);
}
void WoodCuttingProject::offDelay()
{
	m_area.getBlocks().designation_set(m_location, m_faction, BlockDesignation::WoodCutting);
}
// What would the total delay time be if we started from scratch now with current workers?
Step WoodCuttingProject::getDuration() const
{
	uint32_t totalScore = 0u;
	for(auto& pair : m_workers)
		totalScore += getWorkerWoodCuttingScore(m_area, pair.first);
	return std::max(Step(1u), Config::woodCuttingMaxSteps / totalScore);
}
WoodCuttingLocationDishonorCallback::WoodCuttingLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) : 
	m_faction(deserializationMemo.faction(data["faction"].get<std::wstring>())),
	m_area(deserializationMemo.area(data["area"])),
	m_location(data["location"].get<BlockIndex>()) { }
Json WoodCuttingLocationDishonorCallback::toJson() const { return Json({{"type", "WoodCuttingLocationDishonorCallback"}, {"faction", m_faction.name}, {"location", m_location}}); }
void WoodCuttingLocationDishonorCallback::execute([[maybe_unused]] uint32_t oldCount, [[maybe_unused]] uint32_t newCount)
{
	m_area.m_hasWoodCuttingDesignations.undesignate(m_faction, m_location);
}
HasWoodCuttingDesignationsForFaction::HasWoodCuttingDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Faction& faction) : 
	m_area(deserializationMemo.area(data["area"])),
	m_faction(faction)
{
	for(const Json& pair : data)
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data.try_emplace(block, pair[1], deserializationMemo);
	}
}
Json HasWoodCuttingDesignationsForFaction::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second.toJson()};
		data.push_back(jsonPair);
	}
	return data;
}
void HasWoodCuttingDesignationsForFaction::designate(BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	Plants& plants = m_area.getPlants();
	assert(!m_data.contains(block));
	assert(blocks.plant_exists(block));
	assert(plants.getSpecies(blocks.plant_get(block)).isTree);
	assert(plants.getPercentGrown(blocks.plant_get(block)) >= Config::minimumPercentGrowthForWoodCutting);
	blocks.designation_set(block, m_faction, BlockDesignation::WoodCutting);
	// To be called when block is no longer a suitable location, for example if it got crushed by a collapse.
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<WoodCuttingLocationDishonorCallback>(m_faction, m_area, block);
	m_data.try_emplace(block, &m_faction, m_area, block, std::move(locationDishonorCallback));
}
void HasWoodCuttingDesignationsForFaction::undesignate(BlockIndex block)
{
	assert(m_data.contains(block));
	WoodCuttingProject& project = m_data.at(block);
	project.cancel();
}
void HasWoodCuttingDesignationsForFaction::remove(BlockIndex block)
{
	assert(m_data.contains(block));
	m_area.getBlocks().designation_unset(block, m_faction, BlockDesignation::WoodCutting);
	m_data.erase(block); 
}
void HasWoodCuttingDesignationsForFaction::removeIfExists(BlockIndex block)
{
	if(m_data.contains(block))
		remove(block);
}
bool HasWoodCuttingDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void AreaHasWoodCuttingDesignations::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		Faction& faction = deserializationMemo.faction(pair[0]);
		m_data.try_emplace(&faction, pair[1], deserializationMemo, faction);
	}
}
Json AreaHasWoodCuttingDesignations::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first->name;
		jsonPair[1] = pair.second.toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void AreaHasWoodCuttingDesignations::addFaction(Faction& faction)
{
	assert(!m_data.contains(&faction));
	m_data.try_emplace(&faction, faction, m_area);
}
void AreaHasWoodCuttingDesignations::removeFaction(Faction& faction)
{
	assert(m_data.contains(&faction));
	m_data.erase(&faction);
}
// If blockFeatureType is null then woodCutting out fully rather then woodCuttingging out a feature.
void AreaHasWoodCuttingDesignations::designate(Faction& faction, BlockIndex block)
{
	if(!m_data.contains(&faction))
		addFaction(faction);
	m_data.at(&faction).designate(block);
}
void AreaHasWoodCuttingDesignations::undesignate(Faction& faction, BlockIndex block)
{
	assert(m_data.contains(&faction));
	m_data.at(&faction).undesignate(block);
}
void AreaHasWoodCuttingDesignations::remove(Faction& faction, BlockIndex block)
{
	assert(m_data.contains(&faction));
	m_data.at(&faction).remove(block);
}
void AreaHasWoodCuttingDesignations::clearAll(BlockIndex block)
{
	for(auto& pair : m_data)
		pair.second.removeIfExists(block);
}
void AreaHasWoodCuttingDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second.m_data)
			pair2.second.clearReservations();
}
bool AreaHasWoodCuttingDesignations::areThereAnyForFaction(Faction& faction) const
{
	if(!m_data.contains(&faction))
		return false;
	return !m_data.at(&faction).empty();
}
bool AreaHasWoodCuttingDesignations::contains(Faction& faction, BlockIndex block) const 
{ 
	if(!m_data.contains(&faction))
		return false;
	return m_data.at(&faction).m_data.contains(block); 
}
WoodCuttingProject& AreaHasWoodCuttingDesignations::at(Faction& faction, BlockIndex block) 
{ 
	assert(m_data.contains(&faction));
	assert(m_data.at(&faction).m_data.contains(block)); 
	return m_data.at(&faction).m_data.at(block); 
}
