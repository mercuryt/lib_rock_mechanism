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
struct Faction;
struct DeserializationMemo;
class Area;

enum class CauseOfDeath { none, thirst, hunger, bloodLoss, wound, temperature };

struct ActorParamaters
{
	Simulation* simulation = nullptr;
	ActorId id = 0;
	const AnimalSpecies& species;
	std::wstring name = L"";
	DateTime birthDate = {0,0,0};
	Step birthStep = 0;
	Percent percentGrown = nullPercent;
	BlockIndex location = BLOCK_INDEX_MAX;
	Area* area = nullptr;
	Faction* faction = nullptr;
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
public:
	Attributes m_attributes; // 16 size in pointer equivalents
	EquipmentSet m_equipmentSet; // 13.5
	CanFight m_canFight; // 13
	ActorCanMove m_canMove; // 11.25
	HasObjectives m_hasObjectives; // 10
	Body m_body; // 8.125
	CanReserve m_canReserve; // 7
	MustSleep m_mustSleep; // 7
	//TODO: actors should be reservable.
	MustDrink m_mustDrink; // 6
	MustEat m_mustEat;  // 4.5
	ActorCanSee m_canSee; // 3.5
	CanGrow m_canGrow; // 3.5
	ActorNeedsSafeTemperature m_needsSafeTemperature; // 3.125
	ActorHasUniform m_hasUniform; // 3
	SkillSet m_skillSet; // 2
	CanPickup m_canPickup; // 2
	ActorHasStamina m_stamina; // 1.5
	const AnimalSpecies& m_species;
	Project* m_project;
	std::wstring m_name;
private:
	Step m_birthStep;
	CauseOfDeath m_causeOfDeath;
public:
	const ActorId m_id;

	Actor(Simulation& simulation, ActorId id, const std::wstring& name, const AnimalSpecies& species, Step birthStep, Percent percentGrown, Faction* faction, Attributes attributes);
	Actor(ActorParamaters params);
	Actor(const Json& data, DeserializationMemo& deserializationMemo);
	void sharedConstructor();
	void scheduleNeeds();
	void resetNeeds();
	void setLocation(BlockIndex block, Area* area = nullptr);
	void setLocationAndFacing(BlockIndex block, Facing facing, Area* area = nullptr);
	void exit();
	void removeMassFromCorpse(Mass mass);
	void die(CauseOfDeath causeOfDeath);
	void passout(Step duration);
	void leaveArea();
	void wait(Step duration);
	void takeHit(Hit& hit, BodyPart& bodyPart);
	// May be null.
	void setFaction(Faction* faction);
	void reserveAllBlocksAtLocationAndFacing(BlockIndex location, Facing facing);
	void unreserveAllBlocksAtLocationAndFacing(BlockIndex location, Facing facing);
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
	[[nodiscard]] Quantity getAgeInYears() const;
	[[nodiscard]] Step getAge() const;
	[[nodiscard]] Step getBirthStep() const { return m_birthStep; }
	[[nodiscard]] std::wstring getActionDescription() const;
	[[nodiscard]] bool allBlocksAtLocationAndFacingAreReservable(BlockIndex location, Facing facing) const;
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
