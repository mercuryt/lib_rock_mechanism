#pragma once

#include "actorOrItemIndex.h"
#include "reference.h"
#include "reservable.h"
#include "types.h"
#include <unordered_set>

class Simulation;
class ActorOrItemIndex;
struct FluidType;
struct ItemType;
struct MaterialType;
class Project;
struct ProjectRequirementCounts;
struct Faction;
class HaulSubproject;
struct DeserializationMemo;

enum class HaulStrategy { None, Individual, IndividualCargoIsCart, Team, Cart, TeamCart, Panniers, AnimalCart, StrongSentient };

HaulStrategy haulStrategyFromName(std::string);
std::string haulStrategyToName(HaulStrategy);

// Haul subproject paramaters are never stored between steps so it is safe to use Index here instead of Reference.
struct HaulSubprojectParamaters final
{
	ActorOrItemReference toHaul;
	FluidType* fluidType = nullptr;
	Quantity quantity = 0;
	HaulStrategy strategy = HaulStrategy::None;
	std::vector<ActorIndex> workers;
	ItemIndex haulTool;
	ActorIndex beastOfBurden;
	ProjectRequirementCounts* projectRequirementCounts = nullptr;
	HaulSubprojectParamaters() { reset(); }
	void reset();
	[[nodiscard, maybe_unused]] bool validate(Area& area) const;
};

// Dispatch one or more actors from a project to haul an item or actor to the project location.
// ToHaul is either an Item or an Actor.
class HaulSubproject final
{
	Project& m_project;
	ActorReferences m_workers;
	ActorOrItemReference m_toHaul;
	Quantity m_quantity = 0;
	HaulStrategy m_strategy = HaulStrategy::None;
	ActorReferences m_nonsentients;
	ItemReference m_haulTool;
	std::unordered_map<ActorReference, BlockIndex, ActorReference::Hash> m_liftPoints; // Used by Team strategy.
	ActorReference m_leader;
	bool m_itemIsMoving = false;
	ActorReference m_beastOfBurden;
	ProjectRequirementCounts& m_projectRequirementCounts;
	const ItemType* m_genericItemType;
	const MaterialType* m_genericMaterialType;
	void complete(ActorOrItemIndex delivered);
	[[nodiscard]] bool allWorkersAreAdjacentTo(ItemIndex item);
	[[nodiscard]] bool allWorkersAreAdjacentTo(ActorOrItemIndex actorOrItem);
public:
	HaulSubproject(Project& p, HaulSubprojectParamaters& paramaters);
	HaulSubproject(const Json& json, Project& m_project, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void setup();
	void commandWorker(ActorIndex actor);
	void addWorker(ActorIndex actor);
	void removeWorker(ActorIndex actor);
	void cancel();
	static HaulSubprojectParamaters tryToSetHaulStrategy(const Project& project, ActorOrItemIndex hasShape, ActorIndex worker);
	static std::vector<ActorIndex> actorsNeededToHaulAtMinimumSpeed(const Project& project, ActorIndex leader, const ActorOrItemIndex toHaul);
	[[nodiscard]] static Quantity maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(const Area& area, const ActorIndex leader, const ItemIndex haulTool, const ActorOrItemIndex toHaul, Speed minimumSpeed);
	[[nodiscard]] static Speed getSpeedWithHaulToolAndCargo(const Area& area, const ActorIndex leader, const ItemIndex haulTool, const ActorOrItemIndex toHaul, Quantity quantity);
	[[nodiscard]] static Quantity maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(const Area& area, const ActorIndex leader, ActorIndex yoked, const ItemIndex haulTool, const ActorOrItemIndex toHaul, Speed minimumSpeed);
	[[nodiscard]] static Quantity maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(const Area& area, const ActorIndex leader, const ActorIndex pannierBearer, const ItemIndex panniers, const ActorOrItemIndex toHaul, Speed minimumSpeed);
	[[nodiscard]] static Speed getSpeedWithHaulToolAndAnimal(const Area& area, const ActorIndex leader, const ActorIndex yoked, const ItemIndex haulTool, const ActorOrItemIndex toHaul, Quantity quantity);
	[[nodiscard]] static std::vector<ActorIndex> actorsNeededToHaulAtMinimumSpeedWithTool(const Project& project, ActorIndex leader, const ActorOrItemIndex toHaul, const ItemIndex haulTool);
	[[nodiscard]] static Speed getSpeedWithPannierBearerAndPanniers(const Area& area, const ActorIndex leader, const ActorIndex yoked, const ItemIndex haulTool, const ActorOrItemIndex toHaul, Quantity quantity);
	[[nodiscard]] auto& getWorkers() { return m_workers; }
	// For testing.
	[[nodiscard]] HaulStrategy getHaulStrategy() const { return m_strategy; }
	[[nodiscard]] bool getIsMoving() const { return m_itemIsMoving; }
	[[nodiscard]] bool getQuantity() const { return m_quantity; }
	[[nodiscard]] bool operator==(const HaulSubproject& other) const { return &other == this; }
	friend class Project;
};
struct HaulSubprojectDishonorCallback final : public DishonorCallback
{
	HaulSubproject& m_haulSubproject;
	HaulSubprojectDishonorCallback(HaulSubproject& hs) : m_haulSubproject(hs) { } 
	HaulSubprojectDishonorCallback(const Json data, DeserializationMemo& deserializationMemo);
	void execute([[maybe_unused]] Quantity oldCount, [[maybe_unused]] Quantity newCount) { m_haulSubproject.cancel(); }
	Json toJson() const { return {{"type", "HaulSubprojectDishonorCallback"}, {"haulSubproject", reinterpret_cast<uintptr_t>(&m_haulSubproject)}}; }
};
inline void to_json(Json& data, const HaulSubproject* const& haulSubproject){ data = reinterpret_cast<uintptr_t>(haulSubproject); }
// For Area.
class AreaHasHaulTools final
{
	//TODO: optimize with m_unreservedHaulToos and m_unreservedYolkableActors.
	ItemReferences m_haulTools;
	ActorReferences m_yolkableActors;
public:
	[[nodiscard]] bool hasToolToHaulFluid(const Area& area, FactionId faction) const;
	[[nodiscard]] ItemIndex getToolToHaulFluid(const Area& area, FactionId faction) const;
	[[nodiscard]] bool hasToolToHaulPolymorphic(const Area& area, FactionId faction, const ActorOrItemIndex hasShape) const;
	[[nodiscard]] bool hasToolToHaulItem(const Area& area, FactionId faction, ItemIndex item) const;
	[[nodiscard]] bool hasToolToHaulActor(const Area& area, FactionId faction, ActorIndex actor) const;
	[[nodiscard]] ItemIndex getToolToHaulPolymorphic(const Area& area, FactionId faction, const ActorOrItemIndex hasShape) const;
	[[nodiscard]] ItemIndex getToolToHaulItem(const Area& area, FactionId faction, ItemIndex item) const;
	[[nodiscard]] ItemIndex getToolToHaulActor(const Area& area, FactionId faction, ActorIndex actor) const;
	[[nodiscard]] ItemIndex getToolToHaulVolume(const Area& area, FactionId faction, Volume volume) const;
	[[nodiscard]] ActorIndex getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(const Area& area, FactionId faction, const ItemIndex haulTool, const Mass cargoMass, const Speed minimumHaulSpeed) const;
	[[nodiscard]] ActorIndex getPannierBearerToHaulCargoWithMassWithMinimumSpeed(const Area& area, FactionId faction, const ActorOrItemIndex hasShape, const Speed minimumHaulSpeed) const;
	[[nodiscard]] ItemIndex getPanniersForActorToHaul(const Area& area, FactionId faction, const ActorIndex actor, const ActorOrItemIndex toHaul) const;
	void registerHaulTool(const Area& area, ItemIndex item);
	void registerYokeableActor(const Area& area, ActorIndex actor);
	void unregisterHaulTool(const Area& area, ItemIndex item);
	void unregisterYokeableActor(const Area& area, ActorIndex actor);
};
