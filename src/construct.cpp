#include "construct.h"
#include "item.h"
#include "block.h"
#include "area.h"
#include "util.h"
ConstructThreadedTask::ConstructThreadedTask(ConstructObjective& co) : ThreadedTask(co.m_actor.getThreadedTaskEngine()), m_constructObjective(co), m_findsPath(co.m_actor, co.m_detour) { }
void ConstructThreadedTask::readStep()
{
	std::function<bool(const Block&)> constructCondition = [&](const Block& block)
	{
		return m_constructObjective.joinableProjectExistsAt(block);
	};
	m_findsPath.pathToUnreservedAdjacentToPredicate(constructCondition, *m_constructObjective.m_actor.getFaction());
}
void ConstructThreadedTask::writeStep()
{
	if(!m_findsPath.found())
		m_constructObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_constructObjective);
	else
	{
		// One or more destination blocks has been reserved since result was found, get a new one.
		if(!m_constructObjective.m_actor.allBlocksAtLocationAndFacingAreReservable(*m_findsPath.getPath().back(), m_findsPath.getFacingAtDestination()))
		{
			m_constructObjective.m_constructThreadedTask.create(m_constructObjective);
			return;
		}
		ConstructProject* project = m_constructObjective.getProjectWhichActorCanJoinAdjacentTo(*m_findsPath.getPath().back(), m_findsPath.getFacingAtDestination());
		if(project == nullptr)
		{
			// No longer any project to join at destination, try again.
			m_constructObjective.m_constructThreadedTask.create(m_constructObjective);
			return;
		}
		m_constructObjective.m_actor.m_canMove.setPath(m_findsPath.getPath());
		m_findsPath.reserveBlocksAtDestination(m_constructObjective.m_actor.m_canReserve);
	}
}
void ConstructThreadedTask::clearReferences() { m_constructObjective.m_constructThreadedTask.clearPointer(); }
void ConstructObjective::execute()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return joinableProjectExistsAt(block); };
	Block* block = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
	if(block == nullptr)
		m_constructThreadedTask.create(*this);
	else
	{
		m_project = getProjectWhichActorCanJoinAt(*block);
		m_project->addWorker(m_actor, *this);
	}
}
void ConstructObjective::cancel()
{
	if(m_project != nullptr)
		m_project->removeWorker(m_actor);
	m_constructThreadedTask.maybeCancel();
}
void ConstructObjective::reset() 
{ 
	cancel(); 
	m_project = nullptr; 
	m_actor.m_canReserve.clearAll();
}
ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAdjacentTo(const Block& location, Facing facing)
{
	for(Block* adjacent : m_actor.getAdjacentAtLocationWithFacing(location, facing))
	{
		ConstructProject* project = getProjectWhichActorCanJoinAt(*adjacent);
		if(project != nullptr)
			return project;
	}
	return nullptr;
}
ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAt(Block& block)
{
	if(block.m_hasDesignations.contains(*m_actor.getFaction(), BlockDesignation::Construct))
	{
		ConstructProject& project = block.m_area->m_hasConstructionDesignations.getProject(*m_actor.getFaction(), block);
		if(project.canAddWorker(m_actor))
			return &project;
	}
	return nullptr;
}
bool ConstructObjective::joinableProjectExistsAt(const Block& block) const
{
	return const_cast<ConstructObjective*>(this)->getProjectWhichActorCanJoinAt(const_cast<Block&>(block)) != nullptr;
}
bool ConstructObjective::canJoinProjectAdjacentToLocationAndFacing(const Block& location, Facing facing) const
{
	return const_cast<ConstructObjective*>(this)->getProjectWhichActorCanJoinAdjacentTo(location, facing) != nullptr;
}
bool ConstructObjectiveType::canBeAssigned(Actor& actor) const
{
	return !actor.m_location->m_area->m_hasConstructionDesignations.areThereAnyForFaction(*actor.getFaction());
}
std::unique_ptr<Objective> ConstructObjectiveType::makeFor(Actor& actor) const { return std::make_unique<ConstructObjective>(actor); }
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
	m_data.try_emplace(&block, &m_faction, block, blockFeatureType, materialType);
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
bool HasConstructionDesignations::areThereAnyForFaction(const Faction& faction) const { return !m_data.at(&faction).empty(); }
ConstructProject& HasConstructionDesignations::getProject(const Faction& faction, Block& block) { return m_data.at(&faction).m_data.at(&block); }
