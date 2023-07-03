#include "construct.h"
void ConstructThreadedTask::readStep()
{
	auto destinationCondition = [&](Block* block)
	{
		return m_constructObjective.canConstructAt(*block);
	}
	m_result = path::getForActorToPredicate(m_constructObjective.m_actor, destinationCondition)
}
void ConstructThreadedTask::writeStep()
{
	if(m_result.empty())
		m_constructObjective.m_actor.m_hasObjectives.cannotFulfillObjective();
	else
	{
		// Destination block has been reserved since result was found, get a new one.
		if(m_result.back()->reservable.isFullyReserved())
		{
			m_constructObjective.m_constructThrededTask.create(m_constructObjective);
			return;
		}
		m_constructObjective.m_actor.setPath(m_result);
		m_result.back()->reservable.reserveFor(m_constructObjective.m_actor.m_canReserve);
	}
}
void ConstructObjective::execute()
{
	if(canConstructAt(*m_actor.m_location))
	{
		for(Block* adjacent : m_actor.m_location->getAdjacentWithEdgeAdjacent())
			if(!adjacent.m_reservable.isFullyReserved() && m_actor.m_location->m_area->m_constructDesignations.contains(*adjacent))
				m_actor.m_location->m_area->m_constructDesignations.at(*adjacent).addWorker(m_actor);
	}
	else
		m_constructThreadedTask.create(*this);
}
bool ConstructObjective::canConstructAt(Block& block) const
{
	if(block->m_reservable.isFullyReserved())
		return false;
	for(Block* adjacent : block.getAdjacentWithEdgeAndCornerAdjacent())
		if(adjacent->m_hasDesignations.contains(m_actor.getPlayer(), BlockDesignation::Construct))
			return true;
	return false;
}
bool ConstructObjectiveType::canBeAssigned(Actor& actor)
{
	return !actor.m_location->m_area->m_constructDesignations.areThereAnyForPlayer(actor.getPlayer());
}
std::unique_ptr<Objective> ConstructObjectiveType::makeFor(Actor& actor)
{
	return std::make_pointer<ConstructObjective>(actor);
}
std::vector<std::pair<ItemQuery, uint32_t>> ConstructProject::getConsumed() const
{
	return materialType.constructionType.consumed;
}
std::vector<std::pair<ItemQuery, uint32_t>> ConstructProject::getUnconsumed() const
{
	return materialType.constructionType.unconsumed;
}
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t quantity>> ConstructProject::getByproducts() const
{
	return materialType.constructionType.byproducts;
}
uint32_t ConstructProject::getWorkerConstructScore(Actor& actor) const
{
	return (actor.m_strength * Config::constructStrengthModifier) + (actor.m_skillSet.get(SkillType::Construct) * Config::constructSkillModifier);
}
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
// If blockFeatureType is null then construct a wall rather then a feature.
void HasConstructDesignationsForPlayer::designate(Block& block, const BlockFeatureType* blockFeatureType)
{
	assert(!contains(&block));
	m_data.emplace(&block, block, blockFeatureType);
	block.m_hasDesignations.insert(m_player, BlockDesignation::Construct);
}
void HasConstructDesignationsForPlayer::remove(Block& block)
{
	assert(contains(&block));
	m_data.remove(&block); 
	block.m_hasDesignations.remove(m_player, BlockDesignation::Construct);
}
void HasConstructDesignationsForPlayer::removeIfExists(Block& block)
{
	if(m_data.contains(&block))
		remove(block);
	bool contains(Block& block) const { return m_data.contains(&block); }
	const BlockFeatureType* at(Block& block) const { return m_data.at(&block).blockFeatureType; }
	bool empty() const { return m_data.empty(); }
}
// To be used by Area.
void HasConstructDesignations::addPlayer(Player& player)
{
	assert(!m_data.contains(&player));
	m_data[&player](player);
}
void HasConstructDesignations::removePlayer(Player& player)
{
	assert(m_data.contains(player));
	m_data.remove(&player);
}
// If blockFeatureType is null then dig out fully rather then digging out a feature.
void HasConstructDesignations::designate(Player& player, Block& block, const BlockFeatureType* blockFeatureType)
{
	m_data.at(player).designate(block, blockFeatureType);
}
void HasConstructDesignations::remove(Player& player, Block& block)
{
	assert(m_data.contains(player));
	m_data.at(player).remove(block);
}
void HasConstructDesignations::clearAll(Block& block)
{
	for(auto& pair : m_data)
		pair.second.removeIfExists(block);
}
void HasConstructDesignations::areThereAnyForPlayer(Player& player) const
{
	return !m_data.at(player).empty();
}
