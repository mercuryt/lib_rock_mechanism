#include "woodcutting.h"
#include "actor.h"
#include "item.h"
#include "area.h"
#include "blockFeature.h"
#include "deserializationMemo.h"
#include "random.h"
#include "reservable.h"
#include "util.h"
#include "simulation.h"
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
WoodCuttingThreadedTask::WoodCuttingThreadedTask(WoodCuttingObjective& woodCuttingObjective) : ThreadedTask(woodCuttingObjective.m_actor.m_area->m_simulation.m_threadedTaskEngine), m_woodCuttingObjective(woodCuttingObjective), m_findsPath(woodCuttingObjective.m_actor, woodCuttingObjective.m_detour) { }
void WoodCuttingThreadedTask::readStep()
{
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
	{
		return m_woodCuttingObjective.getJoinableProjectAt(block) != nullptr;
	};
	m_findsPath.m_maxRange = Config::maxRangeToSearchForWoodCuttingDesignations;
	//TODO: We don't need the whole path here, just the destination and facing.
	m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *m_woodCuttingObjective.m_actor.getFaction());
}
void WoodCuttingThreadedTask::writeStep()
{
	if(!m_findsPath.found() && !m_findsPath.m_useCurrentLocation)
		m_woodCuttingObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_woodCuttingObjective);
	else
	{
		if(!m_findsPath.areAllBlocksAtDestinationReservable(m_woodCuttingObjective.m_actor.getFaction()))
		{
			// Proposed location while woodCuttingging has been reserved already, try to find another.
			m_woodCuttingObjective.m_woodCuttingThreadedTask.create(m_woodCuttingObjective);
			return;
		}
		BlockIndex target = m_findsPath.getBlockWhichPassedPredicate();
		WoodCuttingProject& project = m_woodCuttingObjective.m_actor.m_area->m_hasWoodCuttingDesignations.at(*m_woodCuttingObjective.m_actor.getFaction(), target);
		if(project.canAddWorker(m_woodCuttingObjective.m_actor))
		{
			// Join project and reserve standing room.
			m_woodCuttingObjective.joinProject(project);
			m_findsPath.reserveBlocksAtDestination(m_woodCuttingObjective.m_actor.m_canReserve);
		}
		else
			// Project can no longer accept this worker, try again.
			m_woodCuttingObjective.m_woodCuttingThreadedTask.create(m_woodCuttingObjective);
	}
}
void WoodCuttingThreadedTask::clearReferences() { m_woodCuttingObjective.m_woodCuttingThreadedTask.clearPointer(); }
WoodCuttingObjective::WoodCuttingObjective(Actor& a) : Objective(a, Config::woodCuttingObjectivePriority), m_woodCuttingThreadedTask(m_actor.m_area->m_simulation.m_threadedTaskEngine) , m_project(nullptr) 
{ 
	assert(m_actor.getFaction() != nullptr);
}
WoodCuttingObjective::WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_woodCuttingThreadedTask(m_actor.m_area->m_simulation.m_threadedTaskEngine), 
	m_project(data.contains("project") ? static_cast<WoodCuttingProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr)
{
	if(data.contains("threadedTask"))
		m_woodCuttingThreadedTask.create(*this);
}
Json WoodCuttingObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project;
	if(m_woodCuttingThreadedTask.exists())
		data["threadedTask"] = true;
	return data;
}
void WoodCuttingObjective::execute()
{
	if(m_project != nullptr)
		m_project->commandWorker(m_actor);
	else
	{
		WoodCuttingProject* project = nullptr;
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) 
		{ 
			if(!getJoinableProjectAt(block))
				return false;
			project = &m_actor.m_area->m_hasWoodCuttingDesignations.at(*m_actor.getFaction(), block);
			if(project->canAddWorker(m_actor))
				return true;
			return false;
		};
		[[maybe_unused]] BlockIndex adjacent = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
		if(project != nullptr)
		{
			assert(adjacent != BLOCK_INDEX_MAX);
			joinProject(*project);
			return;
		}
		m_woodCuttingThreadedTask.create(*this);
	}
}
void WoodCuttingObjective::cancel()
{
	if(m_project != nullptr)
		m_project->removeWorker(m_actor);
	m_woodCuttingThreadedTask.maybeCancel();
}
void WoodCuttingObjective::delay()
{
	cancel();
	m_project = nullptr;
	m_actor.m_project = nullptr;
}
void WoodCuttingObjective::reset() 
{ 
	if(m_project)
	{
		assert(!m_project->getWorkers().contains(&m_actor));
		m_project = nullptr; 
		m_actor.m_project = nullptr;
	}
	else 
		assert(!m_actor.m_project);
	m_woodCuttingThreadedTask.maybeCancel();
	m_actor.m_canReserve.deleteAllWithoutCallback();
}
void WoodCuttingObjective::onProjectCannotReserve()
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
WoodCuttingProject* WoodCuttingObjective::getJoinableProjectAt(BlockIndex block)
{
	if(!m_actor.m_area->getBlocks().designation_has(block, *m_actor.getFaction(), BlockDesignation::WoodCutting))
		return nullptr;
	WoodCuttingProject& output = m_actor.m_area->m_hasWoodCuttingDesignations.at(*m_actor.getFaction(), block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return nullptr;
	if(!output.canAddWorker(m_actor))
		return nullptr;
	return &output;
}
bool WoodCuttingObjectiveType::canBeAssigned(Actor& actor) const
{
	//TODO: check for any axes?
	return actor.m_area->m_hasWoodCuttingDesignations.areThereAnyForFaction(*actor.getFaction());
}
std::unique_ptr<Objective> WoodCuttingObjectiveType::makeFor(Actor& actor) const
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
	Plant& plant = m_area.getBlocks().plant_get(m_location);
	uint32_t unitsLogsGenerated = util::scaleByPercent(plant.m_plantSpecies.logsGeneratedByFellingWhenFullGrown, plant.getGrowthPercent());
	uint32_t unitsBranchesGenerated = util::scaleByPercent(plant.m_plantSpecies.branchesGeneratedByFellingWhenFullGrown, plant.getGrowthPercent());
	assert(unitsLogsGenerated);
	assert(unitsBranchesGenerated);
	auto& woodType = plant.m_plantSpecies.woodType;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> output{
		
		{&ItemType::byName("branch"), woodType, unitsBranchesGenerated},
		{&ItemType::byName("log"), woodType, unitsLogsGenerated}
	};
	return output;
}
// Static.
uint32_t WoodCuttingProject::getWorkerWoodCuttingScore(Actor& actor)
{
	static const SkillType& woodCuttingType = SkillType::byName("wood cutting");
	return (actor.m_attributes.getStrength() * Config::woodCuttingStrengthModifier) + (actor.m_skillSet.get(woodCuttingType) * Config::woodCuttingSkillModifier);
}
void WoodCuttingProject::onComplete()
{
	if(m_area.getBlocks().plant_exists(m_location))
	{
		Plant& plant = m_area.getBlocks().plant_get(m_location);
		plant.die();
	}
	// Remove designations for other factions as well as owning faction.
	auto workers = std::move(m_workers);
	m_area.m_hasWoodCuttingDesignations.clearAll(m_location);
	for(auto& [actor, projectWorker] : workers)
		actor->m_hasObjectives.objectiveComplete(projectWorker.objective);
}
void WoodCuttingProject::onCancel()
{
	std::vector<Actor*> actors = getWorkersAndCandidates();
	m_area.m_hasWoodCuttingDesignations.remove(m_faction, m_location);
	for(Actor* actor : actors)
	{
		static_cast<WoodCuttingObjective&>(actor->m_hasObjectives.getCurrent()).m_project = nullptr;
		actor->m_project = nullptr;
		actor->m_hasObjectives.getCurrent().reset();
		actor->m_hasObjectives.cannotCompleteSubobjective();
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
		totalScore += getWorkerWoodCuttingScore(*pair.first);
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
	assert(!m_data.contains(block));
	assert(blocks.plant_exists(block));
	assert(blocks.plant_get(block).m_plantSpecies.isTree);
	assert(blocks.plant_get(block).getGrowthPercent() >= Config::minimumPercentGrowthForWoodCutting);
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
