#include "dig.h"
#include "block.h"
#include "area.h"
#include "randomUtil.h"
#include "util.h"
DigThreadedTask::DigThreadedTask(DigObjective& digObjective) : ThreadedTask(digObjective.m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine), m_digObjective(digObjective), m_findsPath(digObjective.m_actor) { }
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
	if(!m_findsPath.found())
		m_digObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_digObjective);
	else
	{
		Block* destination = m_findsPath.getPath().back();
		Facing facing = m_findsPath.getFacingAtDestination();
		if(!m_digObjective.m_actor.allBlocksAtLocationAndFacingAreReservable(*destination, facing))
		{
			// Proposed location while digging has been reserved already, try to find another.
			m_digObjective.m_digThrededTask.create(m_digObjective);
			return;
		}
		// Find an adjacent project to contribute to.
		for(Block* block : m_digObjective.m_actor.getAdjacentAtLocationWithFacing(*destination, facing))
		{
			assert(m_digObjective.m_actor.getFaction() != nullptr);
			if(block->m_hasDesignations.contains(*m_digObjective.m_actor.getFaction(), BlockDesignation::Dig))
			{
				DigProject& project = block->m_area->m_hasDiggingDesignations.at(*m_digObjective.m_actor.getFaction(), *block);
				if(project.canAddWorker(m_digObjective.m_actor))
				{
					// Join project and reserve standing room.
					m_digObjective.m_project = &project;
					project.addWorker(m_digObjective.m_actor, m_digObjective);
					m_digObjective.m_actor.reserveAllBlocksAtLocationAndFacing(*destination, facing);
				}
			}
		}
		if(m_digObjective.m_project == nullptr)
			m_digObjective.m_digThrededTask.create(m_digObjective);
	}
}
void DigThreadedTask::clearReferences() { m_digObjective.m_digThrededTask.clearPointer(); }
DigObjective::DigObjective(Actor& a) : Objective(Config::digObjectivePriority), m_actor(a), m_digThrededTask(m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine) , m_project(nullptr) 
{ 
	assert(m_actor.getFaction() != nullptr);
}
void DigObjective::execute()
{
	if(m_project != nullptr)
		m_project->commandWorker(m_actor);
	else
		m_digThrededTask.create(*this);
}
void DigObjective::cancel()
{
	if(m_project != nullptr)
		m_project->removeWorker(m_actor);
	m_digThrededTask.maybeCancel();
}
DigProject* DigObjective::getJoinableProjectAt(const Block& block)
{
	if(!block.m_hasDesignations.contains(*m_actor.getFaction(), BlockDesignation::Dig))
		return nullptr;
	DigProject& output = block.m_area->m_hasDiggingDesignations.at(*m_actor.getFaction(), block);
	if(!output.canAddWorker(m_actor))
		return nullptr;
	return &output;
}
bool DigObjectiveType::canBeAssigned(Actor& actor) const
{
	//TODO: check for any picks?
	return actor.m_location->m_area->m_hasDiggingDesignations.areThereAnyForFaction(*actor.getFaction());
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
	for(const SpoilData& spoilData : getLocation().getSolidMaterial().spoilData)
	{
		if(!randomUtil::chance(spoilData.chance))
			continue;
		uint32_t quantity = randomUtil::getInRange(spoilData.min, spoilData.max);
		getLocation().m_hasItems.add(spoilData.itemType, spoilData.materialType, quantity);
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
	if(blockFeatureType == nullptr)
		getLocation().setNotSolid();
	else
		getLocation().m_hasBlockFeatures.hew(*blockFeatureType);
	// Remove designations for other factions as well as owning faction.
	getLocation().m_area->m_hasDiggingDesignations.clearAll(getLocation());

}
// What would the total delay time be if we started from scratch now with current workers?
Step DigProject::getDelay() const
{
	uint32_t totalScore = 0;
	for(auto& pair : m_workers)
		totalScore += getWorkerDigScore(*pair.first);
	return Config::digScoreCost / totalScore;
}
void HasDigDesignationsForFaction::designate(Block& block, const BlockFeatureType* blockFeatureType)
{
	assert(!m_data.contains(&block));
	block.m_hasDesignations.insert(m_faction, BlockDesignation::Dig);
	m_data.try_emplace(&block, &m_faction, block, blockFeatureType);
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
const BlockFeatureType* HasDigDesignationsForFaction::at(const Block& block) const { return m_data.at(const_cast<Block*>(&block)).blockFeatureType; }
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
	return !m_data.at(&faction).empty();
}
DigProject& HasDigDesignations::at(const Faction& faction, const Block& block) 
{ 
	assert(m_data.contains(&faction));
	assert(m_data.at(&faction).m_data.contains(const_cast<Block*>(&block))); 
	return m_data.at(&faction).m_data.at(const_cast<Block*>(&block)); 
}
