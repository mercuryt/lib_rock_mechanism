#include "craft.h"
#include "area.h"
#include "deserializationMemo.h"
#include "materialType.h"
#include "util.h"
#include <chrono>
#include <cstdint>
#include <memory>
void CraftInputAction::execute()
{
	m_area.m_hasCraftingLocationsAndJobs.at(m_faction).addJob(m_craftJobType, m_materialType, m_quantity);
}
void CraftCancelInputAction::execute()
{
	m_area.m_hasCraftingLocationsAndJobs.at(m_faction).removeJob(m_job);
}
CraftStepProject::CraftStepProject(const Json& data, DeserializationMemo& deserializationMemo, CraftJob& cj) : 
	Project(data, deserializationMemo), m_craftStepType(cj.craftStepProject->m_craftStepType), m_craftJob(cj) { }
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
CraftJob::CraftJob(const Json& data, DeserializationMemo& deserializationMemo, HasCraftingLocationsAndJobsForFaction& hclaj) :
	craftJobType(*data["craftJobType"].get<const CraftJobType*>()),
	hasCraftingLocationsAndJobs(hclaj),
	workPiece(data.contains("workPiece") ? &deserializationMemo.m_simulation.getItemById(data["workPiece"].get<ItemId>()) : nullptr),
	materialType(data.contains("materialType") ? &MaterialType::byName(data["materialType"].get<std::string>()) : nullptr),
	stepIterator(craftJobType.stepTypes.begin() + data["stepIndex"].get<size_t>()),
	craftStepProject(std::make_unique<CraftStepProject>(data["craftStepProject"], deserializationMemo, *this)),
	minimumSkillLevel(data["minimumSkillLevel"].get<uint32_t>()),
	totalSkillPoints(data["totalSkillPoints"].get<uint32_t>()), reservable(1) 
{ 
	deserializationMemo.m_craftJobs[data["address"].get<uintptr_t>()] = this;
}
Json CraftJob::toJson() const
{
	Json data{
		{"craftJobType", craftJobType},
		{"stepIndex", stepIterator - craftJobType.stepTypes.begin()},
		{"craftStepProejct", craftStepProject->toJson()},
		{"minimumSkillLevel", minimumSkillLevel},
		{"totalSkillPoints", totalSkillPoints}
	};
	if(workPiece != nullptr)
		data["workPiece"] = workPiece->m_id;
	if(materialType != nullptr)
		data["materialType"] = materialType->name;
	return data;
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
CraftThreadedTask::CraftThreadedTask(CraftObjective& co): ThreadedTask(co.m_actor.getThreadedTaskEngine()), m_craftObjective(co), m_craftJob(nullptr), m_location(nullptr) { }
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
// ObjectiveType.
CraftObjectiveType::CraftObjectiveType(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) : m_skillType(*data["skillType"].get<const SkillType*>()) { }
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
Json CraftObjectiveType::toJson() const 
{
	Json data = ObjectiveType::toJson();
	data["skillType"] = m_skillType;
	return data;
}
// Objective.
CraftObjective::CraftObjective(Actor& a, const SkillType& st) : Objective(a, Config::craftObjectivePriority), m_skillType(st), m_craftJob(nullptr), m_threadedTask(a.getThreadedTaskEngine()) { }
CraftObjective::CraftObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_skillType(*data["skillType"].get<const SkillType*>()), m_craftJob(deserializationMemo.m_craftJobs.at(data["craftJob"].get<uintptr_t>())), 
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine) 
{ 
	if(data.contains("failedJobs"))
		for(const Json& job : data["failedJobs"])
			m_failedJobs.insert(deserializationMemo.m_craftJobs.at(job.get<uintptr_t>()));
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
}
Json CraftObjective::toJson() const 
{
	Json data = Objective::toJson();
	data["skillType"] = m_skillType;
	data["craftJob"] = m_craftJob;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(!m_failedJobs.empty())
	{
		data["failedJobs"] = Json::array();
		for(CraftJob* job : m_failedJobs)
			data["failedJobs"].push_back(job);
	}
	return data;
}
void CraftObjective::execute()
{
	if(m_craftJob)
		m_craftJob->craftStepProject->commandWorker(m_actor);
	else
		m_threadedTask.create(*this);
}
void CraftObjective::cancel()
{
	m_threadedTask.maybeCancel();
	if(m_craftJob && m_craftJob->craftStepProject)
		m_craftJob->craftStepProject->cancel();
}
void CraftObjective::reset()
{
	m_actor.m_canReserve.clearAll();
	m_craftJob = nullptr;
	cancel();
}
// HasCraftingLocationsAndJobs
HasCraftingLocationsAndJobsForFaction::HasCraftingLocationsAndJobsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const Faction& f) : m_faction(f)
{
	for(const Json& pair : data["locationsByCategory"])
	{
		const CraftStepTypeCategory& category = CraftStepTypeCategory::byName(pair[0].get<std::string>());
		for(const Json& blockQuery : pair[1])
		{
			Block& block = deserializationMemo.blockReference(blockQuery);
			m_locationsByCategory[&category].insert(&block);
			m_stepTypeCategoriesByLocation[&block].insert(&category);
		}
	}
	for(const Json& jobData : data["jobs"])
		m_jobs.emplace_back(jobData, deserializationMemo, *this);
	for(const Json& pair : data["unassignedProjectsByStepTypeCategory"])
	{
		const CraftStepTypeCategory& category = CraftStepTypeCategory::byName(pair[0].get<std::string>());
		for(const Json& jobReference : pair[1])
			m_unassignedProjectsByStepTypeCategory[&category].insert(deserializationMemo.m_craftJobs.at(jobReference.get<uintptr_t>()));
	}
	for(const Json& pair : data["unassignedProjectsBySkill"])
	{
		const SkillType& skill = SkillType::byName(pair[0].get<std::string>());
		for(const Json& jobReference : pair[1])
			m_unassignedProjectsBySkill[&skill].insert(deserializationMemo.m_craftJobs.at(jobReference.get<uintptr_t>()));
	}
}
Json HasCraftingLocationsAndJobsForFaction::toJson() const
{
	Json data{{"faction", m_faction}, {"locationsByCategory", Json::array()}, {"stepTypeCategoriesByLocation", Json::array()}, {"unassignedProjectsByStepTypeCategory", Json::array()}, {"unassignedProjectsBySkill", Json::array()}};
	for(auto& [category, locations] : m_locationsByCategory)
	{
		Json pair = Json::array();
		pair[0] = category;
		pair[1] = Json::array();
		for(Block* location : locations)
			pair[1].push_back(location->positionToJson());
		data["locationsByCagegory"].push_back(pair);
	}
	for(auto& [location, categories] : m_stepTypeCategoriesByLocation)
	{
		Json pair = Json::array();
		pair[0] = location->positionToJson();
		pair[1] = Json::array();
		for(auto& category : categories)
			pair[1].push_back(category);
		data["stepTypeCategoriesByLocation"].push_back(pair);
	}
	for(auto& [category, projects] : m_unassignedProjectsByStepTypeCategory)
	{
		Json pair = Json::array();
		pair[0] = category;
		pair[1] = Json::array();
		for(auto* project : projects)
			pair[1].push_back(project);
		data["unassignedProjectsByStepTypeCategory"].push_back(pair);
	}
	for(auto& [skill, projects] : m_unassignedProjectsBySkill)
	{
		Json pair = Json::array();
		pair[0] = skill;
		pair[1] = Json::array();
		for(auto* project : projects)
			pair[1].push_back(project);
		data["unassignedProjectsBySkill"].push_back(pair);
	}
	for(auto& job : m_jobs)
		data["jobs"].push_back(job.toJson());
	return data;
}
void HasCraftingLocationsAndJobsForFaction::addLocation(const CraftStepTypeCategory& category, Block& block)
{
	m_locationsByCategory[&category].insert(&block);
	m_stepTypeCategoriesByLocation[&block].insert(&category);
}
void HasCraftingLocationsAndJobsForFaction::removeLocation(const CraftStepTypeCategory& category, Block& block)
{
	for(const CraftJob& craftJob : m_jobs)
	{
		if(craftJob.craftStepProject && craftJob.craftStepProject->getLocation() == block)
			craftJob.craftStepProject->cancel();
	}
	if(m_locationsByCategory.at(&category).size() == 1)
		m_locationsByCategory.erase(&category);
	else
		m_locationsByCategory[&category].erase(&block);
	if(m_stepTypeCategoriesByLocation.at(&block).size() == 1)
		m_stepTypeCategoriesByLocation.erase(&block);
	else
		m_stepTypeCategoriesByLocation[&block].erase(&category);
}
void HasCraftingLocationsAndJobsForFaction::maybeRemoveLocation(Block& block)
{
	if(m_stepTypeCategoriesByLocation.contains(&block))
	{
		std::vector<const CraftStepTypeCategory*> categories(m_stepTypeCategoriesByLocation.at(&block).begin(), m_stepTypeCategoriesByLocation.at(&block).end());
		for(auto* category : categories)
			removeLocation(*category, block);
	}
}
void HasCraftingLocationsAndJobsForFaction::addJob(const CraftJobType& craftJobType, const MaterialType* materialType, uint32_t quantity, uint32_t minimumSkillLevel)
{
	if(craftJobType.materialtypeCategory != nullptr)
		assert(materialType->materialTypeCategory == craftJobType.materialtypeCategory);
	for(uint32_t i = 0; i < quantity; i++)
	{
		CraftJob& craftJob = m_jobs.emplace_back(craftJobType, *this, materialType, minimumSkillLevel);
		indexUnassigned(craftJob);
	}
}
void HasCraftingLocationsAndJobsForFaction::cloneJob(CraftJob& job)
{
	CraftJob& craftJob = m_jobs.emplace_back(job.craftJobType, *this, job.materialType, job.minimumSkillLevel);
	indexUnassigned(craftJob);
}
void HasCraftingLocationsAndJobsForFaction::removeJob(CraftJob& job)
{
	if(job.craftStepProject)
		job.craftStepProject->cancel();
	m_jobs.remove(job);
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
			Item& item = location->m_area->m_simulation.createItemNongeneric(craftJob.craftJobType.productType, *craftJob.materialType, 0, 0, &craftJob);
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
		craftJob.workPiece->m_location->m_hasItems.addGeneric(craftJob.craftJobType.productType, *craftJob.materialType, craftJob.craftJobType.productQuantity);
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
bool HasCraftingLocationsAndJobsForFaction::hasLocationsFor(const CraftJobType& craftJobType) const
{
	for(const CraftStepType& craftStepType : craftJobType.stepTypes)
		if(!m_locationsByCategory.contains(&craftStepType.craftStepTypeCategory))
			return false;
	return true;
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
void HasCraftingLocationsAndJobs::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		const Faction& faction = deserializationMemo.faction(pair[0].get<std::wstring>());
		m_data.try_emplace(&faction, pair[1], deserializationMemo, faction);
	}
}
Json HasCraftingLocationsAndJobs::toJson() const
{
	Json data = Json::array();
	for(auto& [faction, hasCraftingLocationsAndJobsForFaction] : m_data)
	{
		auto pair = Json::array();
		pair[0] = faction;
		pair[1] = hasCraftingLocationsAndJobsForFaction.toJson();
		data.push_back(pair);
	}
	return data;
}