/*
 * Represents and actor which has a shape, location, visual range, destination, and response to exposure to fluids.
 * To be inherited from in DerivedActor declaration, not to be instanced directly.
 */
#pragma once

#include "deserializationMemo.h"
#include "hasShape.h"
#include "objective.h"
#include "eat.h"
#include "drink.h"
#include "sleep.h"
#include "grow.h"
#include "move.h"
//#include "item.h"
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
#include "types.h"
#include "uniform.h"

#include <TGUI/Loading/DataIO.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

struct MoveType;
struct AnimalSpecies;
struct Faction;

enum class CauseOfDeath { none, thirst, hunger, bloodLoss, wound, temperature };

struct ActorParamaters
{
	const AnimalSpecies& species;
	Simulation* simulation = nullptr;
	ActorId id = 0;
	std::wstring name = L"";
	DateTime birthDate = {0,0,0};
	Percent percentGrown = 0;
	Block* location = nullptr;
	const Faction* faction = nullptr;
	Percent percentHunger = 0;
	bool needsEat = false;
	Percent percentTired = 0;
	bool needsSleep = false;
	Percent percentThirst = 0;
	bool needsDrink = false;

	Percent getPercentGrown();
	std::wstring getName();
	DateTime getBirthDate();
	ActorId getId();
	Percent getPercentThirst();
	Percent getPercentHunger();
	Percent getPercentTired();
};

class Actor final : public HasShape
{	
	const Faction* m_faction;
public:
	const ActorId m_id;
	std::wstring m_name;
	const AnimalSpecies& m_species;
	DateTime m_birthDate;
	bool m_alive;
	CauseOfDeath m_causeOfDeath;
	Body m_body;
	Project* m_project;
	Attributes m_attributes;
	SkillSet m_skillSet;
	MustEat m_mustEat;
	MustDrink m_mustDrink;
	MustSleep m_mustSleep;
	ActorNeedsSafeTemperature m_needsSafeTemperature;
	CanPickup m_canPickup;
	EquipmentSet m_equipmentSet;
	ActorCanMove m_canMove;
	CanFight m_canFight;
	CanGrow m_canGrow;
	HasObjectives m_hasObjectives;
	CanReserve m_canReserve;
	ActorHasStamina m_stamina;
	ActorHasUniform m_hasUniform;
	std::unordered_set<Actor*> m_canSee;
	uint32_t m_visionRange;

	Actor(Simulation& simulation, ActorId id, const std::wstring& name, const AnimalSpecies& species, DateTime birthDate, Percent percentGrown, Faction* faction, Attributes attributes);
	Actor(ActorParamaters params);
	Actor(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void setLocation(Block& block);
	void exit();
	void removeMassFromCorpse(Mass mass);
	void die(CauseOfDeath causeOfDeath);
	void passout(Step duration);
	void doVision(std::unordered_set<Actor*>& actors);
	void leaveArea();
	void wait(Step duration);
	void takeHit(Hit& hit, BodyPart& bodyPart);
	// May be null.
	void setFaction(const Faction* faction);
	void reserveAllBlocksAtLocationAndFacing(const Block& location, Facing facing);
	void unreserveAllBlocksAtLocationAndFacing(const Block& location, Facing facing);
	bool isItem() const { return false; }
	bool isActor() const { return true; }
	bool isGeneric() const { return false; }
	bool isEnemy(Actor& actor) const;
	bool isAlly(Actor& actor) const;
	//TODO: Zombies are not sentient.
	bool isSentient() const { return m_species.sentient; }
	bool isInjured() const;
	bool canMove() const;
	Mass getMass() const;
	Volume getVolume() const;
	const MoveType& getMoveType() const { return m_canMove.getMoveType(); }
	Mass singleUnitMass() const { return getMass(); }
	const Faction* getFaction() const { return m_faction; }
	uint32_t getAgeInYears() const;
	bool allBlocksAtLocationAndFacingAreReservable(const Block& location, Facing facing) const;
	// May return nullptr.
	EventSchedule& getEventSchedule();
	ThreadedTaskEngine& getThreadedTaskEngine();
	Actor(const Actor& actor) = delete;
	Actor(Actor&& actor) = delete;
	// For debugging.
	void log() const;
};
inline void to_json(Json& data, const Actor& actor){ data = actor.m_id; }
inline void to_json(Json& data, const Actor* const & actor){ data = actor->m_id; }
class BlockHasActors
{
	Block& m_block;
	std::vector<Actor*> m_actors;
public:
	BlockHasActors(Block& b) : m_block(b) { }
	void enter(Actor& actor);
	void exit(Actor& actor);
	void setTemperature(Temperature temperature);
	[[nodiscard]] bool canStandIn() const;
	[[nodiscard]] bool contains(Actor& actor) const;
	[[nodiscard]] bool empty() const { return m_actors.empty(); }
	[[nodiscard]] Volume volumeOf(Actor& actor) const;
	[[nodiscard]] std::vector<Actor*>& getAll() { return m_actors; }
	[[nodiscard]] const std::vector<Actor*>& getAll() const { return m_actors; }
	[[nodiscard]] const std::vector<Actor*>& getAllConst() const { return m_actors; }
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
	[[nodiscard]] std::unordered_set<Actor*>& getAll() { return m_actors; }
	[[nodiscard]] const std::unordered_set<Actor*>& getAllConst() const { return m_actors; }
};
