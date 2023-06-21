#pragma once

#include <vector>
#include <pair>
#include <string>
#include "item.h"
#include "util.h"

struct CraftStepTypeCategory
{
	const std::string name;
};
struct CraftStepType
{
	const std::string name;
	const CraftStepTypeCategory& craftStepTypeCategory;
	const std::vector<std::pair<ItemQuery, uint32_t>> consumedItems;
	const std::vector<std::pair<ItemQuery, uint32_t>> unconsumedItems;
	const std::vector<std::pair<ItemType, uint32_t>> consumedItemsOfSameTypeAsProduct;
	const std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t> byproductItems;
	const std::vector<std::pair<unordered_map<const SkillType*, uint32_t>> skillsAndPercentWeights;
	const uint32_t stepsDuration;
};
template<class Block, class Actor>
class CraftStepProject : Project
{
	const CraftStepType& m_craftStepType;
	CraftJob& m_craftJob;
	CraftStepProject(Block& location, const CraftStepType& cst, CraftJob& cj) : Project(location, 1), m_craftStepType(cst), m_craftJob(cj) { }
	uint32_t getDelay() const
	{
		uint32_t totalScore = 0;
		for(Actor* actor : m_workers)
			totalScore += getWorkerCraftScore(*actor);
		return m_craftStepType.stepsDuration / totalScore;
	}
	uint32_t getWorkerCraftScore(Actor& actor) const
	{
		uint32_t output = 0;
		for(auto& [skillType, percentWeight] : m_craftStepType.skillsAndPercentWeights)
			output += util::scaleByPercent(actor.m_skilSet.get(skillType), percentWeight);
		return output;
	}
	void onComplete()
	{
		assert(m_workers.size() == 1);
		m_craftJob.hasCraftingLocationsAndJob.stepComplete(m_craftJob, *m_workers.begin());
	}
	std::vector<std::pair<ItemQuery, uint8_t>> getConsumed() const
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
	std::vector<std::pair<ItemQuery, uint8_t>> getUnconsumed() const 
	{ 
		auto output = m_craftStepType.unconsumedItems; 
		if(craftJob.workPiece != nullptr)
			output.emplace({craftJob.workPiece}, 1);
		return output; 
	}
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint8_t>> getByproducts() const {
	       	auto output =  m_craftStepType.byproductItems; 
		for(auto& tuple : output)
			if(tuple.at(1) == nullptr)
				tuple.at(1) = m_craftJob.materialType;
	       	return output; 
	}
};
struct CraftJobType
{
	const std::string name;
	const std::vector<const CraftStepType> stepTypes;
	const std::unordered_map<const ItemType*, uint32_t> productItems;
	const std::unordered_map<const ItemType*, uint32_t> requiredItems;
	const std::vector<const CraftStepTypeCategory*> requiredCraftingLocations;
};
struct CraftJob
{
	const CraftJobType& craftJobType;
	HasCraftingLocationsAndJobs& hasCraftingLocationsAndJobs;
	Item* workPiece;
	const MaterialType* materialType;
	std::vector<const CraftStepType>::iterator stepIterator;
	CraftStepProject craftStepProject;
	uint32_t minimumSkillLevel;
	uint32_t totalSkillPoints;
	Reservable reservable;
	// Workpiece and materialType can be null.
	CraftJob(const CraftJobType& cjt, HasCraftingLocationsAndJobs& hclaj, Item* wp, const MaterialType* mt, uint32_t msl) : craftJobType(cjt), hasCraftingLocationsAndJobs(hclaj), stepIterator(craftJobType.steps.begin()), workPiece(wp), materialType(mt), totalSkillPoints(0), minimumSkillLevel(msl) { }
};
class CraftThreadedTask : ThreadedTask
{
	CraftObjective& m_craftObjective;
	CraftJob* m_craftJob;
	Block* m_loction;
	CraftThreadedTask(CraftObjective& co) : m_craftObjective(co), m_craftJob(nullptr), m_loction(nullptr) { }
	void readStep()
	{
		[m_craftJob, m_location] = m_craftObjective.m_actor.m_location->m_hasCraftingLocations.getJobAndLocationFor(m_craftObjective.m_actor, m_skillType);
	}
	void writeStep()
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
};
template<class Block, class Actor>
class CraftObjectiveType : ObjectiveType<Actor>
{
	SkillType& m_skillType; // Multiple skills are represented by CraftObjective and CraftObjectiveType, such as Leatherworking, Woodworking, Metalworking, etc.
	CraftObjectiveType(const SkillType& skillType) : m_skillType(skillType) { }
	bool canBeAssigned(Actor& actor)
	{
		auto& hasCraftingLocationsAndJobs = actor.m_location->m_area->m_hasCraftingLocationsAndJobs;
		// No jobs needing this skill.
		if(!hasCraftingLocationsAndJobs.m_unassignedProjectsBySkill.contains(&m_skillType))
			return false;
		for(CraftJob* craftJob : hasCraftingLocationsAndJobs.m_unassignedProjectsBySkill.at(&m_skillType))
		{
			auto category = craftJob->stepIterator->craftStepTypeCategory;
			// No location to do the craftStepTypeCategory work at.
			if(!hasCraftingLocationsAndJob.m_locationsByCategory.contains(category))
				return false;
		}
		return false;
	}
	std::unique_ptr<Objective> makeFor(Actor& actor)
	{

		return std::make_unique<CraftObjective>(actor, m_skillType);
	}
};
template<class Block, class Actor>
class CraftObjective : Objective
{
	Actor& m_actor;
	const SkillType& m_skillType;
	HasThreadedTask<CraftThreadedTask> m_threadedTask;
	CraftObjective(Actor& a, const SkillType& st) : m_actor(a), m_skillType(st) { }
	void execute(){ m_threadedTask.create(*this, m_skillType); }
};
// To be used by Area.
template<class Block, class Actor, class SkillSet>
class HasCraftingLocationsAndJobs
{
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<Block*>> m_locationsByCategory;
	std::unordered_map<Block*, std::unordered_set<const CraftStepTypeCategory*>> m_stepTypeCategoriesByLocation;
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<CraftJob*>> m_unassignedProjectsByStepTypeCategory;
	std::unordered_map<const SkillType*, std::unordered_set<CraftJob*>> m_unassignedProjectsBySkill;
	std::list<CraftJob> m_jobs;
public:
	void addLocation(std::vector<const CraftStepType*>& craftStepTypes, Block& block)
	{
		for(auto* craftStepType : craftStepTypes)
			m_locationsByCategory[craftStepType].insert(&block);
	}
	void removeLocation(std::vector<const CraftStepType*>& craftStepTypes, Block& block)
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
	bool hasLocationsFor(const CraftType& craftType) const
	{
		for(auto* craftStepType : craftType.craftStepTypes)
			if(!m_locationsByCategory.contains(craftStepType))
				return false;
		return true;
	}
	void addJob(CraftJobType& craftJobType)
	{
		CraftJob& craftJob = m_jobs.emplace_back(craftJobType);
		indexUnassigned(craftJob);
	}
	void stepComplete(CraftJob& craftJob, Actor& actor)
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
	void indexUnassigned(CraftJob& craftJob)
	{
		CraftStepType& craftStepType = craftJob.stepIterator->craftStepType;
		m_unassignedProjectsByStepTypeCategory[craftStepType.craftStepTypeCategory].insert(&craftJob);
		m_unassignedProjectsBySkill[craftStepType.skillType].insert(&craftJob);
	}
	void unindexAssigned(CraftJob& craftJob)
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
	void jobComplete(CraftJob& craftJob)
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
	void makeAndAssignStepProject(CraftJob& craftJob, Block& location, Actor& worker)
	{
		craftJob.craftStepProject(location, craftJob.stepIterator, craftJob);
		craftJob.craftStepProject.addWorker(worker);
		auto& category = craftJob.stepIterator->craftStepType.craftStepTypeCategory;
		unindexAssigned(craftJob);
	}
	// May return nullptr;
	CraftJob* getJobForAtLocation(Actor& actor, const SkillType& skillType, Block& block) const
	{
		assert(!block.m_reservable.isFullyReserved());
		if(!m_stepTypeCategoriesByLocation.contains(&block))
			return nullptr;
		for(CraftStepTypeCategory* category : m_stepTypeCategoriesByLocation.at(&block))
			for(CraftJob& craftJob : m_unassignedProjectsByStepTypeCategory.at(category))
				if(craftJob.stepIterator->skillType == skillType && actor.m_skilSet.get(skillType) >= craftJob.minimumSkillLevel)
					return &craftJob;
		return nullptr;
	}
	std::pair<CraftJob*, Block*> getJobAndLocationFor(Actor& actor, const SkillType& skillType)
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
};
