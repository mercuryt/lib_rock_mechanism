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
		if(m_result.back()->m_reservable.isFullyReserved(*m_constructObjective.m_actor.getFaction()))
		{
			m_constructObjective.m_constructThreadedTask.create(m_constructObjective);
			return;
		}
		m_constructObjective.m_actor.m_canMove.setPath(m_result);
		m_result.back()->m_reservable.reserveFor(m_constructObjective.m_actor.m_canReserve, 1);
	}
}
void ConstructThreadedTask::clearReferences() { m_constructObjective.m_constructThreadedTask.clearPointer(); }
void ConstructObjective::execute()
{
	if(canConstructAt(*m_actor.m_location))
	{
		Block* block = selectAdjacentProject(*m_actor.m_location);
		assert(block != nullptr);
		m_actor.m_location->m_area->m_hasConstructionDesignations.getProject(*m_actor.getFaction(), *block).addWorker(m_actor);
	}
	else
		m_constructThreadedTask.create(*this);
}
void ConstructObjective::cancel()
{
	if(m_project != nullptr)
		m_project->removeWorker(m_actor);
}
bool ConstructObjective::canConstructAt(Block& block) const
{
	if(block.m_reservable.isFullyReserved(*m_actor.getFaction()))
		return false;
	for(Block* adjacent : block.getAdjacentWithEdgeAndCornerAdjacent())
		if(adjacent->m_hasDesignations.contains(*m_actor.getFaction(), BlockDesignation::Construct))
			return true;
	return false;
}
Block* ConstructObjective::selectAdjacentProject(Block& block) const
{
	for(Block* adjacent : block.getAdjacentWithEdgeAndCornerAdjacent())
		if(adjacent->m_hasDesignations.contains(*m_actor.getFaction(), BlockDesignation::Construct))
			return adjacent;
	return nullptr;
}
bool ConstructObjectiveType::canBeAssigned(Actor& actor) const
{
	return !actor.m_location->m_area->m_hasConstructionDesignations.areThereAnyForFaction(*actor.getFaction());
}
std::unique_ptr<Objective> ConstructObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<ConstructObjective>(actor);
}
std::vector<std::pair<ItemQuery, uint32_t>> ConstructProject::getConsumed() const
{
	assert(m_materialType.constructionData != nullptr);
	return m_materialType.constructionData->consumed;
}
std::vector<std::pair<ItemQuery, uint32_t>> ConstructProject::getUnconsumed() const
{
	assert(m_materialType.constructionData != nullptr);
	return m_materialType.constructionData->unconsumed;
}
std::vector<std::pair<ActorQuery, uint32_t>> ConstructProject::getActors() const { return {}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> ConstructProject::getByproducts() const
{
	assert(m_materialType.constructionData != nullptr);
	return m_materialType.constructionData->byproducts;
}
uint32_t ConstructProject::getWorkerConstructScore(Actor& actor) const
{
	static const SkillType& constructType = SkillType::byName("construct");
	return (actor.m_attributes.getStrength() * Config::constructStrengthModifier) + (actor.m_skillSet.get(constructType) * Config::constructSkillModifier);
}
void ConstructProject::onComplete()
{
	assert(!m_location.isSolid());
	if(m_blockFeatureType == nullptr)
		m_location.setSolid(m_materialType);
	else
		m_location.m_hasBlockFeatures.construct(*m_blockFeatureType, m_materialType);
}
// What would the total delay time be if we started from scratch now with current workers?
Step ConstructProject::getDelay() const
{
	uint32_t totalScore = 0;
	for(auto& pair : m_workers)
		totalScore += getWorkerConstructScore(*pair.first);
	return Config::constructScoreCost / totalScore;
}
// If blockFeatureType is null then construct a wall rather then a feature.
void HasConstructionDesignationsForFaction::designate(Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType)
{
	assert(!contains(block));
	m_data.try_emplace(&block, m_faction, block, blockFeatureType, materialType);
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
bool HasConstructionDesignationsForFaction::contains(const Block& block) const { return m_data.contains(const_cast<Block*>(&block)); }
const BlockFeatureType* HasConstructionDesignationsForFaction::at(const Block& block) const { return m_data.at(const_cast<Block*>(&block)).m_blockFeatureType; }
bool HasConstructionDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void HasConstructionDesignations::addFaction(const Faction& faction)
{
	assert(!m_data.contains(&faction));
	m_data.emplace(&faction, faction);
}
void HasConstructionDesignations::removeFaction(const Faction& faction)
{
	assert(m_data.contains(&faction));
	m_data.erase(&faction);
}
void HasConstructionDesignations::designate(const Faction& faction, Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType)
{
	m_data.at(&faction).designate(block, blockFeatureType, materialType);
}
void HasConstructionDesignations::remove(const Faction& faction, Block& block)
{
	assert(m_data.contains(&faction));
	m_data.at(&faction).remove(block);
}
void HasConstructionDesignations::clearAll(Block& block)
{
	for(auto& pair : m_data)
		pair.second.removeIfExists(block);
}
bool HasConstructionDesignations::areThereAnyForFaction(const Faction& faction) const
{
	return !m_data.at(&faction).empty();
}
ConstructProject& HasConstructionDesignations::getProject(const Faction& faction, Block& block)
{
	return m_data.at(&faction).m_data.at(&block);
}
