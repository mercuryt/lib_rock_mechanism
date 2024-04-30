#pragma once

#include "reservable.h"
#include "types.h"
#include <unordered_set>

class Item;
class Actor;
class Block;
class HasShape;
struct FluidType;
struct ItemType;
struct MaterialType;
class Project;
struct ProjectRequirementCounts;
struct Faction;
class HaulSubproject;
struct DeserializationMemo;

enum class HaulStrategy { None, Individual, Team, Cart, TeamCart, Panniers, AnimalCart, StrongSentient };

HaulStrategy haulStrategyFromName(std::string);
std::string haulStrategyToName(HaulStrategy);

struct HaulSubprojectParamaters final
{
	HasShape* toHaul;
	FluidType* fluidType;
	Quantity quantity;
	HaulStrategy strategy;
	std::vector<Actor*> workers;
	Item* haulTool;
	Actor* beastOfBurden;
	ProjectRequirementCounts* projectRequirementCounts;
	HaulSubprojectParamaters() { reset(); }
	void reset();
	[[nodiscard, maybe_unused]] bool validate() const;
};

// Dispatch one or more actors from a project to haul an item or actor to the project location.
// ToHaul is either an Item or an Actor.
class HaulSubproject final
{
	Project& m_project;
	std::unordered_set<Actor*> m_workers;
	HasShape& m_toHaul;
	Quantity m_quantity;
	HaulStrategy m_strategy;
	std::unordered_set<Actor*> m_nonsentients;
	Item* m_haulTool;
	std::unordered_map<Actor*, Block*> m_liftPoints; // Used by Team strategy.
	Actor* m_leader;
	bool m_itemIsMoving;
	Actor* m_beastOfBurden;
	ProjectRequirementCounts& m_projectRequirementCounts;
	const ItemType* m_genericItemType;
	const MaterialType* m_genericMaterialType;
	void complete(HasShape& delivered);
public:
	HaulSubproject(Project& p, HaulSubprojectParamaters& paramaters);
	HaulSubproject(const Json& json, Project& m_project, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void setup();
	void commandWorker(Actor& actor);
	void addWorker(Actor& actor);
	void removeWorker(Actor& actor);
	void cancel();
	HasShape& getToHaul() { return m_toHaul; }
	[[nodiscard]] bool allWorkersAreAdjacentTo(HasShape& hasShape);
	static HaulSubprojectParamaters tryToSetHaulStrategy(const Project& project, HasShape& hasShape, Actor& worker);
	static std::vector<Actor*> actorsNeededToHaulAtMinimumSpeed(const Project& project, Actor& leader, const HasShape& toHaul);
	[[nodiscard]] static Quantity maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(const Actor& leader, const Item& haulTool, const HasShape& toHaul, Speed minimumSpeed);
	[[nodiscard]] static Speed getSpeedWithHaulToolAndCargo(const Actor& leader, const Item& haulTool, const HasShape& toHaul, Quantity quantity);
	[[nodiscard]] static Quantity maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(const Actor& leader, Actor& yoked, const Item& haulTool, const HasShape& toHaul, Speed minimumSpeed);
	[[nodiscard]] static Quantity maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(const Actor& leader, const Actor& pannierBearer, const Item& panniers, const HasShape& toHaul, Speed minimumSpeed);
	[[nodiscard]] static Speed getSpeedWithHaulToolAndAnimal(const Actor& leader, const Actor& yoked, const Item& haulTool, const HasShape& toHaul, Quantity quantity);
	[[nodiscard]] static std::vector<Actor*> actorsNeededToHaulAtMinimumSpeedWithTool(const Project& project, Actor& leader, const HasShape& toHaul, const Item& haulTool);
	[[nodiscard]] static Speed getSpeedWithPannierBearerAndPanniers(const Actor& leader, const Actor& yoked, const Item& haulTool, const HasShape& toHaul, Quantity quantity);
	[[nodiscard]] std::unordered_set<Actor*>& getWorkers() { return m_workers; }
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
// Used by Actor for individual haul strategy only. Other strategies use lead/follow.
class CanPickup final
{
	Actor& m_actor;
	HasShape* m_carrying;
public:
	CanPickup(Actor& a) : m_actor(a), m_carrying(nullptr) { }
	CanPickup(const Json& data, Actor& a);
	Json toJson() const;
	void pickUp(HasShape& hasShape, Quantity quantity = 1u);
	void pickUp(Item& item, Quantity quantity = 1u);
	void pickUp(Actor& actor, Quantity quantity = 1u);
	// Returns a reference to has shape, which may be newly created or pre-existing due to generic items.
	HasShape& putDown(Block& location, Quantity quantity = 0u);
	HasShape* putDownIfAny(Block& location);
	void removeFluidVolume(CollisionVolume volume);
	void add(const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	void remove(Item& item);
	Item& getItem();
	Actor& getActor();
	[[nodiscard]] Quantity canPickupQuantityOf(const HasShape& hasShape) const;
	[[nodiscard]] Quantity canPickupQuantityOf(const ItemType& itemType, const MaterialType& materialType, Quantity max) const;
	[[nodiscard]] bool canPickupAny(const HasShape& hasShape) const;
	[[nodiscard]] bool canPickupAnyUnencombered(const HasShape& hasShape) const;
	[[nodiscard]] bool isCarryingAnything() const { return m_carrying != nullptr; }
	[[nodiscard]] bool isCarrying(const HasShape& hasShape) const { return &hasShape == m_carrying; }
	[[nodiscard]] bool isCarryingGeneric(const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const;
	[[nodiscard]] bool isCarryingFluidType(const FluidType& fluidType) const;
	[[nodiscard]] const CollisionVolume& getFluidVolume() const;
	[[nodiscard]] const FluidType& getFluidType() const;
	[[nodiscard]] bool isCarryingEmptyContainerWhichCanHoldFluid() const;
	[[nodiscard]] Mass getMass() const;
	[[nodiscard]] bool exists() const { return m_carrying != nullptr; }
	[[nodiscard]] Speed speedIfCarryingQuantity(const HasShape& hasShape, Quantity quantity) const;
	[[nodiscard]] Quantity maximumNumberWhichCanBeCarriedWithMinimumSpeed(const HasShape& hasShape, Speed minimumSpeed) const;
	// For UI.
	[[nodiscard]] HasShape* getCarrying(){ return m_carrying; }
};
// For Area.
class HasHaulTools final
{
	//TODO: optimize with m_unreservedHaulToos and m_unreservedYolkableActors.
	std::unordered_set<Item*> m_haulTools;
	std::unordered_set<Actor*> m_yolkableActors;
public:
	[[nodiscard]] bool hasToolToHaulFluid(const Faction& faction) const;
	[[nodiscard]] Item* getToolToHaulFluid(const Faction& faction) const;
	[[nodiscard]] bool hasToolToHaul(const Faction& faction, const HasShape& hasShape) const;
	[[nodiscard]] Item* getToolToHaul(const Faction& faction, const HasShape& hasShape) const;
	[[nodiscard]] Actor* getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(const Faction& faction, const Item& haulTool, const Mass cargoMass, const Speed minimumHaulSpeed) const;
	[[nodiscard]] Actor* getPannierBearerToHaulCargoWithMassWithMinimumSpeed(const Faction& faction, const HasShape& hasShape, const Speed minimumHaulSpeed) const;
	[[nodiscard]] Item* getPanniersForActorToHaul(const Faction& faction, const Actor& actor, const HasShape& toHaul) const;
	void registerHaulTool(Item& item);
	void registerYokeableActor(Actor& actor);
	void unregisterHaulTool(Item& item);
	void unregisterYokeableActor(Actor& actor);
};
