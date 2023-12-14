#include "craft.h"
#include "area.h"
#include "util.h"
#include <memory>
Step CraftStepProject::getDuration() const
{
	uint32_t totalScore = 0;
	for(auto& pair : m_workers)
		totalScore += getWorkerCraftScore(*pair.first);
	return m_craftStepType.stepsDuration / totalScore;
}
// Static method.
uint32_t CraftStepProject::getWorkerCraftScore(const Actor& actor) const
{
	return 1 + actor.m_skillSet.get(m_craftStepType.skillType);
}
void CraftStepProject::onComplete()
{
	auto& [actor, projectWorker] = *m_workers.begin();
	Objective& objective = projectWorker.objective;
	Actor* actorPtr = actor;
	m_craftJob.hasCraftingLocationsAndJobs.stepComplete(m_craftJob, *actor);
	actorPtr->m_hasObjectives.objectiveComplete(objective);
}
void CraftStepProject::onCancel()
{
	std::vector<Actor*> actors = getWorkersAndCandidates();
	CraftJob& craftJob = m_craftJob;
	m_location.m_area->m_hasCraftingLocationsAndJobs.at(m_faction).stepDestroy(craftJob);
	for(Actor* actor : actors)
	{
		actor->m_project = nullptr;
		CraftObjective& objective = static_cast<CraftObjective&>(actor->m_hasObjectives.getCurrent());
		//TODO: This records this fail job for this actor regardless of if it might still be possible via another location or subsitute required hasShape.
		objective.recordFailedJob(craftJob);
		objective.reset();
		actor->m_hasObjectives.cannotCompleteTask();
	}
}
std::vector<std::pair<ItemQuery, uint32_t>> CraftStepProject::getConsumed() const
{ 
	// Make a copy so we can edit itemQueries.
	auto output = m_craftStepType.consumed;
	for(auto& pair : output)
		if(pair.first.m_materialType == nullptr)
		{
			assert(pair.first.m_item == nullptr && (pair.first.m_materialTypeCategory == nullptr || pair.first.m_materialTypeCategory == m_craftJob.materialType->materialTypeCategory));
			pair.first.specalize(*m_craftJob.materialType);
		}
	return output;
}
std::vector<std::pair<ItemQuery, uint32_t>> CraftStepProject::getUnconsumed() const 
{ 
	auto output = m_craftStepType.unconsumed; 
	if(m_craftJob.workPiece != nullptr)
		output.emplace_back(*m_craftJob.workPiece, 1);
	return output; 
}
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> CraftStepProject::getByproducts() const {
	auto output =  m_craftStepType.byproducts; 
	for(auto& tuple : output)
		if(std::get<1>(tuple) == nullptr)
			std::get<1>(tuple) = m_craftJob.materialType;
	return output; 
}
uint32_t CraftJob::getQuality() const
{
	float pointsPerStep = (float)totalSkillPoints / craftJobType.stepTypes.size();
	return (pointsPerStep / Config::maxSkillLevel) * 100;
}
uint32_t CraftJob::getStep() const
{
	return 1 + (stepIterator - craftJobType.stepTypes.begin());
}
void CraftThreadedTask::readStep()
{
	assert(m_craftObjective.m_craftJob == nullptr);
	const Faction& faction = *m_craftObjective.m_actor.getFaction();
	HasCraftingLocationsAndJobsForFaction& hasCrafting = m_craftObjective.m_actor.m_location->m_area->m_hasCraftingLocationsAndJobs.at(faction);
	auto pair = hasCrafting.getJobAndLocationForWhileExcluding(m_craftObjective.m_actor, m_craftObjective.m_skillType, m_craftObjective.getFailedJobs());
	m_craftJob = pair.first;
	m_location = pair.second;
}
void CraftThreadedTask::writeStep()
{
	if(m_craftJob  == nullptr)
		m_craftObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_craftObjective);
	else
		// Selected work loctaion has been reserved, try again.
		if(m_location->m_reservable.isFullyReserved(m_craftObjective.m_actor.getFaction()))
			m_craftObjective.m_threadedTask.create(m_craftObjective);
		else
		{
			const Faction& faction = *m_craftObjective.m_actor.getFaction();
			HasCraftingLocationsAndJobsForFaction& hasCrafting = m_craftObjective.m_actor.m_location->m_area->m_hasCraftingLocationsAndJobs.at(faction);
			hasCrafting.makeAndAssignStepProject(*m_craftJob, *m_location, m_craftObjective);
			m_craftObjective.m_craftJob = m_craftJob;
		}
}
void CraftThreadedTask::clearReferences(){ m_craftObjective.m_threadedTask.clearPointer(); }
CraftObjective::CraftObjective(Actor& a, const SkillType& st) : Objective(Config::craftObjectivePriority), m_actor(a), m_skillType(st), m_craftJob(nullptr), m_threadedTask(a.getThreadedTaskEngine()) { }
bool CraftObjectiveType::canBeAssigned(Actor& actor) const
{
	auto& hasCrafting = actor.m_location->m_area->m_hasCraftingLocationsAndJobs.at(*actor.getFaction());
	if(!hasCrafting.m_unassignedProjectsBySkill.contains(&m_skillType))
	{
		// No jobs needing this skill.
		return false;
	}
	// Check if there are any locations designated for this step category.
	for(CraftJob* craftJob : hasCrafting.m_unassignedProjectsBySkill.at(&m_skillType))
	{
		const CraftStepTypeCategory& category = craftJob->stepIterator->craftStepTypeCategory;
		if(hasCrafting.m_locationsByCategory.contains(&category))
			return true;
	}
	return false;
}
std::unique_ptr<Objective> CraftObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<CraftObjective>(actor, m_skillType);
}
void CraftObjective::execute()
{ 
	if(m_craftJob == nullptr)
		m_threadedTask.create(*this); 
	else
		m_craftJob->craftStepProject->commandWorker(m_actor);
}
void CraftObjective::cancel()
{
	if(m_craftJob != nullptr && m_craftJob->craftStepProject)
		m_craftJob->craftStepProject->removeWorker(m_actor);
	m_threadedTask.maybeCancel();
	m_actor.m_canReserve.clearAll();
}
void CraftObjective::reset() 
{ 
	cancel(); 
	m_craftJob = nullptr;
}
void HasCraftingLocationsAndJobsForFaction::addLocation(const CraftStepTypeCategory& category, Block& block)
{
	m_locationsByCategory[&category].insert(&block);
	m_stepTypeCategoriesByLocation[&block].insert(&category);
}
void HasCraftingLocationsAndJobsForFaction::removeLocation(const CraftStepTypeCategory& category, Block& block)
{
	assert(m_locationsByCategory.contains(&category));
	assert(m_stepTypeCategoriesByLocation.contains(&block));
	if(m_locationsByCategory.at(&category).size() == 1)
		m_locationsByCategory.erase(&category);
	else
		m_locationsByCategory.at(&category).erase(&block);
	if(m_stepTypeCategoriesByLocation.at(&block).size() == 0)
		m_stepTypeCategoriesByLocation.erase(&block);
	else
		m_stepTypeCategoriesByLocation.at(&block).erase(&category);
	std::vector<CraftJob*> jobsWithStepsToCancel;
	for(CraftJob& craftJob : m_jobs)
		if(craftJob.craftStepProject && craftJob.craftStepProject->getLocation() == block)
			jobsWithStepsToCancel.push_back(&craftJob);
	for(CraftJob* craftJob : jobsWithStepsToCancel)
		craftJob->craftStepProject->cancel();
}
void HasCraftingLocationsAndJobsForFaction::maybeRemoveLocation(Block& block)
{
	auto categories = m_stepTypeCategoriesByLocation.find(&block);
	if(categories == m_stepTypeCategoriesByLocation.end())
		return;
	auto categoriesCopy = categories->second;
	for(const CraftStepTypeCategory* category : categoriesCopy)
		removeLocation(*category, block);
}
// To be used by the UI.
bool HasCraftingLocationsAndJobsForFaction::hasLocationsFor(const CraftJobType& craftJobType) const
{
	for(auto& craftStepType : craftJobType.stepTypes)
		if(!m_locationsByCategory.contains(&craftStepType.craftStepTypeCategory))
			return false;
	return true;
}
// Material type may be null.
void HasCraftingLocationsAndJobsForFaction::addJob(const CraftJobType& craftJobType, const MaterialType* materialType, uint32_t minimumSkillLevel)
{
	if(craftJobType.materialtypeCategory != nullptr)
		assert(materialType->materialTypeCategory == craftJobType.materialtypeCategory);
	CraftJob& craftJob = m_jobs.emplace_back(craftJobType, *this, materialType, minimumSkillLevel);
	indexUnassigned(craftJob);
}
void HasCraftingLocationsAndJobsForFaction::stepComplete(CraftJob& craftJob, Actor& actor)
{
	craftJob.totalSkillPoints += craftJob.craftStepProject->getWorkerCraftScore(actor);
	Block* location = &craftJob.craftStepProject->getLocation();
	unindexAssigned(craftJob);
	++craftJob.stepIterator;
	if(craftJob.stepIterator == craftJob.craftJobType.stepTypes.end())
		jobComplete(craftJob);
	else
	{
		if(craftJob.workPiece == nullptr)
		{
			// ItemType, MaterialType, quality, wear, craftJob.
			Item& item = location->m_area->m_simulation.createItem(craftJob.craftJobType.productType, *craftJob.materialType, "", 0, 0, &craftJob);
			//TODO: Should the item be reserved?
			location->m_hasItems.add(item);
			craftJob.workPiece = &item;
		}
		indexUnassigned(craftJob);
	}
}
void HasCraftingLocationsAndJobsForFaction::stepDestroy(CraftJob& craftJob)
{
	assert(craftJob.craftStepProject);
	craftJob.craftStepProject = nullptr;
	indexUnassigned(craftJob);
}
void HasCraftingLocationsAndJobsForFaction::indexUnassigned(CraftJob& craftJob)
{
	const CraftStepType& craftStepType = *craftJob.stepIterator;
	m_unassignedProjectsByStepTypeCategory[&craftStepType.craftStepTypeCategory].insert(&craftJob);
	m_unassignedProjectsBySkill[&craftStepType.skillType].insert(&craftJob);
}
void HasCraftingLocationsAndJobsForFaction::unindexAssigned(CraftJob& craftJob)
{
	const CraftStepType& craftStepType = *craftJob.stepIterator;
	if(m_unassignedProjectsByStepTypeCategory[&craftStepType.craftStepTypeCategory].size() == 1)
		m_unassignedProjectsByStepTypeCategory.erase(&craftStepType.craftStepTypeCategory);
	else
		m_unassignedProjectsByStepTypeCategory[&craftStepType.craftStepTypeCategory].erase(&craftJob);
	if(m_unassignedProjectsBySkill[&craftStepType.skillType].size() == 1)
		m_unassignedProjectsBySkill.erase(&craftStepType.skillType);
	else
		m_unassignedProjectsBySkill[&craftStepType.skillType].erase(&craftJob);
}
void HasCraftingLocationsAndJobsForFaction::jobComplete(CraftJob& craftJob)
{
	assert(craftJob.workPiece != nullptr);
	if(craftJob.craftJobType.productType.generic)
	{
		craftJob.workPiece->m_location->m_hasItems.add(craftJob.craftJobType.productType, *craftJob.materialType, craftJob.craftJobType.productQuantity);
		craftJob.workPiece->destroy();
	}
	else
	{
		assert(craftJob.craftJobType.productQuantity == 1);
		craftJob.workPiece->m_quality = craftJob.getQuality();
		craftJob.workPiece->m_craftJobForWorkPiece = nullptr;
	}
	m_jobs.remove(craftJob);
}
void HasCraftingLocationsAndJobsForFaction::makeAndAssignStepProject(CraftJob& craftJob, Block& location, CraftObjective& objective)
{
	craftJob.craftStepProject = std::make_unique<CraftStepProject>(objective.m_actor.getFaction(), location, *craftJob.stepIterator, craftJob);
	std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<CraftStepProjectHasShapeDishonorCallback>(*craftJob.craftStepProject.get());
	craftJob.craftStepProject->setLocationDishonorCallback(std::move(dishonorCallback));
	craftJob.craftStepProject->addWorkerCandidate(objective.m_actor, objective);
	unindexAssigned(craftJob);
}
// May return nullptr;
CraftJob* HasCraftingLocationsAndJobsForFaction::getJobForAtLocation(const Actor& actor, const SkillType& skillType, const Block& block, std::unordered_set<CraftJob*>& excludeJobs)
{
	assert(!block.m_reservable.isFullyReserved(actor.getFaction()));
	if(!m_stepTypeCategoriesByLocation.contains(const_cast<Block*>(&block)))
		return nullptr;
	for(const CraftStepTypeCategory* category : m_stepTypeCategoriesByLocation.at(const_cast<Block*>(&block)))
		if(m_unassignedProjectsByStepTypeCategory.contains(category))
			for(CraftJob* craftJob : m_unassignedProjectsByStepTypeCategory.at(category))
				if(!excludeJobs.contains(craftJob) && craftJob->stepIterator->skillType == skillType && actor.m_skillSet.get(skillType) >= craftJob->minimumSkillLevel)
					return craftJob;
	return nullptr;
}
std::pair<CraftJob*, Block*> HasCraftingLocationsAndJobsForFaction::getJobAndLocationForWhileExcluding(const Actor& actor, const SkillType& skillType, std::unordered_set<CraftJob*>& excludeJobs)
{
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		return !block.m_reservable.isFullyReserved(actor.getFaction()) && getJobForAtLocation(actor, skillType, block, excludeJobs) != nullptr;
	};
	FindsPath findsPath(actor, false);
	findsPath.pathToUnreservedAdjacentToPredicate(predicate, *actor.getFaction());
	if(!findsPath.found() && !findsPath.m_useCurrentLocation)
		return std::make_pair(nullptr, nullptr);
	else
	{
		auto& block = *findsPath.getBlockWhichPassedPredicate();
		return std::make_pair(getJobForAtLocation(actor, skillType, block, excludeJobs), &block);
	}
}
