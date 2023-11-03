#include "craft.h"
#include "area.h"
#include "util.h"
Step CraftStepProject::getDuration() const
{
	uint32_t totalScore = 0;
	for(auto& pair : m_workers)
		totalScore += getWorkerCraftScore(*pair.first);
	return m_craftStepType.stepsDuration / totalScore;
}
uint32_t CraftStepProject::getWorkerCraftScore(const Actor& actor) const
{
	return actor.m_skillSet.get(m_craftStepType.skillType);
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
		if(pair.first.m_materialType == nullptr)
		{
			assert(pair.first.m_item == nullptr && (pair.first.m_materialTypeCategory == nullptr || pair.first.m_materialTypeCategory == m_craftJob.materialType->materialTypeCategory));
			pair.first.specalize(*m_craftJob.materialType);
		}
	return output;
}
std::vector<std::pair<ItemQuery, uint32_t>> CraftStepProject::getUnconsumed() const 
{ 
	auto output = m_craftStepType.unconsumedItems; 
	if(m_craftJob.workPiece != nullptr)
		output.emplace_back(*m_craftJob.workPiece, 1);
	return output; 
}
std::vector<std::pair<ActorQuery, uint32_t>> CraftStepProject::getActors() const { return {}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> CraftStepProject::getByproducts() const {
	auto output =  m_craftStepType.byproductItems; 
	for(auto& tuple : output)
		if(std::get<1>(tuple) == nullptr)
			std::get<1>(tuple) = m_craftJob.materialType;
	return output; 
}
uint32_t CraftJob::getQuality() const
{
	float pointsPerStep = totalSkillPoints / craftJobType.stepTypes.size();
	return (pointsPerStep / Config::maxSkillLevel) * 100;
}
void CraftThreadedTask::readStep()
{
	auto pair = m_craftObjective.m_actor.m_location->m_area->m_hasCraftingLocationsAndJobs.getJobAndLocationFor(m_craftObjective.m_actor, m_craftObjective.m_skillType);
	m_craftJob = pair.first;
	m_location = pair.second;
}
void CraftThreadedTask::writeStep()
{
	if(m_craftJob  == nullptr)
		m_craftObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_craftObjective);
	else
		// Selected work place has been reserved, try again.
		if(m_location->m_reservable.isFullyReserved(m_craftObjective.m_actor.getFaction()))
			m_craftObjective.m_threadedTask.create(m_craftObjective);
		else
		{
			m_craftObjective.m_actor.m_location->m_area->m_hasCraftingLocationsAndJobs.makeAndAssignStepProject(*m_craftJob, *m_location, m_craftObjective);
			m_craftObjective.m_craftJob = m_craftJob;
		}
}
void CraftThreadedTask::clearReferences(){ m_craftObjective.m_threadedTask.clearPointer(); }
CraftObjective::CraftObjective(Actor& a, const SkillType& st) : Objective(Config::craftObjectivePriority), m_actor(a), m_skillType(st), m_craftJob(nullptr), m_threadedTask(a.getThreadedTaskEngine()) { }
bool CraftObjectiveType::canBeAssigned(Actor& actor) const
{
	auto& hasCraftingLocationsAndJobs = actor.m_location->m_area->m_hasCraftingLocationsAndJobs;
	// No jobs needing this skill.
	if(!hasCraftingLocationsAndJobs.m_unassignedProjectsBySkill.contains(&m_skillType))
		return false;
	for(CraftJob* craftJob : hasCraftingLocationsAndJobs.m_unassignedProjectsBySkill.at(&m_skillType))
	{
		auto category = (*craftJob->stepIterator)->craftStepTypeCategory;
		// No location to do the craftStepTypeCategory work at.
		if(!hasCraftingLocationsAndJobs.m_locationsByCategory.contains(&category))
			return false;
	}
	return false;
}
std::unique_ptr<Objective> CraftObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<CraftObjective>(actor, m_skillType);
}
void CraftObjective::execute(){ m_threadedTask.create(*this); }
void CraftObjective::cancel()
{
	if(m_craftJob != nullptr)
		m_actor.m_location->m_area->m_hasCraftingLocationsAndJobs.stepInterupted(*m_craftJob);
	m_threadedTask.maybeCancel();
}
void CraftObjective::reset() 
{ 
	cancel(); 
	m_craftJob = nullptr;
	m_actor.m_canReserve.clearAll();
}
void HasCraftingLocationsAndJobs::addLocation(std::vector<const CraftStepType*>& craftStepTypes, Block& block)
{
	for(auto* craftStepType : craftStepTypes)
		m_locationsByCategory[&craftStepType->craftStepTypeCategory].insert(&block);
}
void HasCraftingLocationsAndJobs::removeLocation(std::vector<const CraftStepType*>& craftStepTypes, Block& block)
{
	for(const CraftStepType* craftStepType : craftStepTypes)
	{
		auto category = &craftStepType->craftStepTypeCategory;
		assert(m_locationsByCategory.contains(category));
		if(m_locationsByCategory.at(category).size() == 1)
			m_locationsByCategory.erase(category);
		else
			m_locationsByCategory.at(category).erase(&block);
	}
}
// To be used by the UI.
bool HasCraftingLocationsAndJobs::hasLocationsFor(const CraftJobType& craftJobType) const
{
	for(auto* craftStepType : craftJobType.stepTypes)
		if(!m_locationsByCategory.contains(&craftStepType->craftStepTypeCategory))
			return false;
	return true;
}
// Material type may be null.
void HasCraftingLocationsAndJobs::addJob(const CraftJobType& craftJobType, const MaterialType* materialType, uint32_t minimumSkillLevel)
{
	CraftJob& craftJob = m_jobs.emplace_back(craftJobType, *this, materialType, minimumSkillLevel);
	indexUnassigned(craftJob);
}
void HasCraftingLocationsAndJobs::stepComplete(CraftJob& craftJob, Actor& actor)
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
void HasCraftingLocationsAndJobs::stepInterupted(CraftJob& craftJob)
{
	craftJob.craftStepProject->cancel();
	craftJob.craftStepProject = nullptr;
	indexUnassigned(craftJob);
}
void HasCraftingLocationsAndJobs::indexUnassigned(CraftJob& craftJob)
{
	const CraftStepType& craftStepType = **craftJob.stepIterator;
	m_unassignedProjectsByStepTypeCategory[&craftStepType.craftStepTypeCategory].insert(&craftJob);
	m_unassignedProjectsBySkill[&craftStepType.skillType].insert(&craftJob);
}
void HasCraftingLocationsAndJobs::unindexAssigned(CraftJob& craftJob)
{
	const CraftStepType& craftStepType = **craftJob.stepIterator;
	if(m_unassignedProjectsByStepTypeCategory[&craftStepType.craftStepTypeCategory].size() == 1)
		m_unassignedProjectsByStepTypeCategory.erase(&craftStepType.craftStepTypeCategory);
	else
		m_unassignedProjectsByStepTypeCategory[&craftStepType.craftStepTypeCategory].erase(&craftJob);
	if(m_unassignedProjectsBySkill[&craftStepType.skillType].size() == 1)
		m_unassignedProjectsBySkill.erase(&craftStepType.skillType);
	else
		m_unassignedProjectsBySkill[&craftStepType.skillType].erase(&craftJob);
}
void HasCraftingLocationsAndJobs::jobComplete(CraftJob& craftJob)
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
void HasCraftingLocationsAndJobs::makeAndAssignStepProject(CraftJob& craftJob, Block& location, CraftObjective& objective)
{
	craftJob.craftStepProject = std::make_unique<CraftStepProject>(objective.m_actor.getFaction(), location, **craftJob.stepIterator, craftJob);
	craftJob.craftStepProject->addWorkerCandidate(objective.m_actor, objective);
	unindexAssigned(craftJob);
}
// May return nullptr;
CraftJob* HasCraftingLocationsAndJobs::getJobForAtLocation(const Actor& actor, const SkillType& skillType, const Block& block)
{
	assert(!block.m_reservable.isFullyReserved(actor.getFaction()));
	if(!m_stepTypeCategoriesByLocation.contains(const_cast<Block*>(&block)))
		return nullptr;
	for(const CraftStepTypeCategory* category : m_stepTypeCategoriesByLocation.at(const_cast<Block*>(&block)))
		for(CraftJob* craftJob : m_unassignedProjectsByStepTypeCategory.at(category))
			if(&(*craftJob->stepIterator)->skillType == &skillType && actor.m_skillSet.get(skillType) >= craftJob->minimumSkillLevel)
				return craftJob;
	return nullptr;
}
std::pair<CraftJob*, Block*> HasCraftingLocationsAndJobs::getJobAndLocationFor(const Actor& actor, const SkillType& skillType)
{
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		return !block.m_reservable.isFullyReserved(actor.getFaction()) && getJobForAtLocation(actor, skillType, block) != nullptr;
	};
	FindsPath findsPath(actor, false);
	findsPath.pathToUnreservedAdjacentToPredicate(predicate, *actor.getFaction());
	if(!findsPath.found())
		return std::make_pair(nullptr, nullptr);
	else
	{
		auto& block = *findsPath.getBlockWhichPassedPredicate();
		return std::make_pair(getJobForAtLocation(actor, skillType, block), &block);
	}
}
