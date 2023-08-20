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
struct Faction;

enum class HaulStrategy { None, Individual, Team, Cart, TeamCart, Panniers, AnimalCart, StrongSentient };

struct HaulSubprojectParamaters final
{
	HasShape* toHaul;
	uint32_t quantity;
	Block* destination;
	HaulStrategy strategy;
	Block* toHaulLocation; // TODO: unnessesary?
	std::vector<Actor*> workers;
	Item* haulTool;
	Actor* beastOfBurden;
	HaulSubprojectParamaters() : toHaul(nullptr), quantity(0), destination(nullptr), strategy(HaulStrategy::None), toHaulLocation(nullptr), haulTool(nullptr), beastOfBurden(nullptr) { }
	bool validate() const;
};
// ToHaul is either an Item or an Actor.
class HaulSubproject final
{
	Project& m_project;
	std::unordered_set<Actor*> m_workers;
	HasShape& m_toHaul;
	uint32_t m_quantity;
	Block& m_destination;
	HaulStrategy m_strategy;
	Block* m_toHaulLocation;
	std::unordered_set<Actor*> m_nonsentients;
	Item* m_haulTool;
	std::unordered_map<Actor*, Block*> m_liftPoints; // Used by Team strategy.
	Actor* m_leader;
	bool m_itemIsMoving;
	Actor* m_beastOfBurden;
	uint32_t m_teamMemberInPlaceForHaulCount;
	void onComplete();
public:
	HaulSubproject(Project& p, HaulSubprojectParamaters& paramaters);
	void commandWorker(Actor& actor);
	void addWorker(Actor& actor);
	void removeWorker(Actor& actor);
	void cancel();
	HasShape& getToHaul() { return m_toHaul; }
	static HaulSubprojectParamaters tryToSetHaulStrategy(const Project& project, HasShape& hasShape, Actor& worker);
	static std::vector<Actor*> actorsNeededToHaulAtMinimumSpeed(const Project& proect, const Actor& leader, const HasShape& toHaul, const std::unordered_set<Actor*>& waiting);
	static uint32_t getSpeedWithHaulToolAndCargo(const Actor& leader, const Item& haulTool, const HasShape& toHaul);
	static uint32_t getSpeedWithHaulToolAndYokedAndCargo(const Actor& leader, const Actor& yoked, const Item& haulTool, const HasShape& toHaul);
	static std::vector<Actor*> actorsNeededToHaulAtMinimumSpeedWithTool(const Project& project, const Actor& leader, const HasShape& toHaul, const Item& haulTool, const std::unordered_set<Actor*>& waiting);
	static uint32_t getSpeedWithPannierBearerAndPanniers(const Actor& leader, const Actor& yoked, const Item& haulTool, const HasShape& toHaul);
	bool operator==(const HaulSubproject& other) const { return &other == this; }
	friend class Project;
};
// Used by Actor for individual haul strategy only. Other strategies use lead/follow.
class CanPickup final
{
	Actor& m_actor;
	HasShape* m_carrying;
	uint32_t m_fluidVolume;
	const FluidType* m_fluidType;
public:
	CanPickup(Actor& a) : m_actor(a), m_carrying(nullptr),  m_fluidType(nullptr) { }
	void pickUp(HasShape& hasShape, uint32_t quantity);
	void pickUp(Item& item, uint32_t quantity);
	void pickUp(Actor& actor, uint32_t quantity);
	void putDown(Block& location);
	void putDownIfAny(Block& location);
	void removeFluidVolume(uint32_t volume);
	void add(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	void remove(Item& item);
	uint32_t canPickupQuantityOf(const HasShape& hasShape) const;
	uint32_t canPickupQuantityOf(const Item& item) const;
	uint32_t canPickupQuantityOf(const Actor& actor) const;
	uint32_t canPickupQuantityOf(const ItemType& itemType, const MaterialType& materialType) const;
	uint32_t canPickupQuantityWithSingeUnitMass(uint32_t unitMass) const;
	bool canPickupAny(const HasShape& hasShape) const { return canPickupQuantityOf(hasShape) != 0; }
	bool isCarrying(const HasShape& hasShape) const { return (void*)&hasShape == (void*)&m_carrying; }
	bool isCarryingFluidType(const FluidType& fluidType) const;
	const uint32_t& getFluidVolume() const;
	const FluidType& getFluidType() const;
	bool isCarryingEmptyContainerWhichCanHoldFluid() const;
	uint32_t getMass() const;
	bool exists() const { return m_carrying != nullptr; }
	uint32_t speedIfCarryingAny(const HasShape& hasShape) const;
};
// For Area.
class HasHaulTools final
{
	//TODO: optimize with m_unreservedHaulToos and m_unreservedYolkableActors.
	std::unordered_set<Item*> m_haulTools;
	std::unordered_set<Actor*> m_yolkableActors;
public:
	bool hasToolToHaul(const Faction& faction, const HasShape& hasShape) const;
	Item* getToolToHaul(const Faction& faction, const HasShape& hasShape) const;
	Actor* getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(const Faction& faction, const Item& haulTool, const uint32_t cargoMass, const uint32_t minimumHaulSpeed) const;
	Actor* getPannierBearerToHaulCargoWithMassWithMinimumSpeed(const Faction& faction, const HasShape& hasShape, const uint32_t minimumHaulSpeed) const;
	Item* getPanniersForActorToHaul(const Faction& faction, const Actor& actor, const HasShape& toHaul) const;
	void registerHaulTool(Item& item);
	void registerYokeableActor(Actor& actor);
	void unregisterHaulTool(Item& item);
	void unregisterYokeableActor(Actor& actor);
};
