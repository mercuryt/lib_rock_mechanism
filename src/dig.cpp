void DigThreadedTask::readStep()
{
	auto destinationCondition = [&](Block* block)
	{
		return m_digObjective.canDigAt(*block);
	}
	m_result = path::getForActorToPredicate(m_digObjective.m_actor, destinationCondition)
}
void DigThreadedTask::writeStep()
{
	if(m_result.empty())
		m_digObjective.m_actor.m_hasObjectives.cannotFulfillObjective();
	else
	{
		// Destination block has been reserved since result was found, get a new one.
		if(m_result.back()->reservable.isFullyReserved())
		{
			m_digObjective.m_digThrededTask.create(m_digObjective);
			return;
		}
		m_digObjective.m_actor.setPath(m_result);
		m_result.back()->reservable.reserveFor(m_digObjective.m_actor.m_canReserve);
	}
}
void DigObjective::execute()
{
	if(m_project != nullptr)
	{
		m_project.commandWorker(m_actor);
		return;
	}
	if(canDigAt(*m_actor.m_location))
	{
		for(Block* adjacent : m_actor.m_location->getAdjacentWithEdgeAdjacent())
			if(!adjacent.m_reservable.isFullyReserved() && m_actor.m_location->m_area->m_digDesignations.contains(*adjacent))
			{
				m_project = m_actor.m_location->m_area->m_digDesignations.at(*adjacent);
				m_project.addWorker(m_actor);
			}
	}
	else
		m_digThrededTask.create(*this);
}
bool DigObjective::canDigAt(Block& block) const
{
	if(block->m_reservable.isFullyReserved())
		return false;
	for(Block* adjacent : block.getAdjacentWithEdgeAndCornerAdjacent())
		if(adjacent.m_hasDesignations.contains(*m_actor.m_player, BlockDesignation::Dig))
			return true;
	return false;
}
bool DigObjectiveType::canBeAssigned(Actor& actor)
{
	return actor.m_location->m_area->m_digDesignations.areThereAnyForPlayer(m_actor.getPlayer());
}
std::unique_ptr<Objective> DigObjectiveType::makeFor(Actor& actor)
{
	return std::make_unique<DigObjective>(actor);
}
std::vector<std::pair<ItemQuery, uint32_t>> DigProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, uint32_t>> DigProject::getUnconsumed() const
{
	return {{ItemQuery::canDig, 1}}; 
}
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t quantity>> DigProject::getByproducts() const
{
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> output;
	for(SpoilData& spoilData : m_location.getSolidMaterial().spoilData)
	{
		if(!randomUtil::chance(spoilData.chance))
			continue;
		uint32_t quantity = randomUtil::getInRange(spoilData.min, spoilData.max);
		m_location.m_hasItems.add(spoilData.itemType, spoilData.materialType, quantity);
	}
	return output;
}
static uint32_t DigProject::getWorkerDigScore(Actor& actor) const
{
	return (actor.m_strength * Config::digStrengthModifier) + (actor.m_skillSet.get(SkillType::Dig) * Config::digSkillModifier);
}
void DigProject::onComplete()
{
	const MaterialType& materialType = m_location.getSolidMaterial();
	if(blockFeatureType == nullptr)
		m_location.setNotSolid();
	else
		m_location.m_hasBlockFeatures.hue(blockFeatureType);
	// Remove designations for other players as well as owning player.
	m_block.m_location->m_area->m_hasDigDesignations.clearAll(m_block);

}
// What would the total delay time be if we started from scratch now with current workers?
uint32_t DigProject::getDelay() const
{
	uint32_t totalScore = 0;
	for(Actor* actor : m_workers)
		totalScore += getWorkerDigScore(*actor);
	return Config::digScoreCost / totalScore;
}
void HasDigDesignationsForPlayer::designate(Block& block, const BlockFeatureType* blockFeatureType)
{
	assert(!contains(&block));
	block.m_hasDesignations.insert(m_player, BlockDesignation::Dig);
	m_data.emplace(&block, block, blockFeatureType);
}
void HasDigDesignationsForPlayer::remove(Block& block)
{
	assert(contains(&block));
	block.m_hasDesignations.remove(m_player, BlockDesignation::Dig);
	m_data.remove(&block); 
}
void HasDigDesignationsForPlayer::removeIfExists(Block& block)
{
	if(m_data.contains(&block))
		remove(block);
}
const BlockFeatureType* HasDigDesignationsForPlayer::at(Block& block) const { return m_data.at(&block).blockFeatureType; }
bool HasDigDesignationsForPlayer::empty() const { return m_data.empty(); }
// To be used by Area.
void HasDigDesignations::addPlayer(Player& player)
{
	assert(!m_data.contains(&player));
	m_data[&player](player);
}
void HasDigDesignations::removePlayer(Player& player)
{
	assert(m_data.contains(player));
	m_data.remove(&player);
}
// If blockFeatureType is null then dig out fully rather then digging out a feature.
void HasDigDesignations::designate(Player& player, Block& block, const BlockFeatureType* blockFeatureType)
{
	m_data.at(player).designate(block, blockFeatureType);
}
void HasDigDesignations::remove(Player& player, Block& block)
{
	assert(m_data.contains(player));
	m_data.at(player).remove(block);
}
void HasDigDesignations::clearAll(Block& block)
{
	for(auto& pair : m_data)
		pair.second.removeIfExists(block);
}
void HasDigDesignations::areThereAnyForPlayer(Player& player) const
{
	return !m_data.at(player).empty();
}
