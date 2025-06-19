#pragma once

#include "config.h"
//#include "input.h"
#include "project.h"
#include "reservable.h"
#include "objectives/craft.h"
#include "items/itemQuery.h"
#include "types.h"

#include <vector>
#include <utility>
#include <string>
#include <memory>
#include <functional>

struct CraftJob;
class HasCraftingLocationsAndJobsForFaction;
class CraftPathRequest;
class CraftStepProject;
class Area;
struct DeserializationMemo;
struct MaterialType;
struct SkillType;
class ActorOrItemIndex;
// Drill, saw, forge, etc.
class CraftStepTypeCategory final
{
	StrongVector<std::string, CraftStepTypeCategoryId> m_name;
public:
	static void create(std::string name);
	[[nodiscard]] static CraftStepTypeCategoryId size();
	[[nodiscard]] static CraftStepTypeCategoryId byName(const std::string name);
	[[nodiscard]] static std::string getName(CraftStepTypeCategoryId id);
};
inline CraftStepTypeCategory craftStepTypeCategoryData;
// Part of the definition of a particular CraftJobType which makes a specific Item.
struct CraftStepType final
{
	std::string name;
	CraftStepTypeCategoryId craftStepTypeCategory;
	SkillTypeId skillType;
	Step stepsDuration;
	std::vector<std::pair<ItemQuery, Quantity>> consumed;
	std::vector<std::pair<ItemQuery, Quantity>> unconsumed;
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> byproducts;
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
	void offDelay() { std::unreachable(); }
	void onAddToMaking(const ActorIndex& actor);
	[[nodiscard]] bool canReset() const { return false; }
	// Use copies rather then references for return types to allow specalization of Queries as well as byproduct material type.
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const;
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const;
	[[nodiscard]] std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const;
	[[nodiscard]] std::vector<ActorReference> getActors() const { return {}; }
public:
	CraftStepProject(const FactionId& faction, Area& area, const BlockIndex& location, const CraftStepType& cst, CraftJob& cj) :
		Project(faction, area, location, Quantity::create(1)), m_craftStepType(cst), m_craftJob(cj) { }
	CraftStepProject(const Json& data, DeserializationMemo& deserializationMemo, CraftJob& cj, Area& area);
	// No toJson needed here, the base class one has everything.
	[[nodiscard]] uint32_t getWorkerCraftScore(const ActorIndex& actor) const;
};
struct CraftStepProjectHasShapeDishonorCallback final : public DishonorCallback
{
	CraftStepProject& m_craftStepProject;
	CraftStepProjectHasShapeDishonorCallback(CraftStepProject& hs) : m_craftStepProject(hs) { }
	CraftStepProjectHasShapeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const { return Json({{"type", "CraftStepProjectHasShapeDishonorCallback"}, {"project", reinterpret_cast<uintptr_t>(&m_craftStepProject)}}); }
	// Craft step project cannot reset so cancel instead and allow to be recreated later.
	void execute([[maybe_unused]] const Quantity& oldCount, [[maybe_unused]] const Quantity& newCount) { m_craftStepProject.cancel(); }
};
// Data about making a specific product type.
class CraftJobType final
{
	StrongVector<std::string, CraftJobTypeId> m_name;
	StrongVector<ItemTypeId, CraftJobTypeId> m_productType;
	StrongVector<Quantity, CraftJobTypeId> m_productQuantity;
	StrongVector<MaterialCategoryTypeId, CraftJobTypeId> m_materialTypeCategory;
	StrongVector<std::vector<CraftStepType>, CraftJobTypeId> m_stepTypes;
public:
	static void create(std::string name, ItemTypeId productType, const Quantity& productQuantity, MaterialCategoryTypeId category, std::vector<CraftStepType> stepTypes);
	[[nodiscard]] static CraftJobTypeId size();
	[[nodiscard]] static CraftJobTypeId byName(const std::string name);
	[[nodiscard]] static std::string getName(const CraftJobTypeId& id);
	[[nodiscard]] static ItemTypeId getProductType(const CraftJobTypeId& id);
	[[nodiscard]] static Quantity getProductQuantity(const CraftJobTypeId& id);
	[[nodiscard]] static MaterialCategoryTypeId getMaterialTypeCategory(const CraftJobTypeId& id);
	[[nodiscard]] static std::vector<CraftStepType>& getStepTypes(const CraftJobTypeId& id);
};
inline CraftJobType craftJobTypeData;
// Make a specific product.
struct CraftJob final
{
	CraftJobTypeId craftJobType;
	HasCraftingLocationsAndJobsForFaction& hasCraftingLocationsAndJobs;
	ItemReference workPiece;
	MaterialTypeId materialType;
	std::vector<CraftStepType>::const_iterator stepIterator;
	std::unique_ptr<CraftStepProject> craftStepProject;
	uint32_t minimumSkillLevel = 0;
	uint32_t totalSkillPoints = 0;
	Reservable reservable;
	// If work piece is provided then this is an upgrade job.
	CraftJob(const CraftJobTypeId& cjt, HasCraftingLocationsAndJobsForFaction& hclaj, const ItemIndex& wp, const MaterialTypeId& mt, uint32_t msl);
	// No work piece provided is a create job.
	CraftJob(const CraftJobTypeId& cjt, HasCraftingLocationsAndJobsForFaction& hclaj, const MaterialTypeId& mt, uint32_t msl) :
		craftJobType(cjt),
		hasCraftingLocationsAndJobs(hclaj),
		materialType(mt),
		minimumSkillLevel(msl),
		totalSkillPoints(0),
		reservable(Quantity::create(1))
	{
		stepIterator = CraftJobType::getStepTypes(craftJobType).begin();
	}
	CraftJob(const Json& data, DeserializationMemo& deserializationMemo, HasCraftingLocationsAndJobsForFaction& hclaj, Area& area);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Quality getQuality() const;
	[[nodiscard]] Step getStep() const;
	[[nodiscard]] bool operator==(const CraftJob& other){ return &other == this; }
};
inline void to_json(Json& data, const CraftJob* const& craftJob){ data = reinterpret_cast<uintptr_t>(craftJob); }
// To be used by Area.
class HasCraftingLocationsAndJobsForFaction final
{
	SmallMap<CraftStepTypeCategoryId, SmallSet<BlockIndex>> m_locationsByCategory;
	SmallMap<BlockIndex, std::vector<CraftStepTypeCategoryId>> m_stepTypeCategoriesByLocation;
	SmallMap<CraftStepTypeCategoryId, std::vector<CraftJob*>> m_unassignedProjectsByStepTypeCategory;
	SmallMap<SkillTypeId, std::vector<CraftJob*>> m_unassignedProjectsBySkill;
	std::list<CraftJob> m_jobs;
	Area& m_area;
	FactionId m_faction;
public:
	HasCraftingLocationsAndJobsForFaction(const FactionId& f, Area& a) : m_area(a), m_faction(f) { }
	HasCraftingLocationsAndJobsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& f);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	// To be used by the player.
	void addLocation(CraftStepTypeCategoryId craftStepTypeCategory, const BlockIndex& block);
	// To be used by the player.
	void removeLocation(CraftStepTypeCategoryId craftStepTypeCategory, const BlockIndex& block);
	// To be used by invalidating events such as set solid.
	void maybeRemoveLocation(const BlockIndex& block);
	// designate something to be crafted.
	void addJob(const CraftJobTypeId& craftJobType, const MaterialTypeId& materialType, const Quantity& quantity, uint32_t minimumSkillLevel = 0);
	void cloneJob(CraftJob& craftJob);
	// Undo an addJob order.
	void removeJob(CraftJob& craftJob);
	// To be called by CraftStepProject::onComplete.
	void stepComplete(CraftJob& craftJob, const ActorIndex& actor);
	// To be called by CraftStepProject::onCancel.
	void stepDestroy(CraftJob& craftJob);
	// List a project as being in need of workers.
	void indexUnassigned(CraftJob& craftJob);
	// Unlist a project.
	void unindexUnassigned(CraftJob& craftJob);
	void maybeUnindexUnassigned(CraftJob& craftJob);
	// To be called when all steps are complete.
	void jobComplete(CraftJob& craftJob, const BlockIndex& location);
	// Generate a project step for craftJob and dispatch the worker from objective.
	void makeAndAssignStepProject(CraftJob& craftJob, const BlockIndex& location, CraftObjective& objective, const ActorIndex& actor);
	// To be used by the UI.
	[[nodiscard]] bool hasLocationsFor(const CraftJobTypeId& craftJobType) const;
	[[nodiscard]] std::list<CraftJob>& getAllJobs() { return m_jobs; }
	[[nodiscard]] std::vector<CraftStepTypeCategoryId>& getStepTypeCategoriesForLocation(const BlockIndex& location);
	[[nodiscard]] CraftStepTypeCategoryId getDisplayStepTypeCategoryForLocation(const BlockIndex& location);
	// May return nullptr;
	[[nodiscard]] CraftJob* getJobForAtLocation(const ActorIndex& actor, const SkillTypeId& skillType, const BlockIndex& block, const SmallSet<CraftJob*>& excludeJobs);
	friend class CraftObjectiveType;
	friend class AreaHasCraftingLocationsAndJobs;
	friend struct CraftJob;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasJobs() const { return !m_jobs.empty(); }
	[[maybe_unused, nodiscard]] bool hasLocationsForCategory(CraftStepTypeCategoryId category) const { return m_locationsByCategory.contains(category); }
	[[maybe_unused, nodiscard]] bool hasUnassignedProjectsForCategory(CraftStepTypeCategoryId category) const { return m_unassignedProjectsByStepTypeCategory.contains(category); }
};
class AreaHasCraftingLocationsAndJobs final
{
	std::unordered_map<FactionId, HasCraftingLocationsAndJobsForFaction, FactionId::Hash> m_data;
	Area& m_area;
public:
	AreaHasCraftingLocationsAndJobs(Area& area) : m_area(area) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void addFaction(const FactionId& faction) { m_data.try_emplace(faction, faction, m_area); }
	void removeFaction(const FactionId& faction) { m_data.erase(faction); }
	void maybeRemoveLocation(const BlockIndex& location) { for(auto& pair : m_data) pair.second.maybeRemoveLocation(location); }
	void clearReservations();
	[[nodiscard]] HasCraftingLocationsAndJobsForFaction& getForFaction(const FactionId& faction);
};
