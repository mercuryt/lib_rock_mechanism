#include "dig.h"
#include "block.h"
#include "area.h"
#include "path.h"
#include "randomUtil.h"
#include "util.h"
void DigThreadedTask::readStep()
{
	auto destinationCondition = [&](Block& block)
	{
		return m_digObjective.canDigAt(block);
	};
	m_result = path::getForActorToPredicate(m_digObjective.m_actor, destinationCondition);
}
void DigThreadedTask::writeStep()
{
	if(m_result.empty())
		m_digObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_digObjective);
	else
	{
		// Destination block has been reserved since result was found, get a new one.
		if(m_result.back()->m_reservable.isFullyReserved(*m_digObjective.m_actor.m_faction))
		{
			m_digObjective.m_digThrededTask.create(m_digObjective);
			return;
		}
		m_digObjective.m_actor.m_canMove.setPath(m_result);
		m_result.back()->m_reservable.reserveFor(m_digObjective.m_actor.m_canReserve, 1);
	}
}
void DigThreadedTask::clearReferences() { m_digObjective.m_digThrededTask.clearPointer(); }
void DigObjective::execute()
{
	if(m_project != nullptr)
	{
		m_project->commandWorker(m_actor);
		return;
	}
	if(canDigAt(*m_actor.m_location))
	{
		for(Block* adjacent : m_actor.m_location->getAdjacentWithEdgeAdjacent())
			if(!adjacent->m_reservable.isFullyReserved(*m_actor.m_faction) && m_actor.m_location->m_area->m_hasDiggingDesignations.contains(*m_actor.m_faction, *adjacent))
			{
				m_project = &m_actor.m_location->m_area->m_hasDiggingDesignations.at(*m_actor.m_faction, *adjacent);
				m_project->addWorker(m_actor);
			}
	}
	else
		m_digThrededTask.create(*this);
}
void DigObjective::cancel()
{
	if(m_project != nullptr)
		m_project->removeWorker(m_actor);
}
bool DigObjective::canDigAt(Block& block) const
{
	if(block.m_reservable.isFullyReserved(*m_actor.m_faction))
		return false;
	for(Block* adjacent : block.getAdjacentWithEdgeAndCornerAdjacent())
		if(adjacent->m_hasDesignations.contains(*m_actor.m_faction, BlockDesignation::Dig))
			return true;
	return false;
}
bool DigObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasDiggingDesignations.areThereAnyForFaction(*actor.m_faction);
}
std::unique_ptr<Objective> DigObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<DigObjective>(actor);
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
	m_data.try_emplace(&block, m_faction, block, blockFeatureType);
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
