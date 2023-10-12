#pragma once

#include "hasShape.h"

class Item;
class Actor;
class Block;
class HasShape;
struct FluidType;
struct ItemType;
struct MaterialType;
class Project;
struct ProjectItemCounts;
class Faction;

enum class HaulStrategy { None, Individual, Team, Cart, TeamCart, Panniers, AnimalCart, StrongSentient };

struct HaulSubprojectParamaters final
{
	HasShape* toHaul;
	FluidType* fluidType;
	uint32_t quantity;
	HaulStrategy strategy;
	std::vector<Actor*> workers;
	Item* haulTool;
	Actor* beastOfBurden;
	ProjectItemCounts* projectItemCounts;
	HaulSubprojectParamaters() : toHaul(nullptr), fluidType(nullptr), quantity(0), strategy(HaulStrategy::None), haulTool(nullptr), beastOfBurden(nullptr), projectItemCounts(nullptr) { }
	[[nodiscard, maybe_unused]] bool validate() const;
};
// ToHaul is either an Item or an Actor.
class HaulSubproject final
{
	Project& m_project;
	std::unordered_set<Actor*> m_workers;
	HasShape& m_toHaul;
	uint32_t m_quantity;
	HaulStrategy m_strategy;
	Block* m_toHaulLocation;
	std::unordered_set<Actor*> m_nonsentients;
	Item* m_haulTool;
	std::unordered_map<Actor*, Block*> m_liftPoints; // Used by Team strategy.
	Actor* m_leader;
	bool m_itemIsMoving;
	Actor* m_beastOfBurden;
	uint32_t m_teamMemberInPlaceForHaulCount;
	ProjectItemCounts& m_projectItemCounts;
	void onComplete();
public:
	HaulSubproject(Project& p, HaulSubprojectParamaters& paramaters);
	void commandWorker(Actor& actor);
	void addWorker(Actor& actor);
	void removeWorker(Actor& actor);
	void cancel();
	HasShape& getToHaul() { return m_toHaul; }
	static HaulSubprojectParamaters tryToSetHaulStrategy(const Project& project, HasShape& hasShape, Actor& worker, ProjectItemCounts& projectItemCounts);
	static std::vector<Actor*> actorsNeededToHaulAtMinimumSpeed(const Project& project, Actor& leader, const HasShape& toHaul);
	[[nodiscard]] static uint32_t maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(const Actor& leader, const Item& haulTool, const HasShape& toHaul, uint32_t minimumSpeed);
	[[nodiscard]] static uint32_t getSpeedWithHaulToolAndCargo(const Actor& leader, const Item& haulTool, const HasShape& toHaul, uint32_t quantity);
	[[nodiscard]] static uint32_t maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(const Actor& leader, Actor& yoked, const Item& haulTool, const HasShape& toHaul, uint32_t minimumSpeed);
	[[nodiscard]] static uint32_t maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(const Actor& leader, const Actor& pannierBearer, const Item& panniers, const HasShape& toHaul, uint32_t minimumSpeed);
	[[nodiscard]] static uint32_t getSpeedWithHaulToolAndAnimal(const Actor& leader, const Actor& yoked, const Item& haulTool, const HasShape& toHaul, uint32_t quantity);
	[[nodiscard]] static std::vector<Actor*> actorsNeededToHaulAtMinimumSpeedWithTool(const Project& project, Actor& leader, const HasShape& toHaul, const Item& haulTool);
	[[nodiscard]] static uint32_t getSpeedWithPannierBearerAndPanniers(const Actor& leader, const Actor& yoked, const Item& haulTool, const HasShape& toHaul, uint32_t quantity);
	// For testing.
	[[nodiscard]] HaulStrategy getHaulStrategy() const { return m_strategy; }
	[[nodiscard]] bool operator==(const HaulSubproject& other) const { return &other == this; }
	friend class Project;
};
// Used by Actor for individual haul strategy only. Other strategies use lead/follow.
class CanPickup final
{
	Actor& m_actor;
	HasShape* m_carrying;
public:
	CanPickup(Actor& a) : m_actor(a), m_carrying(nullptr) { }
	void pickUp(HasShape& hasShape, uint32_t quantity);
	void pickUp(Item& item, uint32_t quantity);
	void pickUp(Actor& actor, uint32_t quantity);
	void putDown(Block& location);
	void putDownIfAny(Block& location);
	void removeFluidVolume(uint32_t volume);
	void add(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	void remove(Item& item);
	Item& getItem();
	Actor& getActor();
	[[nodiscard]] uint32_t canPickupQuantityOf(const HasShape& hasShape) const;
	[[nodiscard]] uint32_t canPickupQuantityOf(const Item& item) const;
	[[nodiscard]] uint32_t canPickupQuantityOf(const Actor& actor) const;
	[[nodiscard]] uint32_t canPickupQuantityOf(const ItemType& itemType, const MaterialType& materialType) const;
	[[nodiscard]] uint32_t canPickupQuantityWithSingeUnitMass(uint32_t unitMass) const;
	[[nodiscard]] bool canPickupAny(const HasShape& hasShape) const { return canPickupQuantityOf(hasShape) != 0; }
	[[nodiscard]] bool isCarrying(const HasShape& hasShape) const { return &hasShape == m_carrying; }
	[[nodiscard]] bool isCarryingFluidType(const FluidType& fluidType) const;
	[[nodiscard]] const uint32_t& getFluidVolume() const;
	[[nodiscard]] const FluidType& getFluidType() const;
	[[nodiscard]] bool isCarryingEmptyContainerWhichCanHoldFluid() const;
	[[nodiscard]] uint32_t getMass() const;
	[[nodiscard]] bool exists() const { return m_carrying != nullptr; }
	[[nodiscard]] uint32_t speedIfCarryingQuantity(const HasShape& hasShape, uint32_t quantity) const;
	[[nodiscard]] uint32_t maximumNumberWhichCanBeCarriedWithMinimumSpeed(const HasShape& hasShape, uint32_t minimumSpeed) const;
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
	[[nodiscard]] Actor* getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(const Faction& faction, const Item& haulTool, const uint32_t cargoMass, const uint32_t minimumHaulSpeed) const;
	[[nodiscard]] Actor* getPannierBearerToHaulCargoWithMassWithMinimumSpeed(const Faction& faction, const HasShape& hasShape, const uint32_t minimumHaulSpeed) const;
	[[nodiscard]] Item* getPanniersForActorToHaul(const Faction& faction, const Actor& actor, const HasShape& toHaul) const;
	void registerHaulTool(Item& item);
	void registerYokeableActor(Actor& actor);
	void unregisterHaulTool(Item& item);
	void unregisterYokeableActor(Actor& actor);
};
