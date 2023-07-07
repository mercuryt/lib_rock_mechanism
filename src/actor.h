/*
 * Represents and actor which has a shape, location, visual range, destination, and response to exposure to fluids.
 * To be inherited from in DerivedActor declaration, not to be instanced directly.
 */
#pragma once

#include "moveType.h"
#include "hasShape.h"
#include "visionRequest.h"
#include "objective.h"
#include "eat.h"
#include "drink.h"
#include "sleep.h"
#include "grow.h"
#include "move.h"
#include "item.h"
#include "reservable.h"
#include "eqipment.h"
#include "fight.h"
#include "animalSpecies.h"
#include "attributes.h"
#include "skill.h"
#include "haul.h"

#include <string>
#include <vector>

class Actor : public HasShape
{	
	inline static uint32_t s_nextId = 1;
public:
	uint32_t m_id;
	std::string m_name;
	const AnimalSpecies& m_species;
	Attributes m_attributes;
	SkillSet m_skilSet;
	CanMove m_canMove;
	CanGrow m_canGrow;
	CanFight m_canFight;
	CanPickup m_canPickup;
	MustEat m_mustEat;
	MustDrink m_mustDrink;
	MustSleep m_mustSleep;
	EquipmentSet m_equipmentSet;
	HasObjectives m_hasObjectives;
	const MoveType* m_moveType;
	//TODO: CanSee.
	uint32_t m_visionRange;
	bool m_alive;
	Faction* m_faction;

	Actor(Block& l, const Shape& s, const MoveType& mt, uint32_t ms, uint32_t m, Faction* f);
	Actor(const Shape& s, const MoveType& mt, uint32_t ms, uint32_t m, Faction* f);
	void setLocation(Block& block);
	bool isEnemy(Actor& actor);
	bool isAlly(Actor& actor);
	void die();
	static Actor create(Faction& faction, const AnimalSpecies& species, std::vector<AttributeModifiers>& modifiers, uint32_t percentGrown);
};
// To be used to find actors fitting criteria.
class ActorQuery
{
public:
	Actor* actor;
	uint32_t carryWeight;
	bool checkIfSentient;
	bool sentient;
	bool operator()(Actor& actor) const;
	static ActorQuery makeFor(Actor& a);
	static ActorQuery makeForCarryWeight(uint32_t cw);
};
// To be used by block.
class HasActors
{
	Block& m_block;
	uint32_t m_volume;
	std::vector<Actor*> m_actors;
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::vector<std::pair<Block*, uint32_t>>>> m_moveCostsCache;
public:
	HasActors(Area& a) : m_area(a), m_volume(0) { }
	void enter(Actor& actor);
	void exit(Actor& actor);
	bool anyoneCanEnterEver() const;
	bool canEnterEverFrom(Actor& actor, Block& block) const;
	bool shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const moveType& moveType, Block& block) const;
	bool moveTypeCanEnter(const MoveType& moveType) const;
	bool canEnterCurrentlyFrom(Actor& actor, Block& block) const;
	std::vector<Block*, uint32_t> getMoveCosts(const Shape& shape, const MoveType& moveType) const;
	uint32_t moveCostFrom(const MoveType& moveType, Block& from) const;
	void clearCache();
};
