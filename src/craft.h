#pragma once

#include "item.h"
#include "project.h"
#include "block.h"

#include <vector>
#include <utility>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

struct CraftJob;
class HasCraftingLocationsAndJobs;
class CraftThreadedTask;
struct skillType;
// Drill, saw, forge, etc.
struct CraftStepTypeCategory final
{
	const std::string name;
	// Infastructure.
	inline static std::list<CraftStepTypeCategory> data;
	static CraftStepTypeCategory& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &CraftStepTypeCategory::name);
		assert(found != data.end());
		return *found;
	}
};
// Part of the definition of a particular CraftJobType which makes a specific Item.
struct CraftStepType final
{
	const std::string name;
	const CraftStepTypeCategory& craftStepTypeCategory;
	const SkillType& skillType;
	const Step stepsDuration;
	std::vector<std::pair<ItemQuery, uint32_t>> consumed;
	std::vector<std::pair<ItemQuery, uint32_t>> unconsumed;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> byproducts;
};
// A specific step of a specific CraftJob.
class CraftStepProject final : public Project
{
	const CraftStepType& m_craftStepType;
	CraftJob& m_craftJob;
	Step getDuration() const;
	void onComplete();
	void onDelay() { cancel(); }
	void offDelay() { assert(false); }
	// Use copies rather then references for return types to allow specalization of Queries as well as byproduct material type.
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const { return {}; }
public:
	CraftStepProject(const Faction* faction, Block& location, const CraftStepType& cst, CraftJob& cj) : Project(faction, location, 1), m_craftStepType(cst), m_craftJob(cj) { }
	uint32_t getWorkerCraftScore(const Actor& actor) const;
};
// Data about making a specific product type.
struct CraftJobType final
{
	const std::string name;
	const ItemType& productType;
	const uint32_t productQuantity;
	const MaterialTypeCategory* materialtypeCategory;
	std::vector<CraftStepType> stepTypes;
	// Infastructure.
	inline static std::list<CraftJobType> data;
	static CraftJobType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &CraftJobType::name);
		assert(!found->stepTypes.empty());
		assert(found != data.end());
		return *found;
	}
};
// Make a specific product.
struct CraftJob final
{
	const CraftJobType& craftJobType;
	HasCraftingLocationsAndJobs& hasCraftingLocationsAndJobs;
	Item* workPiece;
	const MaterialType* materialType;
	std::vector<CraftStepType>::const_iterator stepIterator;
	std::unique_ptr<CraftStepProject> craftStepProject;
	uint32_t minimumSkillLevel;
	uint32_t totalSkillPoints;
	Reservable reservable;
	// If work piece is provided then this is an upgrade job.
	CraftJob(const CraftJobType& cjt, HasCraftingLocationsAndJobs& hclaj, Item& wp, const MaterialType* mt, uint32_t msl) : 
		craftJobType(cjt), hasCraftingLocationsAndJobs(hclaj), workPiece(&wp), materialType(mt), stepIterator(craftJobType.stepTypes.begin()), minimumSkillLevel(msl), totalSkillPoints(0), reservable(1) { }
	// No work piece provided is a create job.
	CraftJob(const CraftJobType& cjt, HasCraftingLocationsAndJobs& hclaj, const MaterialType* mt, uint32_t msl) :
	       	craftJobType(cjt), hasCraftingLocationsAndJobs(hclaj), workPiece(nullptr), materialType(mt), stepIterator(craftJobType.stepTypes.begin()), minimumSkillLevel(msl), totalSkillPoints(0), reservable(1) { }
	uint32_t getQuality() const;
	uint32_t getStep() const;
	bool operator==(const CraftJob& other){ return &other == this; }
};
class CraftObjectiveType final : public ObjectiveType
{
	const SkillType& m_skillType; // Multiple skills are represented by CraftObjective and CraftObjectiveType, such as Leatherworking, Woodworking, Metalworking, etc.
public:
	CraftObjectiveType(const SkillType& skillType) : ObjectiveType(), m_skillType(skillType) { }
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Craft; }
};
class CraftObjective final : public Objective
{
	Actor& m_actor;
	const SkillType& m_skillType;
	CraftJob* m_craftJob;
	HasThreadedTask<CraftThreadedTask> m_threadedTask;
public:
	CraftObjective(Actor& a, const SkillType& st);
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Craft; }
	std::string name() const { return "craft"; }
	friend class CraftThreadedTask;
	friend class HasCraftingLocationsAndJobs;
};
class CraftThreadedTask final : public ThreadedTask
{
	CraftObjective& m_craftObjective;
	CraftJob* m_craftJob;
	Block* m_location;
public:
	CraftThreadedTask(CraftObjective& co) : ThreadedTask(co.m_actor.getThreadedTaskEngine()), m_craftObjective(co), m_craftJob(nullptr), m_location(nullptr) { }
	void readStep();
	void writeStep();
	void clearReferences();
};
// To be used by Area.
class HasCraftingLocationsAndJobs final
{
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<Block*>> m_locationsByCategory;
	std::unordered_map<Block*, std::unordered_set<const CraftStepTypeCategory*>> m_stepTypeCategoriesByLocation;
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<CraftJob*>> m_unassignedProjectsByStepTypeCategory;
	std::unordered_map<const SkillType*, std::unordered_set<CraftJob*>> m_unassignedProjectsBySkill;
	std::list<CraftJob> m_jobs;
public:
	void addLocation(const CraftStepTypeCategory& craftStepTypeCategory, Block& block);
	void removeLocation(const CraftStepTypeCategory& craftStepTypeCategory, Block& block);
	void addJob(const CraftJobType& craftJobType, const MaterialType* materialType, uint32_t minimumSkillLevel = 0);
	void stepComplete(CraftJob& craftJob, Actor& actor);
	void stepInterupted(CraftJob& craftJob);
	void indexUnassigned(CraftJob& craftJob);
	void unindexAssigned(CraftJob& craftJob);
	void jobComplete(CraftJob& craftJob);
	void makeAndAssignStepProject(CraftJob& craftJob, Block& location, CraftObjective& objective);
	// To be used by the UI.
	[[nodiscard]] bool hasLocationsFor(const CraftJobType& craftJobType) const;
	// May return nullptr;
	CraftJob* getJobForAtLocation(const Actor& actor, const SkillType& skillType, const Block& block);
	std::pair<CraftJob*, Block*> getJobAndLocationFor(const Actor& actor, const SkillType& skillType);
	friend class CraftObjectiveType;
};
