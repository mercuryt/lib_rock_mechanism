#pragma once

#include "config.h"
#include "deserializationMemo.h"
#include "item.h"
#include "project.h"
#include "reservable.h"

#include <vector>
#include <utility>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

struct CraftJob;
class HasCraftingLocationsAndJobsForFaction;
class CraftThreadedTask;
struct skillType;
class CraftStepProject;
class Block;
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
inline void to_json(Json& data, const CraftStepTypeCategory* const& category){ data = category->name; }
inline void from_json(const Json& data, const CraftStepTypeCategory*& category){ category = &CraftStepTypeCategory::byName(data.get<std::string>()); }
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
	void onCancel();
	void onDelay() { cancel(); }
	void offDelay() { assert(false); }
	bool canReset() const  { return false; }
	virtual void onHasShapeReservationDishonored([[maybe_unused]] const HasShape& hasShape, [[maybe_unused]]uint32_t oldCount, [[maybe_unused]]uint32_t newCount) { cancel(); }
	// Use copies rather then references for return types to allow specalization of Queries as well as byproduct material type.
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const { return {}; }
public:
	CraftStepProject(const Faction* faction, Block& location, const CraftStepType& cst, CraftJob& cj) : Project(faction, location, 1), m_craftStepType(cst), m_craftJob(cj) { }
	CraftStepProject(const Json& data, DeserializationMemo& deserializationMemo, CraftJob& cj);
	// No toJson needed here, the base class one has everything.
	uint32_t getWorkerCraftScore(const Actor& actor) const;
};
struct CraftStepProjectHasShapeDishonorCallback final : public DishonorCallback
{
	CraftStepProject& m_craftStepProject;
	CraftStepProjectHasShapeDishonorCallback(CraftStepProject& hs) : m_craftStepProject(hs) { } 
	CraftStepProjectHasShapeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) : 
		m_craftStepProject(*static_cast<CraftStepProject*>(deserializationMemo.m_projects.at(data["proejct"].get<uintptr_t>()))) { } 
	Json toJson() const { return Json({{"type", "CraftStepProjectHasShapeDishonorCallback"}, {"project", reinterpret_cast<uintptr_t>(&m_craftStepProject)}}); }
	// Craft step project cannot reset so cancel instead and allow to be recreated later.
	// TODO: Why?
	void execute([[maybe_unused]] uint32_t oldCount, [[maybe_unused]] uint32_t newCount) { m_craftStepProject.cancel(); }
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
inline void to_json(Json& data, const CraftJobType* const& craftJobType){ data = craftJobType->name; }
inline void to_json(Json& data, const CraftJobType& craftJobType){ data = craftJobType.name; }
inline void from_json(const Json& data, const CraftJobType*& craftJobType){ craftJobType = &CraftJobType::byName(data.get<std::string>()); }
// Make a specific product.
struct CraftJob final
{
	const CraftJobType& craftJobType;
	HasCraftingLocationsAndJobsForFaction& hasCraftingLocationsAndJobs;
	Item* workPiece;
	const MaterialType* materialType;
	std::vector<CraftStepType>::const_iterator stepIterator;
	std::unique_ptr<CraftStepProject> craftStepProject;
	uint32_t minimumSkillLevel;
	uint32_t totalSkillPoints;
	Reservable reservable;
	// If work piece is provided then this is an upgrade job.
	CraftJob(const CraftJobType& cjt, HasCraftingLocationsAndJobsForFaction& hclaj, Item& wp, const MaterialType* mt, uint32_t msl) : 
		craftJobType(cjt), hasCraftingLocationsAndJobs(hclaj), workPiece(&wp), materialType(mt), stepIterator(craftJobType.stepTypes.begin()), minimumSkillLevel(msl), totalSkillPoints(0), reservable(1) { }
	// No work piece provided is a create job.
	CraftJob(const CraftJobType& cjt, HasCraftingLocationsAndJobsForFaction& hclaj, const MaterialType* mt, uint32_t msl) :
	       	craftJobType(cjt), hasCraftingLocationsAndJobs(hclaj), workPiece(nullptr), materialType(mt), stepIterator(craftJobType.stepTypes.begin()), minimumSkillLevel(msl), totalSkillPoints(0), reservable(1) { }
	CraftJob(const Json& data, DeserializationMemo& deserializationMemo, HasCraftingLocationsAndJobsForFaction& hclaj);
	Json toJson() const;
	uint32_t getQuality() const;
	uint32_t getStep() const;
	bool operator==(const CraftJob& other){ return &other == this; }
};
inline void to_json(Json& data, const CraftJob* const& craftJob){ data = reinterpret_cast<uintptr_t>(craftJob); }
class CraftObjectiveType final : public ObjectiveType
{
	const SkillType& m_skillType; // Multiple skills are represented by CraftObjective and CraftObjectiveType, such as Leatherworking, Woodworking, Metalworking, etc.
public:
	CraftObjectiveType(const SkillType& skillType) : ObjectiveType(), m_skillType(skillType) { }
	CraftObjectiveType(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Craft; }
};
class CraftObjective final : public Objective
{
	const SkillType& m_skillType;
	CraftJob* m_craftJob;
	HasThreadedTask<CraftThreadedTask> m_threadedTask;
	std::unordered_set<CraftJob*> m_failedJobs;
public:
	CraftObjective(Actor& a, const SkillType& st);
	CraftObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	void recordFailedJob(CraftJob& craftJob) { assert(!m_failedJobs.contains(&craftJob)); m_failedJobs.insert(&craftJob); }
	std::unordered_set<CraftJob*>& getFailedJobs() { return m_failedJobs; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Craft; }
	std::string name() const { return "craft"; }
	friend class CraftThreadedTask;
	friend class HasCraftingLocationsAndJobsForFaction;
	// For testing.
	[[maybe_unused, nodiscard]] CraftJob* getCraftJob() { return m_craftJob; }
	[[maybe_unused, nodiscard]] bool hasThreadedTask() { return m_threadedTask.exists(); }

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
class HasCraftingLocationsAndJobsForFaction final
{
	const Faction& m_faction;
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<Block*>> m_locationsByCategory;
	std::unordered_map<Block*, std::unordered_set<const CraftStepTypeCategory*>> m_stepTypeCategoriesByLocation;
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<CraftJob*>> m_unassignedProjectsByStepTypeCategory;
	std::unordered_map<const SkillType*, std::unordered_set<CraftJob*>> m_unassignedProjectsBySkill;
	std::list<CraftJob> m_jobs;
public:
	HasCraftingLocationsAndJobsForFaction(const Faction& f) : m_faction(f) { }
	HasCraftingLocationsAndJobsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const Faction& f);
	Json toJson() const;
	// To be used by the player.
	void addLocation(const CraftStepTypeCategory& craftStepTypeCategory, Block& block);
	// To be used by the player.
	void removeLocation(const CraftStepTypeCategory& craftStepTypeCategory, Block& block);
	// To be used by invalidating events such as set solid.
	void maybeRemoveLocation(Block& block);
	// designate something to be crafted.
	// TODO: Quantity.
	void addJob(const CraftJobType& craftJobType, const MaterialType* materialType, uint32_t minimumSkillLevel = 0);
	// To be called by CraftStepProject::onComplete.
	void stepComplete(CraftJob& craftJob, Actor& actor);
	// To be called by CraftStepProject::onCancel.
	void stepDestroy(CraftJob& craftJob);
	// List a project as being in need of workers.
	void indexUnassigned(CraftJob& craftJob);
	// Unlist a project.
	void unindexAssigned(CraftJob& craftJob);
	// To be called when all steps are complete.
	void jobComplete(CraftJob& craftJob);
	// Generate a project step for craftJob and dispatch the worker from objective.
	void makeAndAssignStepProject(CraftJob& craftJob, Block& location, CraftObjective& objective);
	// To be used by the UI.
	[[nodiscard]] bool hasLocationsFor(const CraftJobType& craftJobType) const;
	// May return nullptr;
	CraftJob* getJobForAtLocation(const Actor& actor, const SkillType& skillType, const Block& block, std::unordered_set<CraftJob*>& excludeJobs);
	std::pair<CraftJob*, Block*> getJobAndLocationForWhileExcluding(const Actor& actor, const SkillType& skillType, std::unordered_set<CraftJob*>& excludeJobs);
	friend class CraftObjectiveType;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasJobs() const { return !m_jobs.empty(); }
	[[maybe_unused, nodiscard]] bool hasLocationsForCategory(const CraftStepTypeCategory& category) const { return m_locationsByCategory.contains(&category); }
	[[maybe_unused, nodiscard]] bool hasUnassignedProjectsForCategory(const CraftStepTypeCategory& category) const { return m_unassignedProjectsByStepTypeCategory.contains(&category); }
};
class HasCraftingLocationsAndJobs final
{
	std::unordered_map<const Faction*, HasCraftingLocationsAndJobsForFaction> m_data;
public:
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(const Faction& faction) { m_data.try_emplace(&faction, faction); }
	void removeFaction(const Faction& faction) { m_data.erase(&faction); }
	void maybeRemoveLocation(Block& location) { for(auto& pair : m_data) pair.second.maybeRemoveLocation(location); }
	[[nodiscard]] HasCraftingLocationsAndJobsForFaction& at(const Faction& faction) { assert(m_data.contains(&faction)); return m_data.at(&faction); }
};
