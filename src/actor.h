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
#include "equipment.h"
#include "fight.h"
#include "attributes.h"
#include "skill.h"
#include "haul.h"
#include "reservable.h"
#include "body.h"
#include "temperature.h"

#include <string>
#include <vector>
#include <unordered_map>

struct MoveType;
struct AnimalSpecies;
struct Faction;

enum class CauseOfDeath { hunger, bloodLoss };

class Actor : public HasShape
{	
	inline static uint32_t s_nextId = 1;
public:
	uint32_t m_id;
	std::wstring m_name;
	const AnimalSpecies& m_species;
	bool m_alive;
	CauseOfDeath m_causeOfDeath;
	bool m_awake;
	Body m_body;
	Faction* m_faction;
	Attributes m_attributes;
	SkillSet m_skillSet;
	MustEat m_mustEat;
	MustDrink m_mustDrink;
	MustSleep m_mustSleep;
	ActorNeedsSafeTemperature m_needsSafeTemperature;
	ActorCanMove m_canMove;
	CanFight m_canFight;
	CanPickup m_canPickup;
	CanGrow m_canGrow;
	EquipmentSet m_equipmentSet;
	HasObjectives m_hasObjectives;
	CanReserve m_canReserve;
	Reservable m_reservable;
	//TODO: CanSee.
	uint32_t m_visionRange;

	Actor(uint32_t id, const std::wstring name, const AnimalSpecies& species, uint32_t percentGrown, Faction& faction, Attributes attributes);
	void setLocation(Block& block);
	void exit();
	bool isItem() const { return false; }
	bool isActor() const { return true; }
	bool isEnemy(Actor& actor) const;
	bool isAlly(Actor& actor) const;
	bool isSentient() const;
	bool isAwake() const { return m_awake; }
	bool isInjured() const;
	bool canMove() const;
	void die(CauseOfDeath causeOfDeath);
	void passout(uint32_t duration);
	uint32_t getMass() const;
	uint32_t getVolume() const;
	void removeMassFromCorpse(uint32_t mass);
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
	std::vector<Actor*> m_actors;
public:
	HasActors(Block& b) : m_block(b) { }
	void enter(Actor& actor);
	void exit(Actor& actor);
	bool canStandIn() const;
	bool contains(Actor& actor) const;
	uint32_t volumeOf(Actor& actor) const;
	std::vector<Actor*>& getAll() { return m_actors; }
};
