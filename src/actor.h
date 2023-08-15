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
#include "buckets.h"
#include "locationBuckets.h"

#include <string>
#include <vector>
#include <unordered_map>

struct MoveType;
struct AnimalSpecies;
struct Faction;

enum class CauseOfDeath { hunger, bloodLoss };

class Actor final : public HasShape
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
	Project* m_project;//We don't actually need to store this data, it's just used for asserts.
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

	Actor(uint32_t id, const std::wstring name, const AnimalSpecies& species, uint32_t percentGrown, Faction* faction, Attributes attributes);
	void setLocation(Block& block);
	void exit();
	void removeMassFromCorpse(uint32_t mass);
	void die(CauseOfDeath causeOfDeath);
	void passout(uint32_t duration);
	void doVision(const std::unordered_set<Actor*>& actors);
	bool isItem() const { return false; }
	bool isActor() const { return true; }
	bool isEnemy(Actor& actor) const;
	bool isAlly(Actor& actor) const;
	//TODO: Zombies are not sentient.
	bool isSentient() const { return m_species.sentient; }
	bool isAwake() const { return m_awake; }
	bool isInjured() const;
	bool canMove() const;
	uint32_t getMass() const;
	uint32_t getVolume() const;
	const MoveType& getMoveType() const { return m_canMove.getMoveType(); }
	uint32_t singleUnitMass() const { return getMass(); }
	// Infastructure.
	inline static std::list<Actor> data;
	static Actor& create(const AnimalSpecies& species, Block& block, uint32_t percentGrown = 100)
	{
		Attributes attributes(species, percentGrown);
		std::wstring name(species.name.begin(), species.name.end());
		name += std::to_wstring(s_nextId);
		Actor& output = data.emplace_back(
			s_nextId,
			name,
			species,
			percentGrown,
			nullptr,
			attributes
		);	
		s_nextId++;
		output.setLocation(block);
		return output;
	}
};
// To be used to find actors fitting criteria.
struct ActorQuery
{
	Actor* actor;
	uint32_t carryWeight;
	bool checkIfSentient;
	bool sentient;
	ActorQuery(Actor& a) : actor(&a) { }
	ActorQuery(uint32_t cw, bool cis, bool s) : carryWeight(cw), checkIfSentient(cis), sentient(s) { }
	bool operator()(Actor& actor) const;
	static ActorQuery makeFor(Actor& a);
	static ActorQuery makeForCarryWeight(uint32_t cw);
};
class BlockHasActors
{
	Block& m_block;
	std::vector<Actor*> m_actors;
public:
	BlockHasActors(Block& b) : m_block(b) { }
	void enter(Actor& actor);
	void exit(Actor& actor);
	void setTemperature(uint32_t temperature);
	bool canStandIn() const;
	bool contains(Actor& actor) const;
	uint32_t volumeOf(Actor& actor) const;
	std::vector<Actor*>& getAll() { return m_actors; }
	const std::vector<Actor*>& getAllConst() const { return m_actors; }
};
class AreaHasActors
{
	std::unordered_set<Actor*> m_actors;
	std::unordered_set<Actor*> m_onSurface;
	std::vector<VisionRequest> m_visionRequestQueue;
public:
	LocationBuckets m_locationBuckets;
	Buckets<Actor, Config::actorDoVisionInterval> m_visionBuckets;
	AreaHasActors(Area& a) : m_locationBuckets(a) { }
	// Add to m_actors, m_locationBuckets, m_visionBuckets and possibly m_onSurface.
	// Run after initial location has been assigned.
	void add(Actor& actor);
	void remove(Actor& actor);
	// Update temperatures for all actors on surface.
	void onChangeAmbiantSurfaceTemperature();
	// Generate vision request queue and dispatch tasks.
	void processVisionReadStep();
	void processVisionWriteStep();
	void setUnderground(Actor& actor);
	void setNotUnderground(Actor& actor);
};
