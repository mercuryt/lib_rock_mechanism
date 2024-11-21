#include "craft.h"
#include "actors/actors.h"
#include "items/items.h"
#include "area.h"
#include "deserializationMemo.h"
#include "materialType.h"
#include "pathRequest.h"
#include "terrainFacade.h"
#include "objectives/dig.h"
#include "types.h"
#include "util.h"
#include "simulation.h"
#include "itemType.h"

#include <chrono>
#include <cstdint>
#include <memory>
/*
void CraftInputAction::execute()
{
	m_area.m_hasCraftingLocationsAndJobs.at(m_faction).addJob(m_craftJobType, m_materialType, m_quantity);
}
void CraftCancelInputAction::execute()
{
	m_area.m_hasCraftingLocationsAndJobs.at(m_faction).removeJob(m_job);
}
*/
void CraftStepTypeCategory::create(std::string name) { craftStepTypeCategoryData.m_name.add(name); }
CraftStepTypeCategoryId CraftStepTypeCategory::byName(const std::string name)
{
	auto found = craftStepTypeCategoryData.m_name.find(name);
	assert(found != craftStepTypeCategoryData.m_name.end());
	return CraftStepTypeCategoryId::create(found - craftStepTypeCategoryData.m_name.begin());
}
std::string CraftStepTypeCategory::getName(CraftStepTypeCategoryId id) { return craftStepTypeCategoryData.m_name[id]; }

CraftStepProjectHasShapeDishonorCallback::CraftStepProjectHasShapeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_craftStepProject(*static_cast<CraftStepProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))) { }

void CraftJobType::create(std::string name, ItemTypeId productType, const Quantity& productQuantity, MaterialCategoryTypeId category, std::vector<CraftStepType> stepTypes)
{
	craftJobTypeData.m_name.add(name);
	craftJobTypeData.m_productType.add(productType);
	craftJobTypeData.m_productQuantity.add(productQuantity);
	craftJobTypeData.m_materialTypeCategory.add(category);
	craftJobTypeData.m_stepTypes.add(stepTypes);
}
CraftJobTypeId CraftJobType::byName(const std::string name)
{
	auto found = craftJobTypeData.m_name.find(name);
	assert(found != craftJobTypeData.m_name.end());
	return CraftJobTypeId::create(found - craftJobTypeData.m_name.begin());
}
std::string CraftJobType::getName(const CraftJobTypeId& id) { return craftJobTypeData.m_name[id]; }
ItemTypeId CraftJobType::getProductType(const CraftJobTypeId& id) { return craftJobTypeData.m_productType[id]; }
Quantity CraftJobType::getProductQuantity(const CraftJobTypeId& id) { return craftJobTypeData.m_productQuantity[id]; }
MaterialCategoryTypeId CraftJobType::getMaterialTypeCategory(const CraftJobTypeId& id) { return craftJobTypeData.m_materialTypeCategory[id]; }
std::vector<CraftStepType>& CraftJobType::getStepTypes(const CraftJobTypeId& id) { return craftJobTypeData.m_stepTypes[id]; }

CraftStepProject::CraftStepProject(const Json& data, DeserializationMemo& deserializationMemo, CraftJob& cj, Area& area) :
	Project(data, deserializationMemo, area),
	m_craftStepType(*cj.stepIterator),
	m_craftJob(cj) { }
Step CraftStepProject::getDuration() const
{
	uint32_t totalScore = 0;
	for(auto& pair : m_workers)
		totalScore += getWorkerCraftScore(pair.first.getIndex());
	return m_craftStepType.stepsDuration / totalScore;
}
// Static method.
uint32_t CraftStepProject::getWorkerCraftScore(const ActorIndex& actor) const
{
	return 1 + m_area.getActors().skill_getLevel(actor, m_craftStepType.skillType).get();
}
void CraftStepProject::onComplete()
{
	auto& [actor, projectWorker] = *m_workers.begin();
	Objective& objective = *projectWorker.objective;
	Area& area = m_area;
	const ActorIndex actorIndex = actor.getIndex();
	// This is destroyed here.
	m_craftJob.hasCraftingLocationsAndJobs.stepComplete(m_craftJob, actorIndex);
	area.getActors().objective_complete(actorIndex, objective);
}
void CraftStepProject::onCancel()
{
	ActorIndices workersAndCandidates = getWorkersAndCandidates();
	CraftJob& craftJob = m_craftJob;
	Area& area = m_area;
	// This is destroyed here.
	m_area.m_hasCraftingLocationsAndJobs.getForFaction(m_faction).stepDestroy(craftJob);
	Actors& actors = area.getActors();
	for(ActorIndex actor : workersAndCandidates)
	{
		CraftObjective& objective = actors.objective_getCurrent<CraftObjective>(actor);
		//TODO: This records this fail job for this actor regardless of if it might still be possible via another location or subsitute required actor or item.
		objective.recordFailedJob(craftJob);
		objective.reset(area, actor);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
void CraftStepProject::onAddToMaking(const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	Facing facing = m_area.getBlocks().shape_canEnterCurrentlyWithAnyFacingReturnFacing(m_location, actors.getShape(actor), actors.getBlocks(actor));
	if(facing.exists())
		actors.setLocationAndFacing(actor, m_location, facing);
	else
		setDelayOn();
}
std::vector<std::pair<ItemQuery, Quantity>> CraftStepProject::getConsumed() const
{
	// Make a copy so we can edit itemQueries.
	auto output = m_craftStepType.consumed;
	for(auto& pair : output)
		if(pair.first.m_materialType.exists())
		{
			pair.first.validate();
			if(!pair.first.m_item.exists() && !pair.first.m_materialType.exists())
				pair.first.specalize(m_craftJob.materialType);
		}
	return output;
}
std::vector<std::pair<ItemQuery, Quantity>> CraftStepProject::getUnconsumed() const
{
	auto output = m_craftStepType.unconsumed;
	if(m_craftJob.workPiece.exists())
		output.emplace_back(m_craftJob.workPiece, Quantity::create(1));
	return output;
}
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> CraftStepProject::getByproducts() const {
	auto output = m_craftStepType.byproducts;
	for(auto& tuple : output)
		if(std::get<1>(tuple).empty())
			std::get<1>(tuple) = m_craftJob.materialType;
	return output;
}
CraftJob::CraftJob(const CraftJobTypeId& cjt, HasCraftingLocationsAndJobsForFaction& hclaj, const ItemIndex& wp, const MaterialTypeId& mt, uint32_t msl) :
		craftJobType(cjt), hasCraftingLocationsAndJobs(hclaj), materialType(mt),
		stepIterator(CraftJobType::getStepTypes(craftJobType).begin()), minimumSkillLevel(msl),
		totalSkillPoints(0), reservable(Quantity::create(1))
{
	workPiece.setTarget(hasCraftingLocationsAndJobs.m_area.getItems().getReferenceTarget(wp));
}
CraftJob::CraftJob(const Json& data, DeserializationMemo& deserializationMemo, HasCraftingLocationsAndJobsForFaction& hclaj, Area& area) :
	craftJobType(data["craftJobType"].get<CraftJobTypeId>()),
	hasCraftingLocationsAndJobs(hclaj),
	materialType(data.contains("materialType") ? MaterialType::byName(data["materialType"].get<std::string>()) : MaterialTypeId::null()),
	stepIterator(CraftJobType::getStepTypes(craftJobType).begin() + data["stepIndex"].get<size_t>()),
	craftStepProject(data.contains("craftStepProject") ? std::make_unique<CraftStepProject>(data["craftStepProject"], deserializationMemo, *this, area) : nullptr),
	minimumSkillLevel(data["minimumSkillLevel"].get<uint32_t>()),
	totalSkillPoints(data["totalSkillPoints"].get<uint32_t>()),
	reservable(Quantity::create(1))
{
	deserializationMemo.m_craftJobs[data["address"].get<uintptr_t>()] = this;
	if(data.contains("workPiece"))
		workPiece.load(data["workPiece"], hasCraftingLocationsAndJobs.m_area);
}
Json CraftJob::toJson() const
{
	Json data{
		{"address", reinterpret_cast<uintptr_t>(this)},
		{"craftJobType", craftJobType},
		{"stepIndex", stepIterator - CraftJobType::getStepTypes(craftJobType).begin()},
		{"minimumSkillLevel", minimumSkillLevel},
		{"totalSkillPoints", totalSkillPoints}
	};
	if(craftStepProject)
		data["craftStepProject"] = craftStepProject->toJson();
	if(workPiece.exists())
		data["workPiece"] = workPiece;
	if(materialType.exists())
		data["materialType"] = materialType;
	return data;
}
Quality CraftJob::getQuality() const
{
	float pointsPerStep = (float)totalSkillPoints / CraftJobType::getStepTypes(craftJobType).size();
	return Quality::create((pointsPerStep / Config::maxSkillLevel) * 100);
}
Step CraftJob::getStep() const
{
	return Step::create(1) + (stepIterator - CraftJobType::getStepTypes(craftJobType).begin());
}
// HasCraftingLocationsAndJobs
HasCraftingLocationsAndJobsForFaction::HasCraftingLocationsAndJobsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& f) :
	m_area(deserializationMemo.area(data["area"])),
    	m_faction(f)
{
	for(const Json& pair : data["locationsByCategory"])
	{
		CraftStepTypeCategoryId category = CraftStepTypeCategory::byName(pair[0].get<std::string>());
		for(const Json& blockQuery : pair[1])
		{
			BlockIndex block = blockQuery.get<BlockIndex>();
			m_locationsByCategory[category].add(block);
			m_stepTypeCategoriesByLocation[block].push_back(category);
		}
	}
	if(data.contains("jobs"))
		for(const Json& jobData : data["jobs"])
			m_jobs.emplace_back(jobData, deserializationMemo, *this, m_area);
	for(const Json& pair : data["unassignedProjectsByStepTypeCategory"])
	{
		CraftStepTypeCategoryId category = CraftStepTypeCategory::byName(pair[0].get<std::string>());
		for(const Json& jobReference : pair[1])
		{
			CraftJob* job = deserializationMemo.m_craftJobs.at(jobReference.get<uintptr_t>());
			m_unassignedProjectsByStepTypeCategory[category].push_back(job);
		}
	}
	for(const Json& pair : data["unassignedProjectsBySkill"])
	{
		SkillTypeId skill = SkillType::byName(pair[0].get<std::string>());
		for(const Json& jobReference : pair[1])
			m_unassignedProjectsBySkill[skill].push_back(deserializationMemo.m_craftJobs.at(jobReference.get<uintptr_t>()));
	}
}
void HasCraftingLocationsAndJobsForFaction::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	if(data.contains("jobs"))
		for(const Json& jobData : data["jobs"])
		{
			assert(deserializationMemo.m_craftJobs.contains(jobData["address"].get<uintptr_t>()));
			CraftJob& job = *deserializationMemo.m_craftJobs.at(jobData["address"].get<uintptr_t>());
			if(job.craftStepProject)
				job.craftStepProject->loadWorkers(jobData["craftStepProject"], deserializationMemo);
		}
}
Json HasCraftingLocationsAndJobsForFaction::toJson() const
{
	Json data{{"faction", m_faction}, {"locationsByCategory", Json::array()}, {"stepTypeCategoriesByLocation", Json::array()}, {"unassignedProjectsByStepTypeCategory", Json::array()}, {"unassignedProjectsBySkill", Json::array()}, {"area", m_area}};
	for(auto& [category, locations] : m_locationsByCategory)
	{
		Json pair = Json::array();
		pair[0] = category;
		pair[1] = Json::array();
		for(BlockIndex location : locations)
			pair[1].push_back(location);
		data["locationsByCategory"].push_back(pair);
	}
	for(auto& [location, categories] : m_stepTypeCategoriesByLocation)
	{
		Json pair = Json::array();
		pair[0] = location;
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
void HasCraftingLocationsAndJobsForFaction::addLocation(CraftStepTypeCategoryId category, const BlockIndex& block)
{
	m_locationsByCategory.getOrCreate(category).add(block);
	util::addUniqueToVectorAssert(m_stepTypeCategoriesByLocation.getOrCreate(block), category);
}
void HasCraftingLocationsAndJobsForFaction::removeLocation(CraftStepTypeCategoryId category, const BlockIndex& block)
{
	for(const CraftJob& craftJob : m_jobs)
	{
		if(craftJob.craftStepProject && craftJob.craftStepProject->getLocation() == block)
			craftJob.craftStepProject->cancel();
	}
	if(m_locationsByCategory[category].size() == 1)
		m_locationsByCategory.erase(category);
	else
		m_locationsByCategory[category].remove(block);
	if(m_stepTypeCategoriesByLocation[block].size() == 1)
		m_stepTypeCategoriesByLocation.erase(block);
	else
		util::removeFromVectorByValueUnordered(m_stepTypeCategoriesByLocation[block], category);
}
void HasCraftingLocationsAndJobsForFaction::maybeRemoveLocation(const BlockIndex& block)
{
	if(m_stepTypeCategoriesByLocation.contains(block))
	{
		std::vector<CraftStepTypeCategoryId> categories(m_stepTypeCategoriesByLocation[block].begin(), m_stepTypeCategoriesByLocation[block].end());
		for(auto category : categories)
			removeLocation(category, block);
	}
}
void HasCraftingLocationsAndJobsForFaction::addJob(const CraftJobTypeId& craftJobType, const MaterialTypeId& materialType, const Quantity& quantity, uint32_t minimumSkillLevel)
{
	if(CraftJobType::getMaterialTypeCategory(craftJobType).exists() && materialType.exists())
		assert(MaterialType::getMaterialTypeCategory(materialType) == CraftJobType::getMaterialTypeCategory(craftJobType));
	for(Quantity i = Quantity::create(0); i < quantity; ++i)
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
void HasCraftingLocationsAndJobsForFaction::stepComplete(CraftJob& craftJob, const ActorIndex& actor)
{
	craftJob.totalSkillPoints += craftJob.craftStepProject->getWorkerCraftScore(actor);
	BlockIndex location = craftJob.craftStepProject->getLocation();
	++craftJob.stepIterator;
	if(craftJob.stepIterator == CraftJobType::getStepTypes(craftJob.craftJobType).end())
		jobComplete(craftJob, location);
	else
	{
		if(!craftJob.workPiece.exists())
		{
			ItemParamaters params{
				.itemType=CraftJobType::getProductType(craftJob.craftJobType),
				.materialType=craftJob.materialType,
				.craftJob=&craftJob,
				.location=location,
				.quality=Quality::create(0),
				.percentWear=Percent::create(0),
			};
			Items& items = m_area.getItems();
			ItemIndex index = items.create(params);
			craftJob.workPiece = items.getReference(index);
			//TODO: Should the item be reserved?
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
	util::addUniqueToVectorAssert(m_unassignedProjectsByStepTypeCategory.getOrCreate(craftStepType.craftStepTypeCategory), &craftJob);
	util::addUniqueToVectorAssert(m_unassignedProjectsBySkill.getOrCreate(craftStepType.skillType), &craftJob);
}
void HasCraftingLocationsAndJobsForFaction::maybeUnindexUnassigned(CraftJob& craftJob)
{
	const auto& category = craftJob.stepIterator->craftStepTypeCategory;
	if(!m_unassignedProjectsByStepTypeCategory.contains(category))
		return;
	const auto& vector = m_unassignedProjectsByStepTypeCategory[category];
	if(std::ranges::find(vector, &craftJob) != vector.end())
		unindexUnassigned(craftJob);
}
void HasCraftingLocationsAndJobsForFaction::unindexUnassigned(CraftJob& craftJob)
{
	const CraftStepType& craftStepType = *craftJob.stepIterator;
	if(m_unassignedProjectsByStepTypeCategory[craftStepType.craftStepTypeCategory].size() == 1)
		m_unassignedProjectsByStepTypeCategory.erase(craftStepType.craftStepTypeCategory);
	else
		util::removeFromVectorByValueUnordered(m_unassignedProjectsByStepTypeCategory[craftStepType.craftStepTypeCategory], &craftJob);
	if(m_unassignedProjectsBySkill[craftStepType.skillType].size() == 1)
		m_unassignedProjectsBySkill.erase(craftStepType.skillType);
	else
		util::removeFromVectorByValueUnordered(m_unassignedProjectsBySkill[craftStepType.skillType], &craftJob);
}
void HasCraftingLocationsAndJobsForFaction::jobComplete(CraftJob& craftJob, const BlockIndex& location)
{
	ItemIndex product;
	Blocks& blocks = m_area.getBlocks();
	Items& items = m_area.getItems();
	ItemTypeId productType = CraftJobType::getProductType(craftJob.craftJobType);
	if(ItemType::getIsGeneric(productType))
	{
		product = blocks.item_addGeneric(location, productType, craftJob.materialType, CraftJobType::getProductQuantity(craftJob.craftJobType));
		if(craftJob.workPiece.exists())
			items.destroy(craftJob.workPiece.getIndex());
	}
	else
	{
		assert(CraftJobType::getProductQuantity(craftJob.craftJobType) == 1);
		if(craftJob.workPiece.exists())
		{
			// Not generic and workpiece exists, use it as product.
			ItemIndex index = craftJob.workPiece.getIndex();
			items.setQuality(index, craftJob.getQuality());
			items.unsetCraftJobForWorkPiece(index);
			product = index;
		}
		else
		{
			ItemParamaters params{
				.itemType=CraftJobType::getProductType(craftJob.craftJobType),
				.materialType=craftJob.materialType,
				.location=location,
				.quality=craftJob.getQuality(),
				.percentWear=Percent::create(0),
			};
			items.create(params);
		}
	}
	if(!m_area.m_hasStockPiles.contains(m_faction))
		m_area.m_hasStockPiles.registerFaction(m_faction);
	m_area.m_hasStockPiles.getForFaction(m_faction).maybeAddItem(product);
	m_jobs.remove(craftJob);
}
void HasCraftingLocationsAndJobsForFaction::makeAndAssignStepProject(CraftJob& craftJob, const BlockIndex& location, CraftObjective& objective, const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	craftJob.craftStepProject = std::make_unique<CraftStepProject>(actors.getFactionId(actor), m_area, location, *craftJob.stepIterator, craftJob);
	std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<CraftStepProjectHasShapeDishonorCallback>(*craftJob.craftStepProject.get());
	craftJob.craftStepProject->setLocationDishonorCallback(std::move(dishonorCallback));
	craftJob.craftStepProject->addWorkerCandidate(actor, objective);
	unindexUnassigned(craftJob);
}
bool HasCraftingLocationsAndJobsForFaction::hasLocationsFor(const CraftJobTypeId& craftJobType) const
{
	for(const CraftStepType& craftStepType : CraftJobType::getStepTypes(craftJobType))
		if(!m_locationsByCategory.contains(craftStepType.craftStepTypeCategory))
			return false;
	return true;
}
std::vector<CraftStepTypeCategoryId>& HasCraftingLocationsAndJobsForFaction::getStepTypeCategoriesForLocation(const BlockIndex& location)
{
	if(!m_stepTypeCategoriesByLocation.contains(location))
	{
		static std::vector<CraftStepTypeCategoryId> empty;
		return empty;
	}
	return m_stepTypeCategoriesByLocation[location];
}
CraftStepTypeCategoryId HasCraftingLocationsAndJobsForFaction::getDisplayStepTypeCategoryForLocation(const BlockIndex& location)
{
	if(!m_stepTypeCategoriesByLocation.contains(location))
		return CraftStepTypeCategoryId::null();
	return *m_stepTypeCategoriesByLocation[location].begin();
}
// May return nullptr;
CraftJob* HasCraftingLocationsAndJobsForFaction::getJobForAtLocation(const ActorIndex& actor, const SkillTypeId& skillType, const BlockIndex& block, SmallSet<CraftJob*>& excludeJobs)
{
	Actors& actors = m_area.getActors();
	assert(!m_area.getBlocks().isReserved(block, actors.getFactionId(actor)));
	if(!m_stepTypeCategoriesByLocation.contains(block))
		return nullptr;
	for(CraftStepTypeCategoryId category : m_stepTypeCategoriesByLocation[block])
		if(m_unassignedProjectsByStepTypeCategory.contains(category))
			for(CraftJob* craftJob : m_unassignedProjectsByStepTypeCategory[category])
				if(
					!excludeJobs.contains(craftJob) &&
					craftJob->stepIterator->skillType == skillType &&
					actors.skill_getLevel(actor, skillType) >= craftJob->minimumSkillLevel
				)
					
					return craftJob;
	return nullptr;
}
void AreaHasCraftingLocationsAndJobs::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data["projects"])
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.try_emplace(faction, pair[1], deserializationMemo, faction);
	}
}
void AreaHasCraftingLocationsAndJobs::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data["projects"])
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.at(faction).loadWorkers(pair[1], deserializationMemo);
	}
}
Json AreaHasCraftingLocationsAndJobs::toJson() const
{
	Json projects = Json::array();
	for(auto& [faction, hasCraftingLocationsAndJobsForFaction] : m_data)
	{
		auto pair = Json::array();
		pair[0] = faction;
		pair[1] = hasCraftingLocationsAndJobsForFaction.toJson();
		projects.push_back(pair);
	}
	return {{"area", m_area}, {"projects", projects}};
}
void AreaHasCraftingLocationsAndJobs::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& job : pair.second.m_jobs)
			if(job.craftStepProject)
				job.craftStepProject->clearReservations();
}
HasCraftingLocationsAndJobsForFaction& AreaHasCraftingLocationsAndJobs::getForFaction(const FactionId& faction)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	return m_data.at(faction);
}
