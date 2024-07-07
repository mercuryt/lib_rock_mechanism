#pragma once

#include "actorOrItemIndex.h"
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

struct HaulSubprojectParamaters final
{
	ActorOrItemIndex toHaul;
	FluidType* fluidType = nullptr;
	Quantity quantity = 0;
	HaulStrategy strategy = HaulStrategy::None;
	std::vector<ActorIndex> workers;
	ItemIndex haulTool = ITEM_INDEX_MAX;
	ActorIndex beastOfBurden = ACTOR_INDEX_MAX;
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
	std::unordered_set<ActorIndex> m_workers;
	ActorOrItemIndex m_toHaul;
	Quantity m_quantity = 0;
	HaulStrategy m_strategy = HaulStrategy::None;
	std::unordered_set<ActorIndex> m_nonsentients;
	ItemIndex m_haulTool = ITEM_INDEX_MAX;
	std::unordered_map<ActorIndex, BlockIndex> m_liftPoints; // Used by Team strategy.
	ActorIndex m_leader = ACTOR_INDEX_MAX;
	bool m_itemIsMoving = false;
	ActorIndex m_beastOfBurden = ACTOR_INDEX_MAX;
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
	void removeWorker(ItemIndex item);
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
	[[nodiscard]] std::unordered_set<ActorIndex>& getWorkers() { return m_workers; }
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
	std::unordered_set<ItemIndex> m_haulTools;
	std::unordered_set<ActorIndex> m_yolkableActors;
public:
	[[nodiscard]] bool hasToolToHaulFluid(const Area& area, Faction& faction) const;
	[[nodiscard]] ItemIndex getToolToHaulFluid(const Area& area, Faction& faction) const;
	[[nodiscard]] bool hasToolToHaulPolymorphic(const Area& area, Faction& faction, const ActorOrItemIndex hasShape) const;
	[[nodiscard]] bool hasToolToHaulItem(const Area& area, Faction& faction, ItemIndex item) const;
	[[nodiscard]] bool hasToolToHaulActor(const Area& area, Faction& faction, ActorIndex actor) const;
	[[nodiscard]] ItemIndex getToolToHaulPolymorphic(const Area& area, Faction& faction, const ActorOrItemIndex hasShape) const;
	[[nodiscard]] ItemIndex getToolToHaulItem(const Area& area, Faction& faction, ItemIndex item) const;
	[[nodiscard]] ItemIndex getToolToHaulActor(const Area& area, Faction& faction, ActorIndex actor) const;
	[[nodiscard]] ItemIndex getToolToHaulVolume(const Area& area, Faction& faction, Volume volume) const;
	[[nodiscard]] ActorIndex getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(const Area& area, Faction& faction, const ItemIndex haulTool, const Mass cargoMass, const Speed minimumHaulSpeed) const;
	[[nodiscard]] ActorIndex getPannierBearerToHaulCargoWithMassWithMinimumSpeed(const Area& area, Faction& faction, const ActorOrItemIndex hasShape, const Speed minimumHaulSpeed) const;
	[[nodiscard]] ItemIndex getPanniersForActorToHaul(const Area& area, Faction& faction, const ActorIndex actor, const ActorOrItemIndex toHaul) const;
	void registerHaulTool(const Area& area, ItemIndex item);
	void registerYokeableActor(ActorIndex actor);
	void unregisterHaulTool(ItemIndex item);
	void unregisterYokeableActor(ActorIndex actor);
};
