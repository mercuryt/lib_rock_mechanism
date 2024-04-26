/*
 * Represents and actor which has a shape, location, visual range, destination, and response to exposure to fluids.
 */
#pragma once

#include "datetime.h"
#include "hasShape.h"
#include "objective.h"
#include "eat.h"
#include "drink.h"
#include "sleep.h"
#include "move.h"
#include "equipment.h"
#include "fight.h"
#include "attributes.h"
#include "skill.h"
#include "haul.h"
#include "body.h"
#include "temperature.h"
#include "types.h"
#include "uniform.h"
#include "actor/canSee.h"
#include "actor/grow.h"
#include "actor/stamina.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

struct MoveType;
struct AnimalSpecies;
class Faction;
struct DeserializationMemo;
class Area;

enum class CauseOfDeath { none, thirst, hunger, bloodLoss, wound, temperature };

struct ActorParamaters
{
	const AnimalSpecies& species;
	Simulation* simulation = nullptr;
	ActorId id = 0;
	std::wstring name = L"";
	DateTime birthDate = {0,0,0};
	Step birthStep = 0;
	Percent percentGrown = nullPercent;
	Block* location = nullptr;
	const Faction* faction = nullptr;
	Percent percentHunger = nullPercent;
	bool needsEat = false;
	Percent percentTired = nullPercent;
	bool needsSleep = false;
	Percent percentThirst = nullPercent;
	bool needsDrink = false;
	bool hasCloths = true;
	bool hasSidearm = false;
	bool hasLongarm = false;
	bool hasRangedWeapon = false;
	bool hasLightArmor = false;
	bool hasHeavyArmor = false;

	Percent getPercentGrown();
	std::wstring getName();
	Step getBirthStep();
	ActorId getId();
	Percent getPercentThirst();
	Percent getPercentHunger();
	Percent getPercentTired();
	void generateEquipment(Actor& actor);
};

class Actor final : public HasShape
{	
	const Faction* m_faction;
	Step m_birthStep;
	CauseOfDeath m_causeOfDeath;
public:
	const ActorId m_id;
	std::wstring m_name;
	const AnimalSpecies& m_species;
	Body m_body;
	Project* m_project;
	ActorCanSee m_canSee;
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

	Actor(Simulation& simulation, ActorId id, const std::wstring& name, const AnimalSpecies& species, Step birthStep, Percent percentGrown, Faction* faction, Attributes attributes);
	Actor(ActorParamaters params);
	Actor(const Json& data, DeserializationMemo& deserializationMemo);
	void setLocation(Block& block);
	void exit();
	void removeMassFromCorpse(Mass mass);
	void die(CauseOfDeath causeOfDeath);
	void passout(Step duration);
	void leaveArea();
	void wait(Step duration);
	void takeHit(Hit& hit, BodyPart& bodyPart);
	// May be null.
	void setFaction(const Faction* faction);
	void reserveAllBlocksAtLocationAndFacing(const Block& location, Facing facing);
	void unreserveAllBlocksAtLocationAndFacing(const Block& location, Facing facing);
	void setBirthStep(Step step);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool isItem() const { return false; }
	[[nodiscard]] bool isActor() const { return true; }
	[[nodiscard]] bool isGeneric() const { return false; }
	[[nodiscard]] bool isAlive() const { return m_causeOfDeath == CauseOfDeath::none; }
	[[nodiscard]] CauseOfDeath getCauseOfDeath() const { assert(!isAlive()); return m_causeOfDeath; }
	[[nodiscard]] bool isEnemy(Actor& actor) const;
	[[nodiscard]] bool isAlly(Actor& actor) const;
	//TODO: Zombies are not sentient.
	[[nodiscard]] bool isSentient() const { return m_species.sentient; }
	[[nodiscard]] bool isInjured() const;
	[[nodiscard]] bool canMove() const;
	[[nodiscard]] Mass getMass() const;
	[[nodiscard]] Volume getVolume() const;
	[[nodiscard]] const MoveType& getMoveType() const { return m_canMove.getMoveType(); }
	[[nodiscard]] Mass singleUnitMass() const { return getMass(); }
	// May return nullptr.
	[[nodiscard]] const Faction* getFaction() const { return m_faction; }
	[[nodiscard]] Quantity getAgeInYears() const;
	[[nodiscard]] Step getAge() const;
	[[nodiscard]] Step getBirthStep() const { return m_birthStep; }
	[[nodiscard]] std::wstring getActionDescription() const;
	[[nodiscard]] bool allBlocksAtLocationAndFacingAreReservable(const Block& location, Facing facing) const;
	[[nodiscard]] EventSchedule& getEventSchedule();
	[[nodiscard]] ThreadedTaskEngine& getThreadedTaskEngine();
	Actor(const Actor& actor) = delete;
	Actor(Actor&& actor) = delete;
	// For debugging.
	void log() const;
	void satisfyNeeds();
};
inline void to_json(Json& data, const Actor& actor){ data = actor.m_id; }
inline void to_json(Json& data, const Actor* const & actor){ data = actor->m_id; }
