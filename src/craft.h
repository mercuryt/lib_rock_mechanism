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
class CraftStepProject : Project
{
	const CraftStepType& m_craftStepType;
	CraftJob& m_craftJob;
	CraftStepProject(Block& location, const CraftStepType& cst, CraftJob& cj) : Project(location, 1), m_craftStepType(cst), m_craftJob(cj) { }
	uint32_t getDelay() const;
	uint32_t getWorkerCraftScore(Actor& actor) const;
	void onComplete();
	std::vector<std::pair<ItemQuery, uint8_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint8_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint8_t>> getByproducts() const;
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
	void readStep();
	void writeStep();
};
class CraftObjectiveType : ObjectiveType<Actor>
{
	SkillType& m_skillType; // Multiple skills are represented by CraftObjective and CraftObjectiveType, such as Leatherworking, Woodworking, Metalworking, etc.
	CraftObjectiveType(const SkillType& skillType) : m_skillType(skillType) { }
	bool canBeAssigned(Actor& actor);
	std::unique_ptr<Objective> makeFor(Actor& actor);
};
class CraftObjective : Objective
{
	Actor& m_actor;
	const SkillType& m_skillType;
	HasThreadedTask<CraftThreadedTask> m_threadedTask;
	CraftObjective(Actor& a, const SkillType& st) : m_actor(a), m_skillType(st) { }
	void execute();
};
// To be used by Area.
class HasCraftingLocationsAndJobs
{
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<Block*>> m_locationsByCategory;
	std::unordered_map<Block*, std::unordered_set<const CraftStepTypeCategory*>> m_stepTypeCategoriesByLocation;
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<CraftJob*>> m_unassignedProjectsByStepTypeCategory;
	std::unordered_map<const SkillType*, std::unordered_set<CraftJob*>> m_unassignedProjectsBySkill;
	std::list<CraftJob> m_jobs;
public:
	void addLocation(std::vector<const CraftStepType*>& craftStepTypes, Block& block);
	void removeLocation(std::vector<const CraftStepType*>& craftStepTypes, Block& block);
	// To be used by the UI.
	bool hasLocationsFor(const CraftType& craftType) const;
	void addJob(CraftJobType& craftJobType);
	void stepComplete(CraftJob& craftJob, Actor& actor);
	void indexUnassigned(CraftJob& craftJob);
	void unindexAssigned(CraftJob& craftJob);
	void jobComplete(CraftJob& craftJob);
	void makeAndAssignStepProject(CraftJob& craftJob, Block& location, Actor& worker);
	// May return nullptr;
	CraftJob* getJobForAtLocation(Actor& actor, const SkillType& skillType, Block& block) const;
	std::pair<CraftJob*, Block*> getJobAndLocationFor(Actor& actor, const SkillType& skillType);
};
