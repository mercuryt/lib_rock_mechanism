#include "craft.h"
uint32_t CraftStepProject::getDelay() const
{
	uint32_t totalScore = 0;
	for(auto& pair : m_workers)
		totalScore += getWorkerCraftScore(*pair.first);
	return m_craftStepType.stepsDuration / totalScore;
}
uint32_t CraftStepProject::getWorkerCraftScore(const Actor& actor) const
{
	uint32_t output = 0;
	for(auto& [skillType, percentWeight] : m_craftStepType.skillsAndPercentWeights)
		output += util::scaleByPercent(actor.m_skillSet.get(*skillType), percentWeight);
	return output;
}
void CraftStepProject::onComplete()
{
	assert(m_workers.size() == 1);
	Actor* actor;
	for(auto& pair : m_workers)
		actor = pair.first;
	m_craftJob.hasCraftingLocationsAndJobs.stepComplete(m_craftJob, *actor);
}
std::vector<std::pair<ItemQuery, uint32_t>> CraftStepProject::getConsumed() const
{ 
	// Make a copy so we can edit itemQueries.
	auto output = m_craftStepType.consumedItems;
	for(auto& pair : output)
		if(pair.first.primaryMaterial)
		{
			assert(pair.first.materialType == nullptr && pair.first.item == nullptr && 
					(pair.first.materialTypeCategory == nullptr || pair.first.materialTypeCategory == m_craftJob.materialType.materialTypeCategory));
			pair.first.specalize(craftJob.materialType);
		}
	return output;
}
std::vector<std::pair<ItemQuery, uint32_t>> CraftStepProject::getUnconsumed() const 
{ 
	auto output = m_craftStepType.unconsumedItems; 
	if(craftJob.workPiece != nullptr)
		output.emplace({craftJob.workPiece}, 1);
	return output; 
}
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> CraftStepProject::getByproducts() const {
	auto output =  m_craftStepType.byproductItems; 
	for(auto& tuple : output)
		if(tuple.at(1) == nullptr)
			tuple.at(1) = m_craftJob.materialType;
	return output; 
}
void CraftThreadedTask::readStep()
{
	[m_craftJob, m_location] = m_craftObjective.m_actor.m_location->m_hasCraftingLocations.getJobAndLocationFor(m_craftObjective.m_actor, m_skillType);
}
void CraftThreadedTask::writeStep()
{
	if(m_craftJob  == nullptr)
		m_craftObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_craftObjective);
	else
		// Selected work place has been reserved, try again.
		if(m_location.m_reservable.isFullyReserved())
			m_craftObjective.m_threadedTask.create(m_craftObjective);
		else
			m_craftObjective.m_actor.m_location->m_area->m_hasCraftingLocations.makeAndAssignStepProject(*m_craftJob, *m_location, m_craftObjective.m_actor);
}
bool CraftObjectiveType::canBeAssigned(Actor& actor)
{
	auto& hasCraftingLocationsAndJobss = actor.m_location->m_area->m_hasCraftingLocationsAndJobs;
	// No jobs needing this skill.
	if(!hasCraftingLocationsAndJobss.m_unassignedProjectsBySkill.contains(&m_skillType))
		return false;
	for(CraftJob* craftJob : hasCraftingLocationsAndJobss.m_unassignedProjectsBySkill.at(&m_skillType))
	{
		auto category = craftJob->stepIterator->craftStepTypeCategory;
		// No location to do the craftStepTypeCategory work at.
		if(!hasCraftingLocationsAndJobs.m_locationsByCategory.contains(category))
			return false;
	}
	return false;
}
std::unique_ptr<Objective> CraftObjectiveType::makeFor(Actor& actor)
{

	return std::make_unique<CraftObjective>(actor, m_skillType);
}
void CraftObjective::execute(){ m_threadedTask.create(*this, m_skillType); }
void HasCraftingLocationsAndJobs::addLocation(std::vector<const CraftStepType*>& craftStepTypes, Block& block)
{
	for(auto* craftStepType : craftStepTypes)
		m_locationsByCategory[craftStepType].insert(&block);
}
void HasCraftingLocationsAndJobs::removeLocation(std::vector<const CraftStepType*>& craftStepTypes, Block& block)
{
	for(const CraftStepType* craftStepType : craftStepTypes)
	{
		assert(m_locationsByCategory.contains(craftStepType));
		if(m_locationsByCategory.at(craftStepType).size() == 1)
			m_locationsByCategory.remove(craftStepType);
		else
			m_locationsByCategory.at(craftStepType).remove(&block);
	}
}
// To be used by the UI.
bool HasCraftingLocationsAndJobs::hasLocationsFor(const CraftType& craftType) const
{
	for(auto* craftStepType : craftType.craftStepTypes)
		if(!m_locationsByCategory.contains(craftStepType))
			return false;
	return true;
}
void HasCraftingLocationsAndJobs::addJob(CraftJobType& craftJobType)
{
	CraftJob& craftJob = m_jobs.emplace_back(craftJobType);
	indexUnassigned(craftJob);
}
void HasCraftingLocationsAndJobs::stepComplete(CraftJob& craftJob, Actor& actor)
{
	craftJob.totalSkillPoints += craftJob.stepIterator->getWorkerCraftScore(actor);
	Block* location = &craftJob.stepIterator->m_loction;
	unindexAssigned(craftJob);
	++craftJob.stepIterator;
	if(craftJob.stepIterator == craftJob.steps.end())
		jobComplete(craftJob);
	else
	{
		if(craftJob.workPiece == nullptr)
		{
			// Area, ItemType, MaterialType, quality, wear.
			Item& item = Item::create(location->m_area, ItemType::workPiece, craftJob.m_materialType, 0, 0);
			//TODO: Should the item be reserved?
			location->m_hasItems.add(item);
			craftJob.workPiece = &item;
		}
		indexUnassigned(craftJob);
	}
}
void HasCraftingLocationsAndJobs::indexUnassigned(CraftJob& craftJob)
{
	CraftStepType& craftStepType = craftJob.stepIterator->craftStepType;
	m_unassignedProjectsByStepTypeCategory[craftStepType.craftStepTypeCategory].insert(&craftJob);
	m_unassignedProjectsBySkill[craftStepType.skillType].insert(&craftJob);
}
void HasCraftingLocationsAndJobs::unindexAssigned(CraftJob& craftJob)
{
	CraftStepType& craftStepType = craftJob.stepIterator->craftStepType;
	if(m_unassignedProjectsByStepTypeCategory[craftStepType.craftStepTypeCategory].size() == 1)
		m_unassignedProjectsByStepTypeCategory.erase(craftStepType.craftStepTypeCategory);
	else
		m_unassignedProjectsByStepTypeCategory[craftStepType.craftStepTypeCategory].erase(&craftJob);
	if(m_unassignedProjectsBySkill[craftStepType.skillType].size() == 1)
		m_unassignedProjectsBySkill.erase(craftStepType.skillType);
	else
		m_unassignedProjectsBySkill[craftStepType.skillType].erase(&craftJob);
}
void HasCraftingLocationsAndJobs::jobComplete(CraftJob& craftJob)
{
	uint32_t averageSkillPoints = craftJob.totalSkillPoints / craftJob.craftJobType.steps.size();
	for(auto& [itemType, mt, quantity] : craftJob.craftJobType.productItems)
	{
		MaterialType* materialType = mt != nullptr ? mt : craftJob.materialType;
		if(itemType.generic)
			craftJob.workPiece.m_location->m_hasItems.add(itemType, materialType, quantity);
		else
		{
			assert(quantity == 1);
			// Area, ItemType, MaterialType, quality, wear.
			Item& item = Item::create(craftJob.workPiece.m_location->m_area, itemType, materialType, quality, 0);
			craftJob.workPiece.m_location->m_hasItems.add(item);
		}
	}
	craftJob.workPiece->destroy();
	m_jobs.remove(craftJob);
}
void HasCraftingLocationsAndJobs::makeAndAssignStepProject(CraftJob& craftJob, Block& location, Actor& worker)
{
	craftJob.craftStepProject(location, craftJob.stepIterator, craftJob);
	craftJob.craftStepProject.addWorker(worker);
	auto& category = craftJob.stepIterator->craftStepType.craftStepTypeCategory;
	unindexAssigned(craftJob);
}
// May return nullptr;
CraftJob* HasCraftingLocationsAndJobs::getJobForAtLocation(Actor& actor, const SkillType& skillType, Block& block) const
{
	assert(!block.m_reservable.isFullyReserved());
	if(!m_stepTypeCategoriesByLocation.contains(&block))
		return nullptr;
	for(CraftStepTypeCategory* category : m_stepTypeCategoriesByLocation.at(&block))
		for(CraftJob& craftJob : m_unassignedProjectsByStepTypeCategory.at(category))
			if(craftJob.stepIterator->skillType == skillType && actor.m_skillSet.get(skillType) >= craftJob.minimumSkillLevel)
				return &craftJob;
	return nullptr;
}
std::pair<CraftJob*, Block*> HasCraftingLocationsAndJobs::getJobAndLocationFor(Actor& actor, const SkillType& skillType)
{
	auto condition = [&](Block* block)
	{
		return !block->m_reservable.isFullyReserved() && getJobForAtLocation(actor, skillType, *block) != nullptr;
	}
	Block* location = path::getForActorToPredicateReturnEndOnly(actor, condition);
	if(location == nullptr)
		return std::make_pair(nullptr, nullptr);
	else
		return std::make_pair(getJobForAtLocation(actor, skillType, *location), location);
}
