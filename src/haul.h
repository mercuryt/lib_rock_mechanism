#pragma once

#include "subproject.h"
#include "hasShape.h"

class Item;

enum class HaulStrategy { Individual, Team, Cart, TeamCart, Panniers, AnimalCart, StrongSentient };

template<class ToHaul>
class HaulSubproject : Subproject
{
	ToHaul& m_toHaul;
	uint32_t quantity;
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
	void onComplete();
public:
	HaulSubproject(ToHaul& th, Block& d, HaulStrategy hs) : m_toHaul(th), m_destination(d), m_strategy(hs), m_haulTool(nullptr), m_inplaceCount(0), m_itemIsMoving(false), m_leader(nullptr) { }
	void commandWorker(Actor& actor);
	bool tryToSetHaulStrategy(Actor& actor, std::unordered_set<Actor*>& waiting);
};
// Used by Actor for individual haul strategy only. Other strategies use lead/follow.
class CanPickup
{
	Actor& m_actor;
	HasShape* m_carrying;
public:
	CanPickup(Actor& a) : m_actor(a) { }
	void pickUp(Item& item, uint8_t quantity);
	void pickUp(Actor& actor);
	void putDown();
	uint32_t getCarryWeight() const;
	uint32_t canPickupQuantityOf(const ItemType& itemType) const;
};
// For Area.
class HasHaulTools
{
public:
	bool hasToolToHaul(HasShape& hasShape) const;
	Actor& getActorToYokeForHaulTool(Item& haulTool) const;
	Actor& getPannierBearerToHaul(HasShape& hasShape) const;
	Item& getPanniersForActor(Actor& actor) const;
	void registerHaulTool(Item& item);
	void registerYokeableActor(Actor& actor);
	void unregisterHaulTool(Item& item);
	void unregisterYokeableActor(Actor& actor);
};
