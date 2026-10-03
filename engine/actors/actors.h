#pragma once

#include "../actorOrItemIndex.h"
#include "../body.h"
#include "../safeTemperature.h"
#include "../dataStructures/strongVector.h"
#include "../datetime.h"
#include "../definitions/attackType.h"
#include "../equipment.h"
#include "../geometry/cuboidSet.h"
#include "../numericTypes/index.h"
#include "../numericTypes/types.h"
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../path/areaHasPaths.h"
#include "../portables.h"
#include "../reference.h"
#include "../uniform.h"
#include "../vision/visionRequests.h"
#include "../psycology/psycology.h"
#include "../onSight.h"
#include "../attackTable.h"
#include "uniform.h"
#include "skill.h"
#include "soldier.h"
#include <memory>

class Project;
class AttackCoolDownEvent;
class GetIntoAttackPositionPathRequest;
class MustSleep;
class MustDrink;
class MustEat;
class CanGrow;
class ActorNeedsSafeTemperature;
class SkillSet;
class WanderObjective;
class DrinkObjective;
class ThirstEvent;

enum class CauseOfDeath { none, thirst, hunger, bloodLoss, wound, temperature };

struct ActorParamaters
{
	ActorId id = ActorId::null();
	AnimalSpeciesId species;
	std::string name = "";
	DateTime birthDate = {0,0,0};
	Step birthStep = Step::null();
	Percent percentGrown = Percent::null();
	Point3D location = {};
	ActorIndex mountedOn = ActorIndex::null();
	Facing4 facing = Facing4::Null;
	FactionId faction = FactionId::null();
	Percent percentHunger = Percent::null();
	bool needsEat = false;
	Percent percentTired = Percent::null();
	bool needsSleep = false;
	Percent percentThirst = Percent::null();
	bool needsDrink = false;
	bool hasCloths = true;
	bool hasSidearm = false;
	bool hasLongarm = false;
	bool hasRangedWeapon = false;
	bool hasLightArmor = false;
	bool hasHeavyArmor = false;
	bool piloting = false;

	Percent getPercentGrown(Simulation& simulation);
	std::string getName(Simulation& simulation);
	Step getBirthStep(Simulation& simulation);
	ActorId getId(Simulation& simulation);
	Percent getPercentThirst(Simulation& simulation);
	Percent getPercentHunger(Simulation& simulation);
	Percent getPercentTired(Simulation& simulation);
	void generateEquipment(Area& area, ActorIndex actor);
};
class Actors final : public Portables<Actors, ActorIndex, ActorReferenceIndex, true>
{
	StrongVector<ActorId, ActorIndex> m_id;
	StrongVector<std::string, ActorIndex> m_name;
	StrongVector<AnimalSpeciesId, ActorIndex> m_species;
	StrongVector<Project*, ActorIndex> m_project;
	StrongVector<Step, ActorIndex> m_birthStep;
	StrongVector<Step, ActorIndex> m_deathStep;
	StrongVector<CauseOfDeath, ActorIndex> m_causeOfDeath;
	StrongVector<AttributeLevel, ActorIndex> m_strength;
	StrongVector<AttributeLevelBonusOrPenalty, ActorIndex> m_strengthBonusOrPenalty;
	StrongVector<float, ActorIndex> m_strengthModifier;
	StrongVector<AttributeLevel, ActorIndex> m_agility;
	StrongVector<AttributeLevelBonusOrPenalty, ActorIndex> m_agilityBonusOrPenalty;
	StrongVector<float, ActorIndex> m_agilityModifier;
	StrongVector<AttributeLevel, ActorIndex> m_dextarity;
	StrongVector<AttributeLevelBonusOrPenalty, ActorIndex> m_dextarityBonusOrPenalty;
	StrongVector<float, ActorIndex> m_dextarityModifier;
	StrongVector<int, ActorIndex> m_adultHeight;
	StrongVector<Mass, ActorIndex> m_mass;
	StrongVector<int, ActorIndex> m_massBonusOrPenalty;
	StrongVector<float, ActorIndex> m_massModifier;
	StrongVector<Mass, ActorIndex> m_unencomberedCarryMass;
	StrongVector<SmallSet<Point3D>, HasShapeIndex> m_leadFollowPath;
	StrongVector<std::unique_ptr<HasObjectives>, ActorIndex> m_hasObjectives;
	StrongVector<std::unique_ptr<Body>, ActorIndex> m_body;
	StrongVector<std::unique_ptr<MustSleep>, ActorIndex> m_mustSleep;
	StrongVector<std::unique_ptr<MustDrink>, ActorIndex> m_mustDrink;
	StrongVector<std::unique_ptr<MustEat>, ActorIndex> m_mustEat;
	StrongVector<std::unique_ptr<ActorNeedsSafeTemperature>, ActorIndex> m_needsSafeTemperature;
	StrongVector<std::unique_ptr<CanGrow>, ActorIndex> m_canGrow;
	StrongVector<SkillSet, ActorIndex> m_skillSet;
	StrongVector<std::unique_ptr<CanReserve>, ActorIndex> m_canReserve;
	StrongVector<std::unique_ptr<ActorHasUniform>, ActorIndex> m_hasUniform;
	StrongVector<std::unique_ptr<EquipmentSet>, ActorIndex> m_equipmentSet;
	// CanPickUp.
	// TODO: Should be a reference?
	StrongVector<ActorOrItemIndex, ActorIndex> m_carrying;
	// Stamina.
	StrongVector<Stamina, ActorIndex> m_stamina;
	// Vision.
	StrongVector<SmallSet<ActorReference>, ActorIndex> m_canSee;
	StrongVector<SmallSet<ActorReference>, ActorIndex> m_canBeSeenBy;
	StrongVector<Distance, ActorIndex> m_visionRange;
	StrongVector<HasOnSight, ActorIndex> m_onSight;
	// Combat.
	HasScheduledEvents<AttackCoolDownEvent, ActorIndex> m_coolDownEvent;
	StrongVector<AttackTable, ActorIndex> m_meleeAttackTable;
	StrongVector<AttackTable, ActorIndex> m_meleeAttackTableNonLethal;
	StrongVector<SmallSet<ActorIndex>, ActorIndex> m_targetedBy;
	StrongVector<ActorIndex, ActorIndex> m_target;
	StrongVector<Step, ActorIndex> m_onMissCoolDownMelee;
	StrongVector<DistanceFractional, ActorIndex> m_maxMeleeRange;
	StrongVector<DistanceFractional, ActorIndex> m_maxMeleeRangeNonLethal;
	StrongVector<DistanceFractional, ActorIndex> m_maxRange;
	StrongVector<float, ActorIndex> m_coolDownDurationModifier;
	StrongVector<CombatScore, ActorIndex> m_combatScore;
	StrongVector<CombatScore, ActorIndex> m_combatScoreNonLethal;
	StrongVector<SoldierData, ActorIndex> m_soldier;
	// Move.
	HasScheduledEvents<MoveEvent, ActorIndex> m_moveEvent;
	StrongVector<PathRequest*, ActorIndex> m_pathRequest;
	// Path is stored backwards, with the first point being the destination and the last being the next step.
	StrongVector<SmallSet<Point3D>, ActorIndex> m_path;
	StrongVector<Point3D, ActorIndex> m_destination;
	StrongVector<Speed, ActorIndex> m_speedIndividual;
	StrongVector<Speed, ActorIndex> m_speedActual;
	StrongVector<int, ActorIndex> m_moveRetries;
	StrongVector<Psycology, ActorIndex> m_psycology;
	StrongVector<std::pair<std::string, Step>, ActorIndex> m_dialog;
	StrongVector<ExpeditionId, ActorIndex> m_expedition;
	// Is piloting current m_isOnDeckOf
	StrongBitSet<ActorIndex> m_isPilot;
	void moveIndex(ActorIndex oldIndex, ActorIndex newIndex);
public:
	Actors(Area& area);
	void load(const Json& data);
	void loadObjectivesAndReservations(const Json& data);
	void onChangeAmbiantSurfaceTemperature(Temperature newAmbiant, const CuboidSet& exclude);
	template<typename Action>
	void forEachData(Action&& action)
	{
		forEachDataPortables(action);
		action(m_id);
		action(m_name);
		action(m_species);
		action(m_project);
		action(m_birthStep);
		action(m_deathStep);
		action(m_causeOfDeath);
		action(m_strength);
		action(m_strengthBonusOrPenalty);
		action(m_strengthModifier);
		action(m_agility);
		action(m_agilityBonusOrPenalty);
		action(m_agilityModifier);
		action(m_dextarity);
		action(m_dextarityBonusOrPenalty);
		action(m_dextarityModifier);
		action(m_adultHeight);
		action(m_mass);
		action(m_massBonusOrPenalty);
		action(m_massModifier);
		action(m_unencomberedCarryMass);
		action(m_leadFollowPath);
		action(m_hasObjectives);
		action(m_body);
		action(m_mustSleep);
		action(m_mustDrink);
		action(m_mustEat);
		action(m_needsSafeTemperature);
		action(m_canGrow);
		action(m_skillSet);
		action(m_canReserve);
		action(m_hasUniform);
		action(m_equipmentSet);
		action(m_carrying);
		action(m_stamina);
		action(m_canSee);
		action(m_canBeSeenBy);
		action(m_visionRange);
		action(m_onSight);
		action(m_coolDownEvent);
		action(m_meleeAttackTable);
		action(m_meleeAttackTableNonLethal);
		action(m_targetedBy);
		action(m_target);
		action(m_onMissCoolDownMelee);
		action(m_maxMeleeRange);
		action(m_maxMeleeRangeNonLethal);
		action(m_maxRange);
		action(m_coolDownDurationModifier);
		action(m_combatScore);
		action(m_combatScoreNonLethal);
		action(m_soldier);
		action(m_moveEvent);
		action(m_pathRequest);
		action(m_path);
		action(m_destination);
		action(m_speedIndividual);
		action(m_speedActual);
		action(m_moveRetries);
		action(m_psycology);
		action(m_dialog);
		action(m_expedition);
		action(m_isPilot);
	}
	ActorIndex create(ActorParamaters params);
	void remove(ActorIndex index);
	void sharedConstructor(ActorIndex index);
	void scheduleNeeds(ActorIndex index);
	void resetNeeds(ActorIndex index);
	void location_set(ActorIndex index, Point3D point, Facing4 facing);
	void location_setStatic(ActorIndex index, Point3D point, Facing4 facing);
	void location_setDynamic(ActorIndex index, Point3D point, Facing4 facing);
	// Used when item already has a location, rolls back position on failure.
	SetLocationAndFacingResult location_tryToMoveToStatic(ActorIndex index, Point3D point);
	SetLocationAndFacingResult location_tryToMoveToDynamic(ActorIndex index, Point3D point);
	// Used when item does not have a location.
	SetLocationAndFacingResult location_tryToSet(ActorIndex index, Point3D point, Facing4 facing);
	SetLocationAndFacingResult location_tryToSetStatic(ActorIndex index, Point3D point, Facing4 facing);
	SetLocationAndFacingResult location_tryToSetDynamic(ActorIndex index, Point3D point, Facing4 facing);
	void location_clear(ActorIndex index);
	void location_clearStatic(ActorIndex index);
	void location_clearDynamic(ActorIndex index);
	[[nodiscard]] bool location_canEnterEverWithFacing(ActorIndex index, Point3D point, Facing4 facing);
	[[nodiscard]] bool location_canEnterEverWithAnyFacing(ActorIndex index, Point3D point);
	[[nodiscard]] Facing4 location_canEnterEverWithAnyFacingReturnFacing(ActorIndex index, Point3D point);
	void removeMassFromCorpse(ActorIndex index, Mass  mass);
	void die(ActorIndex index, CauseOfDeath causeOfDeath);
	void passout(ActorIndex index, Step duration);
	// Used for when an actor leaves the area and ceases to exist, not when they move to an expedition.
	void leaveArea(ActorIndex index);
	void wait(ActorIndex index, Step duration);
	void takeHit(ActorIndex index, Hit& hit, BodyPart& bodyPart);
	// May be null.
	void setFaction(ActorIndex index, FactionId faction);
	void setBirthStep(ActorIndex index, Step step);
	void setName(ActorIndex index, std::string name){ m_name[index] = name; }
	void takeFallDamage(ActorIndex index, Distance distance, MaterialTypeId materialType);
	void resetMoveType(ActorIndex index);
	bool tryToMoveSoAsNotOccuping(ActorIndex index, Point3D point);
	ActorIndex moveTo(Actors& other, ActorIndex index);
	[[nodiscard]] SmallSet<ActorIndex> getAll() const;
	[[nodiscard]] ActorIndex getRandom() const;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ActorId getId(ActorIndex index) const { return m_id[index]; }
	[[nodiscard]] std::string getName(ActorIndex index) const { return m_name[index]; }
	[[nodiscard]] bool isAlive(ActorIndex index) const { return m_causeOfDeath[index] == CauseOfDeath::none; }
	[[nodiscard]] Percent getPercentGrown(ActorIndex index) const;
	[[nodiscard]] CauseOfDeath getCauseOfDeath(ActorIndex index) const { assert(!isAlive(index)); return m_causeOfDeath[index]; }
	[[nodiscard]] bool isEnemy(ActorIndex actor, ActorIndex other) const;
	[[nodiscard]] Point3D getNearestVisibleEnemyLocation(ActorIndex actor) const;
	[[nodiscard]] bool isAlly(ActorIndex actor, ActorIndex other) const;
	[[nodiscard]] bool isSentient(ActorIndex index) const;
	[[nodiscard]] bool isInjured(ActorIndex index) const;
	[[nodiscard]] bool canMove(ActorIndex index) const;
	[[nodiscard]] FullDisplacement getVolume(ActorIndex index) const;
	[[nodiscard]] Mass getMass(ActorIndex index) const;
	[[nodiscard]] Quantity getAgeInYears(ActorIndex index) const;
	[[nodiscard]] Step getAge(ActorIndex index) const;
	[[nodiscard]] Step getBirthStep(ActorIndex index) const { return m_birthStep[index]; }
	[[nodiscard]] Step getDeathStep(ActorIndex index) const { return m_deathStep[index]; }
	[[nodiscard]] std::string getActionDescription(ActorIndex index) const;
	[[nodiscard]] AnimalSpeciesId getSpecies(ActorIndex index) const { return m_species[index]; }
	[[nodiscard]] Mass getUnencomberedCarryMass(ActorIndex index) const { return m_unencomberedCarryMass[index]; }
	[[nodiscard]] Point3D getCombinedLocation(ActorIndex index) const;
	[[nodiscard]] ActorOrItemIndex getIsPiloting(ActorIndex index) const;
	[[nodiscard]] bool canSeeEnemy(ActorIndex index) const;
	// -Stamina.
	void stamina_recover(ActorIndex index);
	void stamina_spend(ActorIndex index, const Stamina stamina);
	void stamina_setFull(ActorIndex index);
	bool stamina_hasAtLeast(ActorIndex index, const Stamina stamina) const;
	bool stamina_isFull(ActorIndex index) const;
	bool stamina_empty(ActorIndex index) const;
	[[nodiscard]] Stamina stamina_getMax(ActorIndex index) const;
	[[nodiscard]] Stamina stamina_get(ActorIndex index) const { return m_stamina[index]; }
	// -Vision.
	void vision_createRequestIfCanSee(ActorIndex index);
	void vision_clearRequestIfExists(ActorIndex index);
	void vision_setCanSee(ActorIndex index, const ActorReference other);
	void vision_setCanBeSeenBy(ActorIndex index, const ActorReference other);
	void vision_setCanSee(ActorIndex index, SmallSet<ActorReference>&& others);
	void vision_setCanBeSeenBy(ActorIndex index, SmallSet<ActorReference>&& others);
	void vision_setNoLongerCanSee(ActorIndex index, const ActorReference other);
	void vision_setNoLongerCanBeSeenBy(ActorIndex index, const ActorReference other);
	void vision_clearCanSee(ActorIndex index);
	void vision_maybeUpdateRange(ActorIndex index, Distance range);
	void vision_maybeUpdateLocation(ActorIndex index, Point3D location);
	void vision_removeOpaqueFromCuboidSet(const CuboidSet& cuboidSet) const;
	[[nodiscard]] SmallSet<ActorReference>& vision_getCanSee(ActorIndex index) { return m_canSee[index]; }
	[[nodiscard]] SmallSet<ActorReference>& vision_getCanBeSeenBy(ActorIndex index) { return m_canBeSeenBy[index]; }
	[[nodiscard]] Distance vision_getRange(ActorIndex index) const { return m_visionRange[index]; }
	[[nodiscard]] DistanceSquared vision_getRangeSquared(ActorIndex index) const { return m_visionRange[index].squared(); }
	[[nodiscard]] bool vision_canSeeActor(ActorIndex index, ActorIndex other) const;
	[[nodiscard]] bool vision_canSeeAnything(ActorIndex index) const;
	[[nodiscard]] bool vision_canSeeEnemy(ActorIndex index) const;
	// -OnSight
	[[nodiscard]] HasOnSight& onSight_get(ActorIndex index) { return m_onSight[index]; }
	[[nodiscard]] const HasOnSight& onSight_get(ActorIndex index) const { return m_onSight[index]; }
	// -Combat.
	void combat_attackMeleeRange(ActorIndex index, ActorIndex target);
	// Return cooldown for this attack so ConfrontationObjective or another objective or drama arc can manage it and run code before/after hits.
	Step combat_attackMeleeRangeNonLethal(ActorIndex index, ActorIndex target);
	void combat_attackLongRange(ActorIndex index, ActorIndex target, ItemIndex weapon, ItemIndex ammo);
	void combat_coolDownCompleted(ActorIndex index);
	void combat_update(ActorIndex index);
	void combat_setTarget(ActorIndex index, ActorIndex actor);
	void combat_recordTargetedBy(ActorIndex index, ActorIndex actor);
	void combat_removeTargetedBy(ActorIndex index, ActorIndex actor);
	void combat_onMoveFrom(ActorIndex index, Point3D previous);
	void combat_onDeath(ActorIndex index);
	void combat_onLeaveArea(ActorIndex index);
	void combat_noLongerTargetable(ActorIndex index);
	void combat_targetNoLongerTargetable(ActorIndex index);
	void combat_onTargetMoved(ActorIndex index);
	void combat_freeHit(ActorIndex index, ActorIndex actor);
	// TODO: Combat vs. items?
	void combat_getIntoRangeAndLineOfSightOfActor(ActorIndex index, ActorIndex target, const DistanceFractional range);
	void combat_flee(ActorIndex index);
	[[nodiscard]] bool combat_isFleeing(ActorIndex index);
	[[nodiscard]] CombatScore combat_getCurrentMeleeCombatScore(ActorIndex index);
	[[nodiscard]] bool combat_isOnCoolDown(ActorIndex index) const;
	[[nodiscard]] bool combat_inRange(ActorIndex index, ActorIndex target) const;
	[[nodiscard]] bool combat_doesProjectileHit(ActorIndex index, Attack &attack, ActorIndex target) const;
	[[nodiscard]] Percent combat_projectileHitPercent(ActorIndex index, const Attack& attack, ActorIndex target) const;
	[[nodiscard]] DistanceFractional combat_getMaxMeleeRange(ActorIndex index) const { return m_maxMeleeRange[index]; }
	[[nodiscard]] DistanceFractional combat_getMaxMeleeRangeNonLethal(ActorIndex index) const { return m_maxMeleeRangeNonLethal[index]; }
	[[nodiscard]] DistanceFractional combat_getMaxRange(ActorIndex index) const { return m_maxRange[index] != DistanceFractional::create(0) ? m_maxRange[index] : m_maxMeleeRange[index]; }
	[[nodiscard]] CombatScore combat_getCombatScoreForAttack(ActorIndex index, const Attack& attack) const;
	[[nodiscard]] const Attack& combat_getAttackForCombatScoreDifference(ActorIndex index, const CombatScore scoreDifference) const;
	[[nodiscard]] const Attack& combat_getNonLethalAttackForCombatScoreDifference(ActorIndex index, CombatScore scoreDifference) const;
	[[nodiscard]] float combat_getQualityModifier(ActorIndex index, Quality quality) const;
	[[nodiscard]] bool combat_positionIsValid(ActorIndex index, Point3D point, const DistanceFractional attackRangeSquared) const;
	[[nodiscard]] AttackTypeId combat_getRangedAttackType(ActorIndex index, ItemIndex weapon) const;
	[[nodiscard]] CuboidSet combat_makeMalicePoints(ActorIndex index) const;
	[[nodiscard]] Step combat_getCoolDown(ActorIndex index, const Attack& attack) const;
	[[nodiscard]] const AttackTable& combat_getAttackTable(ActorIndex index) const { return m_meleeAttackTable[index]; }
	[[nodiscard]] const AttackTable& combat_getAttackTableNonLethal(ActorIndex index) const { return m_meleeAttackTableNonLethal[index]; }
	[[nodiscard]] float combat_getCoolDownDurationModifier(ActorIndex index) const { return m_coolDownDurationModifier[index]; }
	[[nodiscard]] CombatScore combat_getCombatScore(ActorIndex index) const { return m_combatScore[index]; }
	[[nodiscard]] CombatScore combat_getCombatScoreNonLethal(ActorIndex index) const { return m_combatScoreNonLethal[index]; }
	// -Soldier.
	// Updates MaliceMap for faction. Used by location_setDynamic and when combatScore changes.
	void soldier_removeFromMaliceMap(ActorIndex index);
	void soldier_recordInMaliceMap(ActorIndex index);
	void soldier_mobilize(ActorIndex index);
	void soldier_demobilize(ActorIndex index);
	void soldier_onSetLocation(ActorIndex index);
	void soldier_setSquad(ActorIndex index, const SquadIndex squad);
	// If an actor's SoldierData does not have a squad set then the actor is not a soldier.
	[[nodiscard]] bool soldier_is(ActorIndex index) const;
	[[nodiscard]] CuboidSet& soldier_getRecordedMalicePoints(ActorIndex index);
	// -Body.
	[[nodiscard]] Percent body_getImpairMovePercent(ActorIndex index);
	//TODO: change to getImpairDextarityPercent?
	[[nodiscard]] Percent body_getImpairManipulationPercent(ActorIndex index);
	[[nodiscard]] Step body_getStepsTillWoundsClose(ActorIndex index);
	[[nodiscard]] Step body_getStepsTillBleedToDeath(ActorIndex index);
	[[nodiscard]] bool body_hasBleedEvent(ActorIndex index) const;
	[[nodiscard]] bool body_isInjured(ActorIndex index) const;
	[[nodiscard]] bool body_isSeriouslyInjured(ActorIndex index) const;
	[[nodiscard]] BodyPart& body_pickABodyPartByVolume(ActorIndex index) const;
	[[nodiscard]] BodyPart& body_pickABodyPartByType(ActorIndex index, const BodyPartTypeId bodyPartType) const;
	[[nodiscard]] const std::vector<Wound*> body_getWounds(ActorIndex index) const;
	[[nodiscard]] const PsycologyWeight body_getPain(ActorIndex index) const;
	// -Move.
	void move_updateIndividualSpeed(ActorIndex index);
	void move_updateActualSpeed(ActorIndex index);
	void move_setPath(ActorIndex index, const SmallSet<Point3D>& path);
	void move_setType(ActorIndex index, const MoveTypeId moveType);
	void move_setMoveSpeedActual(ActorIndex index, Speed speed);
	void move_clearPath(ActorIndex index);
	void move_callback(ActorIndex index);
	void move_schedule(ActorIndex index, Point3D moveFrom);
	void move_setDestination(ActorIndex index, Point3D destination, bool detour = false, bool adjacent = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToLocation(ActorIndex index, Point3D destination, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationToAny(ActorIndex index, const CuboidSet& candidates, bool detour, bool unreserved, bool reserve, Point3D huristicDestination);
	void move_setDestinationAdjacentToActor(ActorIndex index, ActorIndex other, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToItem(ActorIndex index, ItemIndex item, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToPolymorphic(ActorIndex index, ActorOrItemIndex actorOrItemIndex, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToPlant(ActorIndex index, const PlantIndex plant, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToFluidType(ActorIndex index, FluidTypeId fluidType, bool detour = false, bool unreserved = false, bool reserve = false, Distance maxRange = Distance::max());
	void move_setDestinationAdjacentToDesignation(ActorIndex index, const SpaceDesignation& designation, bool detour = false, bool unreserved = false, bool reserve = false, Distance maxRange = Distance::max());
	void move_setDestinationToEdge(ActorIndex index, bool detour = false);
	// Used when destination changes in an absolute frame of refrence but not a local one.
	// For example when on deck of an other object which is moving.
	void move_updateDestination(ActorIndex index, Point3D point) { m_destination[index] = point; }
	void move_clearAllEventsAndTasks(ActorIndex index);
	void move_onDeath(ActorIndex index);
	void move_onLeaveArea(ActorIndex index);
	void move_pathRequestCallback(ActorIndex index, SmallSet<Point3D> path, bool useCurrentLocation, bool reserveDestination);
	void move_pathRequestMaybeCancel(ActorIndex index);
	void move_pathRequestRecord(ActorIndex index, std::unique_ptr<PathRequest> pathRequest);
	void move_pathRequestClear(ActorIndex index);
	void move_clearAllPathRequests();
	[[nodiscard]] bool move_destinationIsAdjacentToLocation(ActorIndex index, Point3D location);
	[[nodiscard]] bool move_tryToReserveProposedDestination(ActorIndex index, const SmallSet<Point3D>& path);
	[[nodiscard]] bool move_tryToReserveOccupied(ActorIndex index);
	[[nodiscard]] Speed move_getIndividualSpeedWithAddedMass(ActorIndex index, Mass  mass) const;
	[[nodiscard]] Speed move_getSpeed(ActorIndex index) const { return m_speedActual[index]; }
	[[nodiscard]] bool move_canMove(ActorIndex index) const;
	[[nodiscard]] Step move_delayToMoveInto(ActorIndex index, Point3D moveFrom, Point3D moveTo) const;
	[[nodiscard]] SmallSet<Point3D> move_makePathTo(ActorIndex index, Point3D destination) const;
	// For debugging move.
	[[nodiscard]] PathRequest& move_getPathRequest(ActorIndex index) { return *m_pathRequest[index]; }
	[[nodiscard]] auto& move_getPath(ActorIndex index) { return m_path[index]; }
	[[nodiscard]] Point3D move_getDestination(ActorIndex index) { return m_destination[index]; }
	[[nodiscard]] bool move_hasEvent(ActorIndex index) const { return m_moveEvent.exists(index); }
	[[nodiscard]] bool move_hasPathRequest(ActorIndex index) const { return m_pathRequest[index] != nullptr; }
	[[nodiscard]] bool move_hasPath(ActorIndex index) const { return !m_path[index].empty(); }
	[[nodiscard]] Step move_stepsTillNextMoveEvent(ActorIndex index) const;
	[[nodiscard]] int move_getRetries(ActorIndex index) const { return m_moveRetries[index]; }
	[[nodiscard]] bool move_canPathTo(ActorIndex index, Point3D destination);
	[[nodiscard]] bool move_canPathFromTo(ActorIndex index, Point3D start, Facing4 startFacing, Point3D destination);
	// -CanPickUp.
	void canPickUp_pickUpItem(ActorIndex index, ItemIndex item);
	void canPickUp_pickUpItemQuantity(ActorIndex index, ItemIndex item, Quantity quantity);
	void canPickUp_pickUpActor(ActorIndex index, ActorIndex actor);
	ActorOrItemIndex canPickUp_pickUpPolymorphic(ActorIndex index, ActorOrItemIndex actorOrItemIndex, Quantity quantity);
	void canPickUp_removeFluidVolume(ActorIndex index, CollisionVolume volume);
	void canPickUp_add(ActorIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void canPickUp_removeItem(ActorIndex index, ItemIndex item);
	void canPickUp_removeActor(ActorIndex index, ActorIndex actor);
	void canPickUp_remove(ActorIndex index, ActorOrItemIndex actorOrItem);
	void canPickUp_destroyItem(ActorIndex index, ItemIndex item);
	void canPickUp_updateActorIndex(ActorIndex index, ActorIndex oldIndex, ActorIndex newIndex);
	void canPickUp_updateItemIndex(ActorIndex index, ItemIndex oldIndex, ItemIndex newIndex);
	void canPickUp_updateUnencomberedCarryMass(ActorIndex index);
	void canPickUp_addFluidToContainerFromAdjacentPointsIncludingOtherContainersWithLimit(ActorIndex index, FluidTypeId fluidType, CollisionVolume limit);
	[[nodiscard]] ActorIndex canPickUp_tryToPutDownActor(ActorIndex index, Point3D location, Distance maxRange = Distance::create(1));
	[[nodiscard]] ItemIndex canPickUp_tryToPutDownItem(ActorIndex index, Point3D location, Distance maxRange = Distance::create(1));
	[[nodiscard]] ActorOrItemIndex canPickUp_tryToPutDownIfAny(ActorIndex index, Point3D location, Distance maxRange = Distance::create(1));
	[[nodiscard]] ActorOrItemIndex canPickUp_tryToPutDownPolymorphic(ActorIndex index, Point3D location, Distance maxRange = Distance::create(1));
	[[nodiscard]] ItemIndex canPickUp_getItem(ActorIndex index) const;
	[[nodiscard]] ActorIndex canPickUp_getActor(ActorIndex index) const;
	[[nodiscard]] ActorOrItemIndex canPickUp_getPolymorphic(ActorIndex index) const;
	[[nodiscard]] bool canPickUp_polymorphic(ActorIndex index, ActorOrItemIndex target) const;
	[[nodiscard]] bool canPickUp_singleItem(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool canPickUp_item(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool canPickUp_itemQuantity(ActorIndex index, ItemIndex item, Quantity quantity) const;
	[[nodiscard]] bool canPickUp_actor(ActorIndex index, ActorIndex other) const;
	[[nodiscard]] bool canPickUp_anyWithMass(ActorIndex index, Mass  mass) const;
	[[nodiscard]] bool canPickUp_polymorphicUnencombered(ActorIndex index, ActorOrItemIndex target) const;
	[[nodiscard]] bool canPickUp_itemUnencombered(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool canPickUp_actorUnencombered(ActorIndex index, ActorIndex actor) const;
	[[nodiscard]] bool canPickUp_anyWithMassUnencombered(ActorIndex index, Mass  mass) const;
	[[nodiscard]] Quantity canPickUp_quantityWhichCanBePickedUpUnencombered(ActorIndex index, ItemTypeId itemType, MaterialTypeId materialType) const;
	[[nodiscard]] bool canPickUp_exists(ActorIndex index) const { return m_carrying[index].exists(); }
	[[nodiscard]] bool canPickUp_isCarryingActor(ActorIndex index, ActorIndex actor) const { return m_carrying[index].exists() && m_carrying[index].isActor() && m_carrying[index].get().toActor() == actor; }
	[[nodiscard]] bool canPickUp_isCarryingItem(ActorIndex index, ItemIndex item) const { return m_carrying[index].exists() && m_carrying[index].isItem() && m_carrying[index].get().toItem() == item; }
	[[nodiscard]] bool canPickUp_isCarryingItemGeneric(ActorIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity) const;
	[[nodiscard]] bool canPickUp_isCarryingFluidType(ActorIndex index, FluidTypeId fluidType) const;
	[[nodiscard]] bool canPickUp_isCarryingPolymorphic(ActorIndex index, ActorOrItemIndex actorOrItemIndex) const;
	[[nodiscard]] CollisionVolume canPickUp_getFluidVolume(ActorIndex index) const;
	[[nodiscard]] FluidTypeId canPickUp_getFluidType(ActorIndex index) const;
	[[nodiscard]] bool canPickUp_isCarryingEmptyContainerWhichCanHoldFluid(ActorIndex index) const;
	[[nodiscard]] Mass canPickUp_getMass(ActorIndex index) const;
	[[nodiscard]] Speed canPickUp_speedIfCarryingQuantity(ActorIndex index, Mass  mass, Quantity quantity) const;
	[[nodiscard]] Quantity canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(ActorIndex index, Mass  unitMass, Speed minimumSpeed) const;
	[[nodiscard]] bool canPickUp_canPutDown(ActorIndex index, Point3D point);
	// Objectives.
	void objective_addTaskToStart(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_addTaskToEnd(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_addNeed(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_replaceTasks(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_canNotCompleteSubobjective(ActorIndex index);
	void objective_canNotCompleteObjective(ActorIndex index, Objective& objective);
	void objective_canNotFulfillNeed(ActorIndex index, Objective& objective);
	void objective_maybeDoNext(ActorIndex index);
	void objective_setPriority(ActorIndex index, const ObjectiveTypeId objectiveType, Priority priority);
	void objective_reset(ActorIndex index);
	void objective_projectCannotReserve(ActorIndex index);
	void objective_complete(ActorIndex index, Objective& objective);
	void objective_subobjectiveComplete(ActorIndex index);
	void objective_cancel(ActorIndex index, Objective& objective);
	void objective_execute(ActorIndex index);
	[[nodiscard]] bool objective_exists(ActorIndex index) const;
	[[nodiscard]] bool objective_hasTask(ActorIndex index, const ObjectiveTypeId objectiveTypeId) const;
	[[nodiscard]] bool objective_hasNeed(ActorIndex index, NeedType needType) const;
	[[nodiscard]] bool objective_hasSupressedNeed(ActorIndex index, NeedType needType) const;
	[[nodiscard]] Priority objective_getPriorityFor(ActorIndex index, const ObjectiveTypeId objectiveType) const;
	[[nodiscard]] std::string objective_getCurrentName(ActorIndex index) const;
	[[nodiscard]] ObjectiveTypeId objective_getCurrentTypeId(ActorIndex index) const;
	template<typename T>
	T& objective_getCurrent(ActorIndex index) { return static_cast<T&>(m_hasObjectives[index]->getCurrent()); }
	template<typename T>
	const T& objective_getCurrent(ActorIndex index) const { return static_cast<T&>(m_hasObjectives[index]->getCurrent()); }
	// For testing.
	[[nodiscard]] bool objective_queuesAreEmpty(ActorIndex index) const;
	[[nodiscard]] bool objective_isOnDelay(ActorIndex index, const ObjectiveTypeId objectiveTypeId) const;
	[[nodiscard]] Step objective_getDelayEndFor(ActorIndex index, const ObjectiveTypeId objectiveTypeId) const;
	[[nodiscard]] Step objective_getNeedDelayRemaining(ActorIndex index, NeedType objectiveTypeId) const;
	// CanReserve.
	void canReserve_clearAll(ActorIndex index);
	void canReserve_setFaction(ActorIndex index, FactionId faction);
	// Default dishonor callback is canNotCompleteCurrentObjective.
	void canReserve_reserveLocation(ActorIndex index, Point3D point, std::unique_ptr<DishonorCallback> callback = nullptr);
	void canReserve_reserveItem(ActorIndex index, ItemIndex item, Quantity quantity, std::unique_ptr<DishonorCallback> callback = nullptr);
	[[nodiscard]] bool canReserve_translateAndReservePositions(ActorIndex index, SmallMap<Point3D, std::unique_ptr<DishonorCallback>>&& pointsAndCallbacks, Point3D prevousPivot, Point3D newPivot, Facing4 previousFacing, Facing4 newFacing);
	[[nodiscard]] bool canReserve_tryToReserveLocation(ActorIndex index, Point3D point, std::unique_ptr<DishonorCallback> callback = nullptr);
	[[nodiscard]] bool canReserve_tryToReserveItem(ActorIndex index, ItemIndex item, Quantity quantity, std::unique_ptr<DishonorCallback> callback = nullptr);
	[[nodiscard]] SmallMap<Point3D, std::unique_ptr<DishonorCallback>> canReserve_unreserveAndReturnPointsAndCallbacksOnSameDeck(ActorIndex index);
	[[nodiscard]] bool canReserve_hasReservationWith(ActorIndex index, Reservable& reservable) const;
	[[nodiscard]] bool canReserve_canReserveLocation(ActorIndex index, Point3D point, Facing4 facing) const;
	[[nodiscard]] bool canReserve_locationAtEndOfPathIsUnreserved(ActorIndex index, const SmallSet<Point3D>& path) const;
private:
	[[nodiscard]] CanReserve& canReserve_get(ActorIndex index);
	// Project.
public:
	[[nodiscard]] bool project_exists(ActorIndex index) const { return m_project[index] != nullptr; }
	[[nodiscard]] Project* project_get(ActorIndex index) const { assert(m_project[index] != nullptr); return m_project[index]; }
	void project_set(ActorIndex index, Project& project) { assert(m_project[index] == nullptr); m_project[index] = &project; }
	void project_unset(ActorIndex index) { assert(m_project[index] != nullptr); m_project[index] = nullptr; }
	void project_maybeUnset(ActorIndex index) { m_project[index] = nullptr; }
	// -Equipment.
	void equipment_add(ActorIndex index, ItemIndex item);
	void equipment_addGeneric(ActorIndex index, ItemTypeId itemType, MaterialTypeId materalType, Quantity quantity);
	void equipment_remove(ActorIndex index, ItemIndex item);
	void equipment_removeGeneric(ActorIndex index, ItemTypeId itemType, MaterialTypeId materalType, Quantity quantity);
	[[nodiscard]] bool equipment_canEquipCurrently(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool equipment_containsItem(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool equipment_containsItemType(ActorIndex index, ItemTypeId type) const;
	[[nodiscard]] Mass equipment_getMass(ActorIndex index) const;
	[[nodiscard]] ItemIndex equipment_getWeaponToAttackAtRange(ActorIndex index, const DistanceFractional range) const;
	[[nodiscard]] ItemIndex equipment_getAmmoForRangedWeapon(ActorIndex index, ItemIndex weapon) const;
	[[nodiscard]] const auto& equipment_getAll(ActorIndex index) const { return m_equipmentSet[index]->getAll(); }
	[[nodiscard]] const EquipmentSet& equipment_getSet(ActorIndex index) const { return *m_equipmentSet[index]; }
	[[nodiscard]] EquipmentSet& equipment_getSet(ActorIndex index) { return *m_equipmentSet[index]; }
	[[nodiscard]] ItemIndex equipment_getFirstItemWithType(ActorIndex index, ItemTypeId itemType) { return m_equipmentSet[index]->getFirstItemWithType(m_area, itemType); }
	// -Uniform.
	void uniform_set(ActorIndex index, Uniform& uniform);
	void uniform_unset(ActorIndex index);
	[[nodiscard]] bool uniform_exists(ActorIndex index) const;
	[[nodiscard]] Uniform& uniform_get(ActorIndex index);
	[[nodiscard]] const Uniform& uniform_get(ActorIndex index) const;
	// Sleep.
	void sleep_do(ActorIndex actor);
	void sleep_wakeUp(ActorIndex actor);
	void sleep_wakeUpEarly(ActorIndex actor);
	void sleep_setSpot(ActorIndex index, Point3D location);
	void sleep_makeTired(ActorIndex index);
	void sleep_clearObjective(ActorIndex index);
	void sleep_maybeClearSpot(ActorIndex index);
	[[nodiscard]] Point3D sleep_getSpot(ActorIndex index) const;
	[[nodiscard]] bool sleep_isAwake(ActorIndex index) const;
	[[nodiscard]] bool sleep_isTired(ActorIndex index) const;
	[[nodiscard]] Percent sleep_getPercentDoneSleeping(ActorIndex index) const;
	[[nodiscard]] Percent sleep_getPercentTired(ActorIndex index) const;
	// For testing.
	[[nodiscard]] bool sleep_hasTiredEvent(ActorIndex index) const;
	// Drink.
	void drink_do(ActorIndex index, CollisionVolume volume);
	void drink_setNeedsFluid(ActorIndex index);
	void drink_setNeverThirsty(ActorIndex index);
	[[nodiscard]] CollisionVolume drink_getVolumeOfFluidRequested(ActorIndex index) const;
	[[nodiscard]] bool drink_isThirsty(ActorIndex index) const;
	[[nodiscard]] FluidTypeId drink_getFluidType(ActorIndex index) const;
	[[nodiscard]] Percent drink_getPercentDead(ActorIndex index) const;
	[[nodiscard]] Step drink_getStepsTillDead(ActorIndex index) const;
	// For Testing.
	[[nodiscard]] bool drink_hasThristEvent(ActorIndex index) const;
	// Eat.
	void eat_do(ActorIndex index, Mass  mass);
	void eat_setIsHungry(ActorIndex index);
	void eat_setNeverHungry(ActorIndex index);
	[[nodiscard]] bool eat_isHungry(ActorIndex index) const;
	[[nodiscard]] bool eat_isEating(ActorIndex index) const;
	[[nodiscard]] bool eat_canEatActor(ActorIndex index, ActorIndex other) const;
	[[nodiscard]] bool eat_canEatItem(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool eat_canEatPlant(ActorIndex index, const PlantIndex plant) const;
	[[nodiscard]] Percent eat_getPercentStarved(ActorIndex index) const;
	[[nodiscard]] Point3D eat_getOccupiedOrAdjacentPointWithTheMostDesiredFood(ActorIndex index) const;
	// Psycology.
	[[nodiscard]] PsycologyWeight psycology_actorCausesFear(ActorIndex index, ActorIndex other) const;
	[[nodiscard]] const Psycology& psycology_getConst(ActorIndex index) const;
	[[nodiscard]] Psycology& psycology_get(ActorIndex index);
	void psycology_event(ActorIndex index, const PsycologyEventType type, const PsycologyAttribute attribute, const PsycologyWeight weight, Step duration = {}, Step cooldown = {});
	void psycology_event(ActorIndex index, const PsycologyEventType type, const PsycologyData delta, Step duration = {}, Step cooldown = {});
	// For Testing.
	[[nodiscard]] Mass eat_getMassFoodRequested(ActorIndex index) const;
	[[nodiscard]] std::pair<Point3D, int> eat_getDesireToEatSomethingAt(ActorIndex index, const Cuboid cuboid) const;
	[[nodiscard]] int eat_getMinimumAcceptableDesire(ActorIndex index) const;
	[[nodiscard]] bool eat_hasObjective(ActorIndex index) const;
	[[nodiscard]] Step eat_getHungerEventStep(ActorIndex index) const;
	[[nodiscard]] bool eat_hasHungerEvent(ActorIndex index) const;
	// Temperature.
	void temperature_onChange(ActorIndex index);
	[[nodiscard]] bool temperature_isSafe(ActorIndex index, const Temperature temperature) const;
	[[nodiscard]] bool temperature_isSafeAtCurrentLocation(ActorIndex index) const;
	[[nodiscard]] Temperature temperature_getMaxSafe(ActorIndex index) const;
	[[nodiscard]] Temperature temperature_getMinSafe(ActorIndex index) const;
	// Attributes.
	void attributes_onUpdateGrowthPercent(ActorIndex index);
	void addStrengthBonusOrPenalty(ActorIndex index, const AttributeLevelBonusOrPenalty bonusOrPenalty);
	void setStrengthBonusOrPenalty(ActorIndex index, const AttributeLevelBonusOrPenalty bonusOrPenalty);
	void addStrengthModifier(ActorIndex index, float modifer);
	void setStrengthModifier(ActorIndex index, float modifer);
	void onStrengthChanged(ActorIndex index);
	void updateStrength(ActorIndex index);
	void addDextarityBonusOrPenalty(ActorIndex index, const AttributeLevelBonusOrPenalty bonusOrPenalty);
	void setDextarityBonusOrPenalty(ActorIndex index, const AttributeLevelBonusOrPenalty bonusOrPenalty);
	void addDextarityModifier(ActorIndex index, float modifer);
	void setDextarityModifier(ActorIndex index, float modifer);
	void onDextarityChanged(ActorIndex index);
	void updateDextarity(ActorIndex index);
	void addAgilityBonusOrPenalty(ActorIndex index, const AttributeLevelBonusOrPenalty bonusOrPenalty);
	void setAgilityBonusOrPenalty(ActorIndex index, const AttributeLevelBonusOrPenalty bonusOrPenalty);
	void addAgilityModifier(ActorIndex index, float modifer);
	void setAgilityModifier(ActorIndex index, float modifer);
	void onAgilityChanged(ActorIndex index);
	void updateAgility(ActorIndex index);
	void addIntrinsicMassBonusOrPenalty(ActorIndex index, int bonusOrPenalty);
	void setIntrinsicMassBonusOrPenalty(ActorIndex index, int bonusOrPenalty);
	void addIntrinsicMassModifier(ActorIndex index, float modifer);
	void setIntrinsicMassModifier(ActorIndex index, float modifer);
	void onIntrinsicMassChanged(ActorIndex index);
	void updateIntrinsicMass(ActorIndex index);
	[[nodiscard]] AttributeLevel getStrength(ActorIndex index) const;
	[[nodiscard]] AttributeLevelBonusOrPenalty getStrengthBonusOrPenalty(ActorIndex index) const;
	[[nodiscard]] float getStrengthModifier(ActorIndex index) const;
	[[nodiscard]] AttributeLevel getDextarity(ActorIndex index) const;
	[[nodiscard]] AttributeLevelBonusOrPenalty getDextarityBonusOrPenalty(ActorIndex index) const;
	[[nodiscard]] float getDextarityModifier(ActorIndex index) const;
	[[nodiscard]] AttributeLevel getAgility(ActorIndex index) const;
	[[nodiscard]] AttributeLevelBonusOrPenalty getAgilityBonusOrPenalty(ActorIndex index) const;
	[[nodiscard]] float getAgilityModifier(ActorIndex index) const;
	[[nodiscard]] Mass getIntrinsicMass(ActorIndex index) const;
	[[nodiscard]] int getIntrinsicMassBonusOrPenalty(ActorIndex index) const;
	[[nodiscard]] float getIntrinsicMassModifier(ActorIndex index) const;
	[[nodiscard]] CombatScore attributes_getCombatScore(ActorIndex index) const;
	[[nodiscard]] Speed attributes_getMoveSpeed(ActorIndex index) const;
	// Skills.
	[[nodiscard]] SkillLevel skill_getLevel(ActorIndex index, const SkillTypeId skillType) const;
	[[nodiscard]] const SkillSet& skill_getSet(ActorIndex index) const { return m_skillSet[index]; }
	[[nodiscard]] SkillSet& skill_getSet(ActorIndex index) { return m_skillSet[index]; }
	void skill_addXp(ActorIndex index, const SkillTypeId skillType, const SkillExperiencePoints xp);
	// Growth.
	void grow_maybeStart(ActorIndex index);
	void grow_stop(ActorIndex index);
	void grow_updateGrowingStatus(ActorIndex index);
	void grow_setPercent(ActorIndex index, Percent percentGrown);
	[[nodiscard]] bool grow_isGrowing(ActorIndex index) const;
	[[nodiscard]] Percent grow_getPercent(ActorIndex index) const;
	// For Line leader.
	[[nodiscard]] const SmallSet<Point3D>& lineLead_getPath(ActorIndex index) const;
	[[nodiscard]] CuboidSet lineLead_getOccupiedCuboids(ActorIndex index) const;
	[[nodiscard]] bool lineLead_pathEmpty(ActorIndex index) const;
	[[nodiscard]] ShapeId lineLead_getLargestShape(ActorIndex index) const;
	[[nodiscard]] MoveTypeId lineLead_getMoveType(ActorIndex index) const;
	[[nodiscard]] Speed lineLead_getSpeedWithAddedMass(ActorIndex index, Mass  mass) const;
	[[nodiscard]] Speed lineLead_getSpeedWithAddedMass(const SmallSet<ActorIndex>& indices, Mass  mass) const;
	[[nodiscard]] std::vector<ActorOrItemIndex> lineLead_getAll(ActorIndex index) const;
	[[nodiscard]] bool lineLead_followersCanMoveEver(ActorIndex index) const;
	[[nodiscard]] bool lineLead_followersCanMoveCurrently(ActorIndex index) const;
	[[nodiscard]] std::pair<Point3D, Facing4> lineLead_followerGetNextStep(ActorOrItemIndex follower, const SmallSet<Point3D>& path, const CuboidSet& occupiedByCurrentLeader) const;
	[[nodiscard]] SetLocationAndFacingResult lineLead_tryToMove(ActorIndex index);
	[[nodiscard]] OffsetCuboid lineLead_getHypotheticalStraightLineBoundry(ActorIndex index, Facing4 facing) const;
	void lineLead_clearPath(ActorIndex index);
	void lineLead_appendToPath(ActorIndex index, Point3D point, Facing4 facing);
	void lineLead_pushFront(ActorIndex index, Point3D point);
	void lineLead_popBackUnlessOccupiedByFollower(ActorIndex index);
	void lineLead_moveFollowers(ActorIndex index);
	// Mount.
	[[nodiscard]] Point3D mount_findLocationToMountOn(ActorIndex index, ActorIndex toMount) const;
	// This method is used for both piloting animals and vehicles, name should be changed.
	[[nodiscard]] bool mount_isPilot(ActorIndex index) const { return m_isPilot[index]; }
	[[nodiscard]] bool mount_hasPilot(ActorIndex index) const;
	[[nodiscard]] bool mount_exists(ActorIndex index) const { return m_isOnDeckOf[index].exists(); }
	[[nodiscard]] ActorIndex mount_getPilot(ActorIndex index) const;
	[[nodiscard]] OffsetCuboidSet getDeckOffsets(ActorIndex) { return {}; }
	void mount_do(ActorIndex index, ActorIndex toMount, Point3D location, bool pilot);
	void mount_undo(ActorIndex index, Point3D location, Facing4 facing);
	void mount_set(ActorIndex index, ActorIndex toMount);
	void mount_unset(ActorIndex index, ActorIndex mount);
	// Pilot Item.
	void pilotItem_set(ActorIndex index, ItemIndex item);
	void pilotItem_unset(ActorIndex index);
	[[nodiscard]] bool pilotItem_isPilotingConstructedItem(ActorIndex index);
	// Drama.
	// Duration paramater is how long to show the line for.
	// Default value null indicates to keep shoiwng it untill clearDialog is called.
	void drama_setDialog(ActorIndex index, const std::string& line, Step duration = Step::null());
	void drama_clearDialog(ActorIndex index);
	// Set actor to be puppeted by another actor's objective.
	// Undone by calling objective_complete.
	void drama_castForRole(ActorIndex index, Objective& objective, ActorIndex objectiveOwner);
	//Expedition.
	void expedition_set(ActorIndex index, ExpeditionId expedition) { m_expedition[index] = expedition; }
	void expedition_unset(ActorIndex index) { m_expedition[index].clear(); }
	[[nodiscard]] ExpeditionId expedition_get(ActorIndex index) const { return m_expedition[index]; }
	// For testing.
	[[nodiscard]] bool grow_getEventExists(ActorIndex index) const;
	[[nodiscard]] Percent grow_getEventPercent(ActorIndex index) const;
	[[nodiscard]] Step grow_getEventStep(ActorIndex index) const;
	[[nodiscard]] bool grow_eventIsPaused(ActorIndex index) const;
	// For UI.
	[[nodiscard]] ActorOrItemIndex canPickUp_getCarrying(ActorIndex index) const;

	// For debugging.
	void log(ActorIndex index) const;
	void satisfyNeeds(ActorIndex index);
	friend class MoveEvent;
	friend class AttackCoolDownEvent;
	friend class SupressedNeed;
	friend class ThirstEvent;
	friend class DrinkObjective;
	friend class EatEvent;
	friend class HungerEvent;
	friend class EatPathRequest;
	friend class EatObjective;
	friend class UnsafeTemperatureEvent;
	friend class UniformObjective;
	Actors(Actors&) = delete;
	Actors(Actors&&) = delete;
};
class MoveEvent final : public ScheduledEvent
{
	ActorIndex m_actor;
public:
	MoveEvent(Step delay, Area& area, ActorIndex actor, Step start = Step::null());
	MoveEvent(Simulation& simulation, const Json& data);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	void onMoveIndex([[maybe_unused]] HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_actor == oldIndex.toActor()); m_actor = ActorIndex::create(newIndex.get()); }
	void updateIndex([[maybe_unused]] ActorIndex oldIndex, ActorIndex newIndex)
	{
		assert(oldIndex == m_actor);
		m_actor = newIndex;
	}
	[[nodiscard]] Json toJson() const;
};
class AttackCoolDownEvent final : public ScheduledEvent
{
	ActorIndex m_actor;
public:
	AttackCoolDownEvent(Area& area, ActorIndex index, Step duration, Step start = Step::null());
	AttackCoolDownEvent(Simulation& simulation, const Json& data);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	void onMoveIndex([[maybe_unused]] HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_actor == oldIndex.toActor()); m_actor = ActorIndex::create(newIndex.get()); }
	[[nodiscard]] Json toJson() const;
};
class GetIntoAttackPositionPathRequest final : public PathRequest
{
	ActorReference target;
	DistanceFractional attackRangeSquared;
	Distance attackRangeInteger;
public:
	GetIntoAttackPositionPathRequest(Area& area, ActorIndex attacker, ActorIndex target, DistanceFractional attackRangeFractional);
	GetIntoAttackPositionPathRequest(const Json& data, Area& area);
	PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() { return "attack"; }
};
inline void to_json(Json& data, const std::unique_ptr<GetIntoAttackPositionPathRequest>& pathRequest) { data = pathRequest->toJson(); }
