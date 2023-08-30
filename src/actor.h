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
//#include "item.h"
#include "reservable.h"
#include "equipment.h"
#include "fight.h"
#include "attributes.h"
#include "skill.h"
#include "haul.h"
#include "body.h"
#include "temperature.h"
#include "buckets.h"
#include "locationBuckets.h"
#include "stamina.h"

#include <string>
#include <vector>
#include <unordered_map>

struct MoveType;
struct AnimalSpecies;
struct Faction;

enum class CauseOfDeath { thirst, hunger, bloodLoss };

class Actor final : public HasShape
{	
	Simulation& m_simulation;
	const Faction* m_faction;
public:
	const uint32_t m_id;
	std::wstring m_name;
	const AnimalSpecies& m_species;
	bool m_alive;
	CauseOfDeath m_causeOfDeath;
	bool m_awake;
	Body m_body;
	Project* m_project;//We don't actually need to store this data, it's just used for asserts.
	Attributes m_attributes;
	SkillSet m_skillSet;
	EquipmentSet m_equipmentSet;
	MustEat m_mustEat;
	MustDrink m_mustDrink;
	MustSleep m_mustSleep;
	ActorNeedsSafeTemperature m_needsSafeTemperature;
	CanPickup m_canPickup;
	ActorCanMove m_canMove;
	CanFight m_canFight;
	CanGrow m_canGrow;
	HasObjectives m_hasObjectives;
	CanReserve m_canReserve;
	Reservable m_reservable;
	ActorHasStamina m_stamina;
	std::unordered_set<Actor*> m_canSee;
	uint32_t m_visionRange;

	Actor(Simulation& simulation, uint32_t id, const std::wstring name, const AnimalSpecies& species, uint32_t percentGrown, Faction* faction, Attributes attributes);
	void setLocation(Block& block);
	void exit();
	void removeMassFromCorpse(uint32_t mass);
	void die(CauseOfDeath causeOfDeath);
	void passout(uint32_t duration);
	void doVision(std::unordered_set<Actor*>& actors);
	// May be null.
	void setFaction(const Faction* faction) { m_faction = faction; m_canReserve.setFaction(faction); }
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
	const Faction* getFaction() const { return m_faction; }
	Simulation& getSimulation() {return m_simulation; }
	EventSchedule& getEventSchedule();
	ThreadedTaskEngine& getThreadedTaskEngine();
	Actor(const Actor& actor) = delete;
	Actor(Actor&& actor) = delete;
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
	const std::vector<Actor*>& getAll() const { return m_actors; }
	const std::vector<Actor*>& getAllConst() const { return m_actors; }
};
class AreaHasActors
{
	Area& m_area;
	std::unordered_set<Actor*> m_actors;
	std::unordered_set<Actor*> m_onSurface;
	std::vector<VisionRequest> m_visionRequestQueue;
public:
	LocationBuckets m_locationBuckets;
	Buckets<Actor, Config::actorDoVisionInterval> m_visionBuckets;
	AreaHasActors(Area& a) : m_area(a), m_locationBuckets(a) { }
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
	~AreaHasActors() { m_visionRequestQueue.clear(); }
};
