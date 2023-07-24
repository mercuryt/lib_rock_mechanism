#pragma once

#include "subproject.h"
#include "hasShape.h"

class Item;
class Actor;
class Block;
class HasShape;
struct FluidType;
struct ItemType;
struct MaterialType;

enum class HaulStrategy { None, Individual, Team, Cart, TeamCart, Panniers, AnimalCart, StrongSentient };

// ToHaul is either an Item or an Actor.
template<class ToHaul>
class HaulSubproject : Subproject
{
	ToHaul& m_toHaul;
	uint32_t m_quantity;
	Block& m_destination;
	HaulStrategy m_strategy;
	Block* m_toHaulLocation;
	std::unordered_set<Actor*> m_workers;
	std::unordered_set<Actor*> m_nonsentients;
	Item* m_haulTool;
	std::unordered_map<Actor*, Block*> m_liftPoints; // Used by Team strategy.
	uint32_t m_inplaceCount;
	Actor* m_leader;
	bool m_itemIsMoving;
	Actor* m_beastOfBurden;
	uint32_t m_minimumHaulSpeed;
	void onComplete();
public:
	HaulSubproject(ToHaul& th, Block& d, uint32_t ms, HaulStrategy hs = HaulStrategy::None) : m_toHaul(th), m_destination(d), m_minimumHaulSpeed(ms), m_strategy(hs), m_haulTool(nullptr), m_inplaceCount(0), m_itemIsMoving(false), m_leader(nullptr), m_beastOfBurden(nullptr)  { }
	void commandWorker(Actor& actor);
	bool tryToSetHaulStrategy(Actor& actor, std::unordered_set<Actor*>& waiting);
	void addWorker(Actor& actor);
	void removeWorker(Actor& actor);
	void cancel();
};
// Used by Actor for individual haul strategy only. Other strategies use lead/follow.
class CanPickup
{
	Actor& m_actor;
	HasShape* m_carrying;
	uint32_t m_fluidVolume;
	const FluidType* m_fluidType;
public:
	CanPickup(Actor& a) : m_actor(a), m_fluidType(nullptr) { }
	void pickUp(Item& item, uint32_t quantity);
	void pickUp(Actor& actor, uint32_t quantity);
	void putDown(Block& location);
	void putDownIfAny(Block& location);
	void removeFluidVolume(uint32_t volume);
	void add(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	void remove(Item& item);
	uint32_t canPickupQuantityOf(const ItemType& itemType, const MaterialType& materialType) const;
	bool isCarrying(const HasShape& hasShape) const { return (void*)&hasShape == (void*)&m_carrying; }
	bool isCarryingFluidType(const FluidType& fluidType) const;
	const uint32_t& getFluidVolume() const;
	const FluidType& getFluidType() const;
	bool isCarryingEmptyContainerWhichCanHoldFluid() const;
	uint32_t getMass() const;
};
// For Area.
class HasHaulTools
{
public:
	bool hasToolToHaul(HasShape& hasShape) const;
	Actor* getActorToYokeForHaulTool(Item& haulTool) const;
	Actor* getPannierBearerToHaul(HasShape& hasShape) const;
	Item* getPanniersForActor(Actor& actor) const;
	void registerHaulTool(Item& item);
	void registerYokeableActor(Actor& actor);
	void unregisterHaulTool(Item& item);
	void unregisterYokeableActor(Actor& actor);
};
