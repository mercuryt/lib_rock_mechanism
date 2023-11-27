#include "dig.h"
#include "block.h"
#include "area.h"
#include "random.h"
#include "reservable.h"
#include "util.h"
#include <sys/types.h>
DigThreadedTask::DigThreadedTask(DigObjective& digObjective) : ThreadedTask(digObjective.m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine), m_digObjective(digObjective), m_findsPath(digObjective.m_actor, digObjective.m_detour) { }
void DigThreadedTask::readStep()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		return m_digObjective.getJoinableProjectAt(block) != nullptr;
	};
	//TODO: We don't need the whole path here, just the destination and facing.
	m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *m_digObjective.m_actor.getFaction());
}
void DigThreadedTask::writeStep()
{
	if(!m_findsPath.found() && !m_findsPath.m_useCurrentLocation)
		m_digObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_digObjective);
	else
	{
		if(!m_findsPath.areAllBlocksAtDestinationReservable(m_digObjective.m_actor.getFaction()))
		{
			// Proposed location while digging has been reserved already, try to find another.
			m_digObjective.m_digThreadedTask.create(m_digObjective);
			return;
		}
		Block& target = *m_findsPath.getBlockWhichPassedPredicate();
		DigProject& project = target.m_area->m_hasDigDesignations.at(*m_digObjective.m_actor.getFaction(), target);
		if(project.canAddWorker(m_digObjective.m_actor))
		{
			// Join project and reserve standing room.
			m_digObjective.joinProject(project);
			m_findsPath.reserveBlocksAtDestination(m_digObjective.m_actor.m_canReserve);
		}
		else
			// Project can no longer accept this worker, try again.
			m_digObjective.m_digThreadedTask.create(m_digObjective);
	}
}
void DigThreadedTask::clearReferences() { m_digObjective.m_digThreadedTask.clearPointer(); }
DigObjective::DigObjective(Actor& a) : Objective(Config::digObjectivePriority), m_actor(a), m_digThreadedTask(m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine) , m_project(nullptr) 
{ 
	assert(m_actor.getFaction() != nullptr);
}
void DigObjective::execute()
{
	if(m_project != nullptr)
		m_project->commandWorker(m_actor);
	else
	{
		DigProject* project = nullptr;
		std::function<bool(const Block&)> predicate = [&](const Block& block) 
		{ 
			if(!getJoinableProjectAt(block))
				return false;
			project = &block.m_area->m_hasDigDesignations.at(*m_actor.getFaction(), block);
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
		m_digThreadedTask.create(*this);
	}
}
void DigObjective::cancel()
{
	if(m_project != nullptr)
		m_project->removeWorker(m_actor);
	m_digThreadedTask.maybeCancel();
}
void DigObjective::reset() 
{ 
	m_digThreadedTask.maybeCancel();
	m_project = nullptr; 
	m_actor.m_project = nullptr;
	m_actor.m_canReserve.clearAll();
}
void DigObjective::joinProject(DigProject& project)
{
	assert(m_project == nullptr);
	m_project = &project;
	project.addWorkerCandidate(m_actor, *this);
}
DigProject* DigObjective::getJoinableProjectAt(const Block& block)
{
	if(!block.m_hasDesignations.contains(*m_actor.getFaction(), BlockDesignation::Dig))
		return nullptr;
	DigProject& output = block.m_area->m_hasDigDesignations.at(*m_actor.getFaction(), block);
	if(!output.canAddWorker(m_actor))
		return nullptr;
	return &output;
}
bool DigObjectiveType::canBeAssigned(Actor& actor) const
{
	//TODO: check for any picks?
	return actor.m_location->m_area->m_hasDigDesignations.areThereAnyForFaction(*actor.getFaction());
}
std::unique_ptr<Objective> DigObjectiveType::makeFor(Actor& actor) const
{
	std::unique_ptr<Objective> objective = std::make_unique<DigObjective>(actor);
	return objective;
}
std::vector<std::pair<ItemQuery, uint32_t>> DigProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, uint32_t>> DigProject::getUnconsumed() const
{
	static const ItemType& pick = ItemType::byName("pick");
	return {{pick, 1}}; 
}
std::vector<std::pair<ActorQuery, uint32_t>> DigProject::getActors() const { return {}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> DigProject::getByproducts() const
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
uint32_t DigProject::getWorkerDigScore(Actor& actor)
{
	static const SkillType& digType = SkillType::byName("dig");
	return (actor.m_attributes.getStrength() * Config::digStrengthModifier) + (actor.m_skillSet.get(digType) * Config::digSkillModifier);
}
void DigProject::onComplete()
{
	if(m_blockFeatureType == nullptr)
		m_location.setNotSolid();
	else
		m_location.m_hasBlockFeatures.hew(*m_blockFeatureType);
	// Remove designations for other factions as well as owning faction.
	auto workers = std::move(m_workers);
	m_location.m_area->m_hasDigDesignations.clearAll(m_location);
	for(auto& [actor, projectWorker] : workers)
		actor->m_hasObjectives.objectiveComplete(projectWorker.objective);
}
void DigProject::onCancel()
{
	//TODO: use std::copy with a projection.
	std::vector<Actor*> actors;
	actors.reserve(m_workers.size());
	for(auto& pair : m_workers)
		actors.push_back(pair.first);
	m_location.m_area->m_hasDigDesignations.remove(m_faction, m_location);
	for(Actor* actor : actors)
	{
		static_cast<DigObjective&>(actor->m_hasObjectives.getCurrent()).m_project = nullptr;
		actor->m_project = nullptr;
		actor->m_hasObjectives.getCurrent().reset();
		actor->m_hasObjectives.cannotCompleteTask();
	}
}
void DigProject::onDelay()
{
	m_location.m_hasDesignations.removeIfExists(m_faction, BlockDesignation::Dig);
}
void DigProject::offDelay()
{
	m_location.m_hasDesignations.insert(m_faction, BlockDesignation::Dig);
}
// What would the total delay time be if we started from scratch now with current workers?
Step DigProject::getDuration() const
{
	uint32_t totalScore = 0u;
	for(auto& pair : m_workers)
		totalScore += getWorkerDigScore(*pair.first);
	return std::max(Step(1u), Config::digMaxSteps / totalScore);
}
void HasDigDesignationsForFaction::designate(Block& block, const BlockFeatureType* blockFeatureType)
{
	assert(!m_data.contains(&block));
	block.m_hasDesignations.insert(m_faction, BlockDesignation::Dig);
	// To be called when block is no longer a suitable location, for example if it got dug out already.
	DishonorCallback locationDishonorCallback = [&]([[maybe_unused]] uint32_t oldCount, [[maybe_unused]] uint32_t newCount) { undesignate(block); };
	m_data.try_emplace(&block, &m_faction, block, blockFeatureType, locationDishonorCallback);
}
void HasDigDesignationsForFaction::undesignate(Block& block)
{
	assert(m_data.contains(&block));
	DigProject& project = m_data.at(&block);
	project.cancel();
}
void HasDigDesignationsForFaction::remove(Block& block)
{
	assert(m_data.contains(&block));
	block.m_hasDesignations.remove(m_faction, BlockDesignation::Dig);
	m_data.erase(&block); 
}
void HasDigDesignationsForFaction::removeIfExists(Block& block)
{
	if(m_data.contains(&block))
		remove(block);
}
const BlockFeatureType* HasDigDesignationsForFaction::at(const Block& block) const { return m_data.at(const_cast<Block*>(&block)).m_blockFeatureType; }
bool HasDigDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void HasDigDesignations::addFaction(const Faction& faction)
{
	assert(!m_data.contains(&faction));
	m_data.emplace(&faction, faction);
}
void HasDigDesignations::removeFaction(const Faction& faction)
{
	assert(m_data.contains(&faction));
	m_data.erase(&faction);
}
// If blockFeatureType is null then dig out fully rather then digging out a feature.
void HasDigDesignations::designate(const Faction& faction, Block& block, const BlockFeatureType* blockFeatureType)
{
	m_data.at(&faction).designate(block, blockFeatureType);
}
void HasDigDesignations::undesignate(const Faction& faction, Block& block)
{
	m_data.at(&faction).undesignate(block);
}
void HasDigDesignations::remove(const Faction& faction, Block& block)
{
	assert(m_data.contains(&faction));
	m_data.at(&faction).remove(block);
}
void HasDigDesignations::clearAll(Block& block)
{
	for(auto& pair : m_data)
		pair.second.removeIfExists(block);
}
bool HasDigDesignations::areThereAnyForFaction(const Faction& faction) const
{
	if(!m_data.contains(&faction))
		return false;
	return !m_data.at(&faction).empty();
}
DigProject& HasDigDesignations::at(const Faction& faction, const Block& block) 
{ 
	assert(m_data.contains(&faction));
	assert(m_data.at(&faction).m_data.contains(const_cast<Block*>(&block))); 
	return m_data.at(&faction).m_data.at(const_cast<Block*>(&block)); 
}
