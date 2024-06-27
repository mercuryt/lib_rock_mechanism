#pragma once

#include "config.h"
#include "input.h"
#include "pathRequest.h"
#include "project.h"
#include "reservable.h"
#include "objective.h"
#include "items/itemQuery.h"
#include "terrainFacade.h"
#include "types.h"

#include <vector>
#include <utility>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

struct CraftJob;
struct CraftJobType;
class HasCraftingLocationsAndJobsForFaction;
class CraftPathRequest;
class CraftStepProject;
class Area;
struct DeserializationMemo;
struct MaterialType;
struct SkillType;
class ActorOrItemIndex;
class CraftInputAction final : public InputAction
{
	Area& m_area;
	Faction& m_faction;
	const CraftJobType& m_craftJobType;
	const MaterialType* m_materialType;
	Quantity m_quantity = 0;
	uint32_t m_quality = 0;
	CraftInputAction(Area& area, Faction& faction, InputQueue& inputQueue, const CraftJobType& craftJobType, const MaterialType* materialType, Quantity quantity) : 
		InputAction(inputQueue), m_area(area), m_faction(faction), m_craftJobType(craftJobType), m_materialType(materialType), m_quantity(quantity) { }
	void execute();
};
class CraftCancelInputAction final : public InputAction
{
	Area& m_area;
	CraftJob& m_job;
	Faction& m_faction;
	CraftCancelInputAction(Area& area, Faction& faction, InputQueue& inputQueue, CraftJob& job) : 
		InputAction(inputQueue), m_area(area), m_job(job), m_faction(faction) { }
	void execute();
};
// Drill, saw, forge, etc.
struct CraftStepTypeCategory final
{
	const std::string name;
	// Infastructure.
	static CraftStepTypeCategory& byName(const std::string name);
};
inline std::list<CraftStepTypeCategory> craftStepTypeCategoryDataStore;
inline void to_json(Json& data, const CraftStepTypeCategory* const& category){ data = category->name; }
inline void from_json(const Json& data, const CraftStepTypeCategory*& category){ category = &CraftStepTypeCategory::byName(data.get<std::string>()); }
// Part of the definition of a particular CraftJobType which makes a specific Item.
struct CraftStepType final
{
	const std::string name;
	const CraftStepTypeCategory& craftStepTypeCategory;
	const SkillType& skillType;
	const Step stepsDuration;
	std::vector<std::pair<ItemQuery, Quantity>> consumed;
	std::vector<std::pair<ItemQuery, Quantity>> unconsumed;
	std::vector<std::tuple<const ItemType*, const MaterialType*, Quantity>> byproducts;
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
	void onAddToMaking(ActorIndex actor);
	[[nodiscard]] bool canReset() const  { return false; }
	void onHasShapeReservationDishonored(ActorOrItemIndex, Quantity, Quantity) { cancel(); }
	// Use copies rather then references for return types to allow specalization of Queries as well as byproduct material type.
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const;
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const;
	[[nodiscard]] std::vector<std::tuple<const ItemType*, const MaterialType*, Quantity>> getByproducts() const;
	[[nodiscard]] std::vector<std::pair<ActorQuery, Quantity>> getActors() const { return {}; }
public:
	CraftStepProject(Faction* faction, Area& area, BlockIndex location, const CraftStepType& cst, CraftJob& cj) : 
		Project(faction, area, location, 1), m_craftStepType(cst), m_craftJob(cj) { }
	CraftStepProject(const Json& data, DeserializationMemo& deserializationMemo, CraftJob& cj);
	// No toJson needed here, the base class one has everything.
	[[nodiscard]] uint32_t getWorkerCraftScore(const ActorIndex actor) const;
};
struct CraftStepProjectHasShapeDishonorCallback final : public DishonorCallback
{
	CraftStepProject& m_craftStepProject;
	CraftStepProjectHasShapeDishonorCallback(CraftStepProject& hs) : m_craftStepProject(hs) { } 
	CraftStepProjectHasShapeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const { return Json({{"type", "CraftStepProjectHasShapeDishonorCallback"}, {"project", reinterpret_cast<uintptr_t>(&m_craftStepProject)}}); }
	// Craft step project cannot reset so cancel instead and allow to be recreated later.
	void execute([[maybe_unused]] Quantity oldCount, [[maybe_unused]] Quantity newCount) { m_craftStepProject.cancel(); }
};
// Data about making a specific product type.
struct CraftJobType final
{
	const std::string name;
	const ItemType& productType;
	const Quantity productQuantity;
	const MaterialTypeCategory* materialTypeCategory;
	std::vector<CraftStepType> stepTypes;
	// Infastructure.
	static CraftJobType& byName(const std::string name);
};
inline std::list<CraftJobType> craftJobTypeDataStore;
inline void to_json(Json& data, const CraftJobType* const& craftJobType){ data = craftJobType->name; }
inline void to_json(Json& data, const CraftJobType& craftJobType){ data = craftJobType.name; }
inline void from_json(const Json& data, const CraftJobType*& craftJobType){ craftJobType = &CraftJobType::byName(data.get<std::string>()); }
// Make a specific product.
struct CraftJob final
{
	const CraftJobType& craftJobType;
	HasCraftingLocationsAndJobsForFaction& hasCraftingLocationsAndJobs;
	ItemIndex workPiece = ITEM_INDEX_MAX;
	const MaterialType* materialType;
	std::vector<CraftStepType>::const_iterator stepIterator;
	std::unique_ptr<CraftStepProject> craftStepProject;
	uint32_t minimumSkillLevel = 0;
	uint32_t totalSkillPoints = 0;
	Reservable reservable;
	// If work piece is provided then this is an upgrade job.
	CraftJob(const CraftJobType& cjt, HasCraftingLocationsAndJobsForFaction& hclaj, ItemIndex wp, const MaterialType* mt, uint32_t msl) : 
		craftJobType(cjt), hasCraftingLocationsAndJobs(hclaj), workPiece(wp), materialType(mt), stepIterator(craftJobType.stepTypes.begin()), minimumSkillLevel(msl), totalSkillPoints(0), reservable(1) { }
	// No work piece provided is a create job.
	CraftJob(const CraftJobType& cjt, HasCraftingLocationsAndJobsForFaction& hclaj, const MaterialType* mt, uint32_t msl) :
	       	craftJobType(cjt), hasCraftingLocationsAndJobs(hclaj), workPiece(ITEM_INDEX_MAX), materialType(mt), stepIterator(craftJobType.stepTypes.begin()), minimumSkillLevel(msl), totalSkillPoints(0), reservable(1) { }
	CraftJob(const Json& data, DeserializationMemo& deserializationMemo, HasCraftingLocationsAndJobsForFaction& hclaj);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] uint32_t getQuality() const;
	[[nodiscard]] Step getStep() const;
	[[nodiscard]] bool operator==(const CraftJob& other){ return &other == this; }
};
inline void to_json(Json& data, const CraftJob* const& craftJob){ data = reinterpret_cast<uintptr_t>(craftJob); }
// To be used by Area.
class HasCraftingLocationsAndJobsForFaction final
{
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<BlockIndex>> m_locationsByCategory;
	std::unordered_map<BlockIndex, std::unordered_set<const CraftStepTypeCategory*>> m_stepTypeCategoriesByLocation;
	std::unordered_map<const CraftStepTypeCategory*, std::unordered_set<CraftJob*>> m_unassignedProjectsByStepTypeCategory;
	std::unordered_map<const SkillType*, std::unordered_set<CraftJob*>> m_unassignedProjectsBySkill;
	std::list<CraftJob> m_jobs;
	Area& m_area;
	Faction& m_faction;
public:
	HasCraftingLocationsAndJobsForFaction(Faction& f, Area& a) : m_area(a), m_faction(f) { }
	HasCraftingLocationsAndJobsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Faction& f);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	// To be used by the player.
	void addLocation(const CraftStepTypeCategory& craftStepTypeCategory, BlockIndex block);
	// To be used by the player.
	void removeLocation(const CraftStepTypeCategory& craftStepTypeCategory, BlockIndex block);
	// To be used by invalidating events such as set solid.
	void maybeRemoveLocation(BlockIndex block);
	// designate something to be crafted.
	void addJob(const CraftJobType& craftJobType, const MaterialType* materialType, Quantity quantity, uint32_t minimumSkillLevel = 0);
	void cloneJob(CraftJob& craftJob);
	// Undo an addJob order.
	void removeJob(CraftJob& craftJob);
	// To be called by CraftStepProject::onComplete.
	void stepComplete(CraftJob& craftJob, ActorIndex actor);
	// To be called by CraftStepProject::onCancel.
	void stepDestroy(CraftJob& craftJob);
	// List a project as being in need of workers.
	void indexUnassigned(CraftJob& craftJob);
	// Unlist a project.
	void unindexAssigned(CraftJob& craftJob);
	// To be called when all steps are complete.
	void jobComplete(CraftJob& craftJob, BlockIndex location);
	// Generate a project step for craftJob and dispatch the worker from objective.
	void makeAndAssignStepProject(CraftJob& craftJob, BlockIndex location, CraftObjective& objective);
	// To be used by the UI.
	[[nodiscard]] bool hasLocationsFor(const CraftJobType& craftJobType) const;
	[[nodiscard]] std::list<CraftJob>& getAllJobs() { return m_jobs; }
	[[nodiscard]] std::unordered_set<const CraftStepTypeCategory*>& getStepTypeCategoriesForLocation(BlockIndex location);
	[[nodiscard]] const CraftStepTypeCategory* getDisplayStepTypeCategoryForLocation(BlockIndex location);
	// May return nullptr;
	[[nodiscard]] CraftJob* getJobForAtLocation(const ActorIndex actor, const SkillType& skillType, BlockIndex block, std::unordered_set<CraftJob*>& excludeJobs);
	friend class CraftObjectiveType;
	friend class AreaHasCraftingLocationsAndJobs;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasJobs() const { return !m_jobs.empty(); }
	[[maybe_unused, nodiscard]] bool hasLocationsForCategory(const CraftStepTypeCategory& category) const { return m_locationsByCategory.contains(&category); }
	[[maybe_unused, nodiscard]] bool hasUnassignedProjectsForCategory(const CraftStepTypeCategory& category) const { return m_unassignedProjectsByStepTypeCategory.contains(&category); }
};
class AreaHasCraftingLocationsAndJobs final
{
	std::unordered_map<Faction*, HasCraftingLocationsAndJobsForFaction> m_data;
	Area& m_area;
public:
	AreaHasCraftingLocationsAndJobs(Area& area) : m_area(area) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void addFaction(Faction& faction) { m_data.try_emplace(&faction, faction, m_area); }
	void removeFaction(Faction& faction) { m_data.erase(&faction); }
	void maybeRemoveLocation(BlockIndex location) { for(auto& pair : m_data) pair.second.maybeRemoveLocation(location); }
	void clearReservations();
	[[nodiscard]] HasCraftingLocationsAndJobsForFaction& at(Faction& faction);
};
