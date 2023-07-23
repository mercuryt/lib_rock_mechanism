#include "construct.h"
#include "item.h"
#include "block.h"
#include "path.h"
#include "area.h"
#include "util.h"
void ConstructThreadedTask::readStep()
{
	auto destinationCondition = [&](Block& block)
	{
		return m_constructObjective.canConstructAt(block);
	};
	m_result = path::getForActorToPredicate(m_constructObjective.m_actor, destinationCondition);
}
void ConstructThreadedTask::writeStep()
{
	if(m_result.empty())
		m_constructObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_constructObjective);
	else
	{
		// Destination block has been reserved since result was found, get a new one.
		if(m_result.back()->m_reservable.isFullyReserved())
		{
			m_constructObjective.m_constructThreadedTask.create(m_constructObjective);
			return;
		}
		m_constructObjective.m_actor.m_canMove.setPath(m_result);
		m_result.back()->m_reservable.reserveFor(m_constructObjective.m_actor.m_canReserve, 1);
	}
}
void ConstructObjective::execute()
{
	if(canConstructAt(*m_actor.m_location))
	{
		Block* block = selectAdjacentProject(*m_actor.m_location);
		assert(block != nullptr);
		m_actor.m_location->m_area->m_hasConstructionDesignations.at(*m_actor.m_faction, *block).addWorker(m_actor);
	}
	else
		m_constructThreadedTask.create(*this);
}
bool ConstructObjective::canConstructAt(Block& block) const
{
	if(block.m_reservable.isFullyReserved())
		return false;
	for(Block* adjacent : block.getAdjacentWithEdgeAndCornerAdjacent())
		if(adjacent->m_hasDesignations.contains(*m_actor.m_faction, BlockDesignation::Construct))
			return true;
	return false;
}
Block* ConstructObjective::selectAdjacentProject(Block& block) const
{
	for(Block* adjacent : block.getAdjacentWithEdgeAndCornerAdjacent())
		if(adjacent->m_hasDesignations.contains(*m_actor.m_faction, BlockDesignation::Construct))
			return adjacent;
	return nullptr;
}
bool ConstructObjectiveType::canBeAssigned(Actor& actor)
{
	return !actor.m_location->m_area->m_hasConstructionDesignations.areThereAnyForFaction(*actor.m_faction);
}
std::unique_ptr<Objective> ConstructObjectiveType::makeFor(Actor& actor)
{
	return std::make_unique<ConstructObjective>(actor);
}
std::vector<std::pair<ItemQuery, uint32_t>> ConstructProject::getConsumed() const
{
	assert(materialType.constructionData != nullptr);
	return materialType.constructionData->consumed;
}
std::vector<std::pair<ItemQuery, uint32_t>> ConstructProject::getUnconsumed() const
{
	assert(materialType.constructionData != nullptr);
	return materialType.constructionData->unconsumed;
}
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> ConstructProject::getByproducts() const
{
	assert(materialType.constructionData != nullptr);
	return materialType.constructionData->byproducts;
}
uint32_t ConstructProject::getWorkerConstructScore(Actor& actor) const
{
	return (actor.m_attributes.getStrength() * Config::constructStrengthModifier) + (actor.m_skillSet.get(SkillType::construct) * Config::constructSkillModifier);
}
/*
void ConstructDesignation::onComplete()
{
	assert(!m_location.isSolid());
	if(blockFeatureType == nullptr)
		m_location.setSolid(m_materialType);
	else
		m_location.m_hasBlockFeatures.construct(blockFeatureType, m_materialType);
}
// What would the total delay time be if we started from scratch now with current workers?
uint32_t ConstructDesignation::getDelay() const
{
	uint32_t totalScore = 0;
	for(Actor* actor : m_workers)
		totalScore += getWorkerConstructScore(*actor);
	return Config::constructScoreCost / totalScore;
}
*/
// If blockFeatureType is null then construct a wall rather then a feature.
void HasConstructionDesignationsForFaction::designate(Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType)
{
	assert(!contains(block));
	m_data.try_emplace(&block, block, blockFeatureType, materialType);
	block.m_hasDesignations.insert(m_faction, BlockDesignation::Construct);
}
void HasConstructionDesignationsForFaction::remove(Block& block)
{
	assert(contains(block));
	m_data.erase(&block); 
	block.m_hasDesignations.remove(m_faction, BlockDesignation::Construct);
}
void HasConstructionDesignationsForFaction::removeIfExists(Block& block)
{
	if(m_data.contains(&block))
		remove(block);
}
bool HasConstructionDesignationsForFaction::contains(Block& block) const { return m_data.contains(&block); }
const BlockFeatureType* HasConstructionDesignationsForFaction::at(Block& block) const { return m_data.at(&block).blockFeatureType; }
bool HasConstructionDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void HasConstructionDesignations::addFaction(Faction& faction)
{
	assert(!m_data.contains(&faction));
	m_data.emplace(&faction, faction);
}
void HasConstructionDesignations::removeFaction(Faction& faction)
{
	assert(m_data.contains(&faction));
	m_data.erase(&faction);
}
void HasConstructionDesignations::designate(Faction& faction, Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType)
{
	m_data.at(&faction).designate(block, blockFeatureType, materialType);
}
void HasConstructionDesignations::remove(Faction& faction, Block& block)
{
	assert(m_data.contains(&faction));
	m_data.at(&faction).remove(block);
}
void HasConstructionDesignations::clearAll(Block& block)
{
	for(auto& pair : m_data)
		pair.second.removeIfExists(block);
}
bool HasConstructionDesignations::areThereAnyForFaction(Faction& faction) const
{
	return !m_data.at(&faction).empty();
}
