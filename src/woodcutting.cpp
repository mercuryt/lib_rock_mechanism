#include "woodcutting.h"
#include "block.h"
#include "area.h"
#include "blockFeature.h"
#include "deserializationMemo.h"
#include "random.h"
#include "reservable.h"
#include "util.h"
#include <memory>
#include <sys/types.h>
// Input.
void DesignateWoodCuttingInputAction::execute()
{
	Block& block = *m_blocks.begin();
	auto& woodCuttingDesginations = block.m_area->m_hasWoodCuttingDesignations;
	const Faction& faction = *(**m_actors.begin()).getFaction();
	for(Block& block : m_blocks)
		woodCuttingDesginations.designate(faction, block);
};
void UndesignateWoodCuttingInputAction::execute()
{
	Block& block = *m_blocks.begin();
	auto& woodCuttingDesginations = block.m_area->m_hasWoodCuttingDesignations;
	const Faction& faction = *(**m_actors.begin()).getFaction();
	for(Block& block : m_blocks)
		woodCuttingDesginations.undesignate(faction, block);
}
WoodCuttingThreadedTask::WoodCuttingThreadedTask(WoodCuttingObjective& woodCuttingObjective) : ThreadedTask(woodCuttingObjective.m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine), m_woodCuttingObjective(woodCuttingObjective), m_findsPath(woodCuttingObjective.m_actor, woodCuttingObjective.m_detour) { }
void WoodCuttingThreadedTask::readStep()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		return m_woodCuttingObjective.getJoinableProjectAt(block) != nullptr;
	};
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
		Block& target = *m_findsPath.getBlockWhichPassedPredicate();
		WoodCuttingProject& project = target.m_area->m_hasWoodCuttingDesignations.at(*m_woodCuttingObjective.m_actor.getFaction(), target);
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
WoodCuttingObjective::WoodCuttingObjective(Actor& a) : Objective(a, Config::woodCuttingObjectivePriority), m_woodCuttingThreadedTask(m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine) , m_project(nullptr) 
{ 
	assert(m_actor.getFaction() != nullptr);
}
WoodCuttingObjective::WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_woodCuttingThreadedTask(m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine), 
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
		std::function<bool(const Block&)> predicate = [&](const Block& block) 
		{ 
			if(!getJoinableProjectAt(block))
				return false;
			project = &block.m_area->m_hasWoodCuttingDesignations.at(*m_actor.getFaction(), block);
			if(project->canAddWorker(m_actor))
				return true;
			return false;
		};
		Block* adjacent = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
		if(project != nullptr)
		{
			assert(adjacent != nullptr);
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
void WoodCuttingObjective::reset() 
{ 
	m_woodCuttingThreadedTask.maybeCancel();
	m_project = nullptr; 
	m_actor.m_project = nullptr;
	m_actor.m_canReserve.clearAll();
}
void WoodCuttingObjective::joinProject(WoodCuttingProject& project)
{
	assert(m_project == nullptr);
	m_project = &project;
	project.addWorkerCandidate(m_actor, *this);
}
WoodCuttingProject* WoodCuttingObjective::getJoinableProjectAt(const Block& block)
{
	if(!block.m_hasDesignations.contains(*m_actor.getFaction(), BlockDesignation::WoodCutting))
		return nullptr;
	WoodCuttingProject& output = block.m_area->m_hasWoodCuttingDesignations.at(*m_actor.getFaction(), block);
	if(!output.canAddWorker(m_actor))
		return nullptr;
	return &output;
}
bool WoodCuttingObjectiveType::canBeAssigned(Actor& actor) const
{
	//TODO: check for any axes?
	return actor.m_location->m_area->m_hasWoodCuttingDesignations.areThereAnyForFaction(*actor.getFaction());
}
std::unique_ptr<Objective> WoodCuttingObjectiveType::makeFor(Actor& actor) const
{
	std::unique_ptr<Objective> objective = std::make_unique<WoodCuttingObjective>(actor);
	return objective;
}
WoodCuttingProject::WoodCuttingProject(const Json& data, DeserializationMemo& deserializationMemo) : Project(data, deserializationMemo) { }
std::vector<std::pair<ItemQuery, uint32_t>> WoodCuttingProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, uint32_t>> WoodCuttingProject::getUnconsumed() const
{
	static const ItemType& axe = ItemType::byName("axe");
	return {{axe, 1}}; 
}
std::vector<std::pair<ActorQuery, uint32_t>> WoodCuttingProject::getActors() const { return {}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> WoodCuttingProject::getByproducts() const
{
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> output;
	Random& random = m_location.m_area->m_simulation.m_random;
	for(const SpoilData& spoilData : getLocation().getSolidMaterial().spoilData)
	{
		if(!random.percentChance(spoilData.chance))
			continue;
		uint32_t quantity = random.getInRange(spoilData.min, spoilData.max);
		output.emplace_back(&spoilData.itemType, &spoilData.materialType, quantity);
	}
	return output;
}
// Static.
uint32_t WoodCuttingProject::getWorkerWoodCuttingScore(Actor& actor)
{
	static const SkillType& woodCuttingType = SkillType::byName("woodCutting");
	return (actor.m_attributes.getStrength() * Config::woodCuttingStrengthModifier) + (actor.m_skillSet.get(woodCuttingType) * Config::woodCuttingSkillModifier);
}
void WoodCuttingProject::onComplete()
{
	if(m_location.m_hasPlant.exists())
	{
		Plant& plant = m_location.m_hasPlant.get();
		// Create logs and branches.
		uint32_t unitsLogsGenerated = util::scaleByPercent(plant.m_plantSpecies.logsGeneratedByFellingWhenFullGrown, plant.getGrowthPercent());
		uint32_t unitsBranchesGenerated = util::scaleByPercent(plant.m_plantSpecies.branchesGeneratedByFellingWhenFullGrown, plant.getGrowthPercent());
		plant.die();
		static const ItemType& log = ItemType::byName("log");
		static const ItemType& branch = ItemType::byName("branch");
		m_location.m_hasItems.addGeneric(log, *plant.m_plantSpecies.woodType, unitsLogsGenerated);
		m_location.m_hasItems.addGeneric(branch, *plant.m_plantSpecies.woodType, unitsBranchesGenerated);
	}
	// Remove designations for other factions as well as owning faction.
	auto workers = std::move(m_workers);
	m_location.m_area->m_hasWoodCuttingDesignations.clearAll(m_location);
	for(auto& [actor, projectWorker] : workers)
		actor->m_hasObjectives.objectiveComplete(projectWorker.objective);
}
void WoodCuttingProject::onCancel()
{
	std::vector<Actor*> actors = getWorkersAndCandidates();
	m_location.m_area->m_hasWoodCuttingDesignations.remove(m_faction, m_location);
	for(Actor* actor : actors)
	{
		static_cast<WoodCuttingObjective&>(actor->m_hasObjectives.getCurrent()).m_project = nullptr;
		actor->m_project = nullptr;
		actor->m_hasObjectives.getCurrent().reset();
		actor->m_hasObjectives.cannotCompleteTask();
	}
}
void WoodCuttingProject::onDelay()
{
	m_location.m_hasDesignations.removeIfExists(m_faction, BlockDesignation::WoodCutting);
}
void WoodCuttingProject::offDelay()
{
	m_location.m_hasDesignations.insert(m_faction, BlockDesignation::WoodCutting);
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
	m_location(deserializationMemo.m_simulation.getBlockForJsonQuery(data["location"])) { }
Json WoodCuttingLocationDishonorCallback::toJson() const { return Json({{"type", "WoodCuttingLocationDishonorCallback"}, {"faction", m_faction.m_name}, {"location", m_location.positionToJson()}}); }
void WoodCuttingLocationDishonorCallback::execute([[maybe_unused]] uint32_t oldCount, [[maybe_unused]] uint32_t newCount)
{
	m_location.m_area->m_hasWoodCuttingDesignations.undesignate(m_faction, m_location);
}
HasWoodCuttingDesignationsForFaction::HasWoodCuttingDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const Faction& faction) : m_faction(faction)
{
	for(const Json& pair : data)
	{
		Block& block = deserializationMemo.m_simulation.getBlockForJsonQuery(pair[0]);
		m_data.try_emplace(&block, pair[1], deserializationMemo);
	}
}
Json HasWoodCuttingDesignationsForFaction::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first->positionToJson(), pair.second.toJson()};
		data.push_back(jsonPair);
	}
	return data;
}
void HasWoodCuttingDesignationsForFaction::designate(Block& block)
{
	assert(!m_data.contains(&block));
	assert(block.m_hasPlant.exists());
	assert(block.m_hasPlant.get().m_plantSpecies.isTree);
	block.m_hasDesignations.insert(m_faction, BlockDesignation::WoodCutting);
	// To be called when block is no longer a suitable location, for example if it got crushed by a collapse.
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<WoodCuttingLocationDishonorCallback>(m_faction, block);
	m_data.try_emplace(&block, &m_faction, block, std::move(locationDishonorCallback));
}
void HasWoodCuttingDesignationsForFaction::undesignate(Block& block)
{
	assert(m_data.contains(&block));
	WoodCuttingProject& project = m_data.at(&block);
	project.cancel();
}
void HasWoodCuttingDesignationsForFaction::remove(Block& block)
{
	assert(m_data.contains(&block));
	block.m_hasDesignations.remove(m_faction, BlockDesignation::WoodCutting);
	m_data.erase(&block); 
}
void HasWoodCuttingDesignationsForFaction::removeIfExists(Block& block)
{
	if(m_data.contains(&block))
		remove(block);
}
bool HasWoodCuttingDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void HasWoodCuttingDesignations::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		const Faction& faction = deserializationMemo.faction(pair[0]);
		m_data.try_emplace(&faction, pair[1], deserializationMemo, faction);
	}
}
Json HasWoodCuttingDesignations::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first->m_name;
		jsonPair[1] = pair.second.toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void HasWoodCuttingDesignations::addFaction(const Faction& faction)
{
	assert(!m_data.contains(&faction));
	m_data.emplace(&faction, faction);
}
void HasWoodCuttingDesignations::removeFaction(const Faction& faction)
{
	assert(m_data.contains(&faction));
	m_data.erase(&faction);
}
// If blockFeatureType is null then woodCutting out fully rather then woodCuttingging out a feature.
void HasWoodCuttingDesignations::designate(const Faction& faction, Block& block)
{
	m_data.at(&faction).designate(block);
}
void HasWoodCuttingDesignations::undesignate(const Faction& faction, Block& block)
{
	m_data.at(&faction).undesignate(block);
}
void HasWoodCuttingDesignations::remove(const Faction& faction, Block& block)
{
	assert(m_data.contains(&faction));
	m_data.at(&faction).remove(block);
}
void HasWoodCuttingDesignations::clearAll(Block& block)
{
	for(auto& pair : m_data)
		pair.second.removeIfExists(block);
}
bool HasWoodCuttingDesignations::areThereAnyForFaction(const Faction& faction) const
{
	if(!m_data.contains(&faction))
		return false;
	return !m_data.at(&faction).empty();
}
WoodCuttingProject& HasWoodCuttingDesignations::at(const Faction& faction, const Block& block) 
{ 
	assert(m_data.contains(&faction));
	assert(m_data.at(&faction).m_data.contains(const_cast<Block*>(&block))); 
	return m_data.at(&faction).m_data.at(const_cast<Block*>(&block)); 
}
