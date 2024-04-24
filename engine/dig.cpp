#include "dig.h"
#include "block.h"
#include "area.h"
#include "blockFeature.h"
#include "deserializationMemo.h"
#include "deserializeDishonorCallbacks.h"
#include "random.h"
#include "reservable.h"
#include "util.h"
#include "simulation.h"
#include <memory>
#include <sys/types.h>
// Input.
void DesignateDigInputAction::execute()
{
	Block& block = *m_cuboid.begin();
	auto& digDesginations = block.m_area->m_hasDigDesignations;
	const Faction& faction = *(**m_actors.begin()).getFaction();
	for(Block& block : m_cuboid)
		digDesginations.designate(faction, block, m_blockFeatureType);
};
void UndesignateDigInputAction::execute()
{
	Block& block = *m_cuboid.begin();
	auto& digDesginations = block.m_area->m_hasDigDesignations;
	const Faction& faction = *(**m_actors.begin()).getFaction();
	for(Block& block : m_cuboid)
		digDesginations.undesignate(faction, block);
}
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
			m_digObjective.joinProject(project);
		else
			// Project can no longer accept this worker, try again.
			m_digObjective.m_digThreadedTask.create(m_digObjective);
	}
}
void DigThreadedTask::clearReferences() { m_digObjective.m_digThreadedTask.clearPointer(); }
DigObjective::DigObjective(Actor& a) : Objective(a, Config::digObjectivePriority), m_digThreadedTask(m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine) , m_project(nullptr) 
{ 
	assert(m_actor.getFaction() != nullptr);
}
DigObjective::DigObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_digThreadedTask(m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine), 
	m_project(data.contains("project") ? deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()) : nullptr)
{
	if(data.contains("threadedTask"))
		m_digThreadedTask.create(*this);
}
Json DigObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project;
	if(m_digThreadedTask.exists())
		data["threadedTask"] = true;
	return data;
}
void DigObjective::execute()
{
	if(m_project != nullptr)
		m_project->commandWorker(m_actor);
	else if(!m_digThreadedTask.exists())
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
void DigObjective::delay() 
{ 
	cancel(); 
	m_project = nullptr;
	m_actor.m_project = nullptr;
}
void DigObjective::reset() 
{ 
	if(m_project)
	{
		assert(!m_project->getWorkers().contains(&m_actor));
		m_project = nullptr; 
		m_actor.m_project = nullptr;
	}
	else
		assert(!m_actor.m_project);
	m_digThreadedTask.maybeCancel();
	m_actor.m_canReserve.deleteAllWithoutCallback();
}
void DigObjective::onProjectCannotReserve()
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void DigObjective::joinProject(DigProject& project)
{
	assert(!m_project);
	m_project = &project;
	project.addWorkerCandidate(m_actor, *this);
}
DigProject* DigObjective::getJoinableProjectAt(const Block& block)
{
	if(!block.m_hasDesignations.contains(*m_actor.getFaction(), BlockDesignation::Dig))
		return nullptr;
	DigProject& output = block.m_area->m_hasDigDesignations.at(*m_actor.getFaction(), block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return nullptr;
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
DigProject::DigProject(const Json& data, DeserializationMemo& deserializationMemo) : Project(data, deserializationMemo), 
	m_blockFeatureType(data.contains("blockFeatureType") ? &BlockFeatureType::byName(data["blockFeatureType"].get<std::string>()) : nullptr) { }
Json DigProject::toJson() const
{
	Json data = Project::toJson();
	if(m_blockFeatureType)
		data["blockFeatureType"] = m_blockFeatureType->name;
	return data;
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
	const MaterialType* materialType = m_location.isSolid() ? &m_location.getSolidMaterial() : m_location.m_hasBlockFeatures.getMaterialType();
	if(materialType)
	{
		for(const SpoilData& spoilData : getLocation().getSolidMaterial().spoilData)
		{
			if(!random.percentChance(spoilData.chance))
				continue;
			//TODO: reduce yield for block features.
			uint32_t quantity = random.getInRange(spoilData.min, spoilData.max);
			output.emplace_back(&spoilData.itemType, &spoilData.materialType, quantity);
		}
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
	if(!m_location.isSolid())
	{
		assert(!m_blockFeatureType);
		m_location.m_hasBlockFeatures.removeAll();
	}
	else
	{
		if(m_blockFeatureType == nullptr)
			m_location.setNotSolid();
		else
			m_location.m_hasBlockFeatures.hew(*m_blockFeatureType);
	}
	// Remove designations for other factions as well as owning faction.
	auto workers = std::move(m_workers);
	m_location.m_area->m_hasDigDesignations.clearAll(m_location);
	for(auto& [actor, projectWorker] : workers)
		actor->m_hasObjectives.objectiveComplete(projectWorker.objective);
}
void DigProject::onCancel()
{
	std::vector<Actor*> actors = getWorkersAndCandidates();
	m_location.m_area->m_hasDigDesignations.remove(m_faction, m_location);
	for(Actor* actor : actors)
	{
		static_cast<DigObjective&>(actor->m_hasObjectives.getCurrent()).m_project = nullptr;
		actor->m_project = nullptr;
		actor->m_hasObjectives.getCurrent().reset();
		actor->m_hasObjectives.cannotCompleteSubobjective();
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
DigLocationDishonorCallback::DigLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) : 
	m_faction(deserializationMemo.faction(data["faction"].get<std::wstring>())),
	m_location(deserializationMemo.m_simulation.getBlockForJsonQuery(data["location"])) { }
Json DigLocationDishonorCallback::toJson() const { return Json({{"type", "DigLocationDishonorCallback"}, {"faction", m_faction.name}, {"location", m_location.positionToJson()}}); }
void DigLocationDishonorCallback::execute([[maybe_unused]] uint32_t oldCount, [[maybe_unused]] uint32_t newCount)
{
	m_location.m_area->m_hasDigDesignations.undesignate(m_faction, m_location);
}
HasDigDesignationsForFaction::HasDigDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const Faction& faction) : m_faction(faction)
{
	for(const Json& pair : data)
	{
		Block& block = deserializationMemo.m_simulation.getBlockForJsonQuery(pair[0]);
		m_data.try_emplace(&block, pair[1], deserializationMemo);
	}
}
void HasDigDesignationsForFaction::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		Block& block = deserializationMemo.m_simulation.getBlockForJsonQuery(pair[0]);
		m_data.at(&block).loadWorkers(pair[1], deserializationMemo);
	}
}
Json HasDigDesignationsForFaction::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first->positionToJson(), pair.second.toJson()};
		data.push_back(jsonPair);
	}
	return data;
}
void HasDigDesignationsForFaction::designate(Block& block, const BlockFeatureType* blockFeatureType)
{
	assert(!m_data.contains(&block));
	block.m_hasDesignations.insert(m_faction, BlockDesignation::Dig);
	// To be called when block is no longer a suitable location, for example if it got dug out already.
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<DigLocationDishonorCallback>(m_faction, block);
	m_data.try_emplace(&block, &m_faction, block, blockFeatureType, std::move(locationDishonorCallback));
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
void HasDigDesignations::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		const Faction& faction = deserializationMemo.faction(pair[0]);
		m_data.try_emplace(&faction, pair[1], deserializationMemo, faction);
	}
}
void HasDigDesignations::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		const Faction& faction = deserializationMemo.faction(pair[0]);
		m_data.at(&faction).loadWorkers(pair[1], deserializationMemo);
	}

}
Json HasDigDesignations::toJson() const
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
	if(!m_data.contains(&faction))
		addFaction(faction);
	m_data.at(&faction).designate(block, blockFeatureType);
}
void HasDigDesignations::undesignate(const Faction& faction, Block& block)
{
	assert(m_data.contains(&faction));
	assert(m_data.at(&faction).m_data.contains(&block)); 
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
void HasDigDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair : pair.second.m_data)
			pair.second.clearReservations();
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
