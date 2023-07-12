/*
 * Represents and actor which has a shape, location, visual range, destination, and response to exposure to fluids.
 * To be inherited from in DerivedActor declaration, not to be instanced directly.
 */
#pragma once

#include "hasShape.h"
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
#include "attributes.h"
#include "skill.h"
#include "haul.h"
#include "reservable.h"

#include <string>
#include <vector>
#include <unordered_map>

class MoveType;
class AnimalSpecies;
struct Faction;

class Actor : public HasShape
{	
	inline static uint32_t s_nextId = 1;
public:
	uint32_t m_id;
	std::wstring m_name;
	const AnimalSpecies& m_species;
	bool m_alive;
	bool m_awake;
	Faction* m_faction;
	Attributes m_attributes;
	SkillSet m_skillSet;
	MustEat m_mustEat;
	MustDrink m_mustDrink;
	MustSleep m_mustSleep;
	CanMove m_canMove;
	CanFight m_canFight;
	CanPickup m_canPickup;
	CanGrow m_canGrow;
	EquipmentSet m_equipmentSet;
	HasObjectives m_hasObjectives;
	CanReserve m_canReserve;
	//TODO: CanSee.
	uint32_t m_visionRange;

	Actor(uint32_t id, const std::wstring name, const AnimalSpecies& species, uint32_t percentGrown, Faction& faction, Attributes attributes);
	void setLocation(Block& block);
	bool isEnemy(Actor& actor) const;
	bool isAlly(Actor& actor) const;
	bool isSentient() const;
	void die();
	void passout(uint32_t duration);
	static std::vector<Actor> data;
};
// To be used to find actors fitting criteria.
struct ActorQuery
{
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
	std::vector<std::pair<Actor*, int32_t>> m_actors;
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::vector<std::pair<Block*, uint32_t>>>> m_moveCostsCache;
public:
	HasActors(Block& b) : m_block(b), m_volume(0) { }
	void enter(Actor& actor);
	void exit(Actor& actor);
	void clearCache();
	bool anyoneCanEnterEver() const;
	bool canEnterEverFrom(Actor& actor, Block& block) const;
	bool canEnterEverWithFacing(Actor& actor, uint8_t facing) const;
	bool shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const MoveType& moveType, Block& block) const;
	bool shapeAndMoveTypeCanEnterEverWithFacing(const Shape& shape, const MoveType& moveType, const uint8_t facing) const;
	bool moveTypeCanEnter(const MoveType& moveType) const;
	bool canEnterCurrentlyFrom(Actor& actor, Block& block) const;
	bool canEnterCurrentlyWithFacing(Actor& actor, uint8_t facing) const;
	const std::vector<std::pair<Block*, uint32_t>>& getMoveCosts(const Shape& shape, const MoveType& moveType);
	uint32_t moveCostFrom(const MoveType& moveType, Block& from) const;
	bool canStandIn() const;
	bool contains(Actor& actor) const;
	uint32_t volumeOf(Actor& actor) const;
};
// To be composed with species to make standardized variants.
