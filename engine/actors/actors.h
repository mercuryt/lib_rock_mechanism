#pragma once

#include "../datetime.h"
#include "../portables.h"
#include "../types.h"
#include "../visionFacade.h"
#include "../body.h"
#include "../objective.h"
#include "../equipment.h"
#include "../actorOrItemIndex.h"
#include "../skill.h"
#include "../terrainFacade.h"
#include "../pathRequest.h"
#include "../reference.h"
#include "../index.h"
#include "../attackType.h"
#include "../uniform.h"
#include "actors/uniform.h"
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
	std::wstring name = L"";
	DateTime birthDate = {0,0,0};
	Step birthStep = Step::null();
	Percent percentGrown = Percent::null();
	BlockIndex location;
	Facing facing = Facing::null();
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

	Percent getPercentGrown(Simulation& simulation);
	std::wstring getName(Simulation& simulation);
	Step getBirthStep(Simulation& simulation);
	ActorId getId(Simulation& simulation);
	Percent getPercentThirst(Simulation& simulation);
	Percent getPercentHunger(Simulation& simulation);
	Percent getPercentTired(Simulation& simulation);
	void generateEquipment(Area& area, ActorIndex actor);
};
class Actors final : public Portables
{
	ActorIndexSet m_onSurface;
	DataVector<std::unique_ptr<ActorReferenceTarget>, ActorIndex> m_referenceTarget;
	DataVector<ActorId, ActorIndex> m_id;
	DataVector<std::wstring, ActorIndex> m_name;
	DataVector<AnimalSpeciesId, ActorIndex> m_species;
	DataVector<Project*, ActorIndex> m_project;
	DataVector<Step, ActorIndex> m_birthStep;
	DataVector<CauseOfDeath, ActorIndex> m_causeOfDeath;
	DataVector<AttributeLevel, ActorIndex> m_strength;
	DataVector<AttributeLevelBonusOrPenalty, ActorIndex> m_strengthBonusOrPenalty;
	DataVector<float, ActorIndex> m_strengthModifier;
	DataVector<AttributeLevel, ActorIndex> m_agility;
	DataVector<AttributeLevelBonusOrPenalty, ActorIndex> m_agilityBonusOrPenalty;
	DataVector<float, ActorIndex> m_agilityModifier;
	DataVector<AttributeLevel, ActorIndex> m_dextarity;
	DataVector<AttributeLevelBonusOrPenalty, ActorIndex> m_dextarityBonusOrPenalty;
	DataVector<float, ActorIndex> m_dextarityModifier;
	DataVector<Mass, ActorIndex> m_mass;
	DataVector<int32_t, ActorIndex> m_massBonusOrPenalty;
	DataVector<float, ActorIndex> m_massModifier;
	DataVector<Mass, ActorIndex> m_unencomberedCarryMass;
	DataVector<BlockIndices, HasShapeIndex> m_leadFollowPath;
	DataVector<std::unique_ptr<HasObjectives>, ActorIndex> m_hasObjectives;
	DataVector<std::unique_ptr<Body>, ActorIndex> m_body;
	DataVector<std::unique_ptr<MustSleep>, ActorIndex> m_mustSleep;
	DataVector<std::unique_ptr<MustDrink>, ActorIndex> m_mustDrink;
	DataVector<std::unique_ptr<MustEat>, ActorIndex> m_mustEat; 
	DataVector<std::unique_ptr<ActorNeedsSafeTemperature>, ActorIndex> m_needsSafeTemperature;
	DataVector<std::unique_ptr<CanGrow>, ActorIndex> m_canGrow;
	DataVector<std::unique_ptr<SkillSet>, ActorIndex> m_skillSet;
	DataVector<std::unique_ptr<CanReserve>, ActorIndex> m_canReserve;
	DataVector<std::unique_ptr<ActorHasUniform>, ActorIndex> m_hasUniform;
	DataVector<std::unique_ptr<EquipmentSet>, ActorIndex> m_equipmentSet;
	// CanPickUp.
	// TODO: Should be a reference?
	DataVector<ActorOrItemIndex, ActorIndex> m_carrying;
	// Stamina.
	DataVector<Stamina, ActorIndex> m_stamina;
	// Vision.
	DataVector<ActorIndices, ActorIndex> m_canSee;
	DataVector<DistanceInBlocks, ActorIndex> m_visionRange;
	DataVector<HasVisionFacade, ActorIndex> m_hasVisionFacade;
	// Combat.
	HasScheduledEvents<AttackCoolDownEvent> m_coolDownEvent;
	DataVector<std::vector<std::pair<CombatScore, Attack>>, ActorIndex> m_meleeAttackTable;
	DataVector<ActorIndices, ActorIndex> m_targetedBy;
	DataVector<ActorIndex, ActorIndex> m_target;
	DataVector<Step, ActorIndex> m_onMissCoolDownMelee;
	DataVector<DistanceInBlocksFractional, ActorIndex> m_maxMeleeRange;
	DataVector<DistanceInBlocksFractional, ActorIndex> m_maxRange;
	DataVector<float, ActorIndex> m_coolDownDurationModifier;
	DataVector<CombatScore, ActorIndex> m_combatScore;
	// Move.
	HasScheduledEvents<MoveEvent> m_moveEvent;
	DataVector<std::unique_ptr<PathRequest>, ActorIndex> m_pathRequest;
	DataVector<BlockIndices, ActorIndex> m_path;
	DataVector<BlockIndices::iterator, ActorIndex> m_pathIter;
	DataVector<BlockIndex, ActorIndex> m_destination;
	DataVector<Speed, ActorIndex> m_speedIndividual;
	DataVector<Speed, ActorIndex> m_speedActual;
	DataVector<uint8_t, ActorIndex> m_moveRetries;
	void resize(ActorIndex newSize);
	void moveIndex(ActorIndex oldIndex, ActorIndex newIndex);
	void destroy(ActorIndex index);
	[[nodiscard]] bool indexCanBeMoved(HasShapeIndex index) const;
public:
	Actors(Area& area);
	~Actors();
	void load(const Json& data);
	void loadObjectivesAndReservations(const Json& data);
	void onChangeAmbiantSurfaceTemperature();
	ActorIndex create(ActorParamaters params);
	void sharedConstructor(ActorIndex index);
	void scheduleNeeds(ActorIndex index);
	void resetNeeds(ActorIndex index);
	void setShape(ActorIndex index, ShapeId shape);
	void setLocation(ActorIndex index, BlockIndex block);
	void setLocationAndFacing(ActorIndex index, BlockIndex block, Facing facing);
	void exit(ActorIndex index);
	void removeMassFromCorpse(ActorIndex index, Mass mass);
	void die(ActorIndex index, CauseOfDeath causeOfDeath);
	void passout(ActorIndex index, Step duration);
	void leaveArea(ActorIndex index);
	void wait(ActorIndex index, Step duration);
	void takeHit(ActorIndex index, Hit& hit, BodyPart& bodyPart);
	// May be null.
	void setFaction(ActorIndex index, FactionId faction);
	void reserveAllBlocksAtLocationAndFacing(ActorIndex index, BlockIndex location, Facing facing);
	void unreserveAllBlocksAtLocationAndFacing(ActorIndex index, BlockIndex location, Facing facing);
	void setBirthStep(ActorIndex index, Step step);
	[[nodiscard]] ActorIndices getAll() const;
	[[nodiscard]] ActorIndexSet getOnSurface() const { return m_onSurface; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ActorId getId(ActorIndex index) const { return m_id[index]; }
	[[nodiscard]] ActorReference getReference(ActorIndex index) const { return *m_referenceTarget[index].get();}
	[[nodiscard]] const ActorReference getReferenceConst(ActorIndex index) const { return *m_referenceTarget[index].get();}
	[[nodiscard]] ActorReferenceTarget& getReferenceTarget(ActorIndex index) const { return *m_referenceTarget[index].get();}
	[[nodiscard]] std::wstring getName(ActorIndex index) const { return m_name[index]; }
	[[nodiscard]] bool isAlive(ActorIndex index) const { return m_causeOfDeath[index] == CauseOfDeath::none; }
	[[nodiscard]] Percent getPercentGrown(ActorIndex index) const;
	[[nodiscard]] CauseOfDeath getCauseOfDeath(ActorIndex index) const { assert(!isAlive(index)); return m_causeOfDeath[index]; }
	[[nodiscard]] bool isEnemy(ActorIndex actor, ActorIndex other) const;
	[[nodiscard]] bool isAlly(ActorIndex actor, ActorIndex other) const;
	[[nodiscard]] bool isSentient(ActorIndex index) const;
	[[nodiscard]] bool isInjured(ActorIndex index) const;
	[[nodiscard]] bool canMove(ActorIndex index) const;
	[[nodiscard]] Volume getVolume(ActorIndex index) const;
	[[nodiscard]] Mass getMass(ActorIndex index) const;
	[[nodiscard]] Quantity getAgeInYears(ActorIndex index) const;
	[[nodiscard]] Step getAge(ActorIndex index) const;
	[[nodiscard]] Step getBirthStep(ActorIndex index) const { return m_birthStep[index]; }
	[[nodiscard]] std::wstring getActionDescription(ActorIndex index) const;
	[[nodiscard]] AnimalSpeciesId getSpecies(ActorIndex index) const { return m_species[index]; }
	[[nodiscard]] Mass getUnencomberedCarryMass(ActorIndex index) const { return m_unencomberedCarryMass[index]; }
	// -Stamina.
	void stamina_recover(ActorIndex index);
	void stamina_spend(ActorIndex index, Stamina stamina);
	void stamina_setFull(ActorIndex index);
	bool stamina_hasAtLeast(ActorIndex index, Stamina stamina) const;
	bool stamina_isFull(ActorIndex index) const;
	[[nodiscard]] Stamina stamina_getMax(ActorIndex index) const;
	[[nodiscard]] Stamina stamina_get(ActorIndex index) const { return m_stamina[index]; }
	// -Vision.
	void vision_do(ActorIndex index, ActorIndices& actors);
	void vision_setRange(ActorIndex index, DistanceInBlocks range);
	void vision_createFacadeIfCanSee(ActorIndex index);
	void vision_clearFacade(ActorIndex index);
	void vision_swap(ActorIndex index, ActorIndices& toSwap);
	[[nodiscard]] ActorIndices& vision_getCanSee(ActorIndex index) { return m_canSee[index]; }
	[[nodiscard]] DistanceInBlocks vision_getRange(ActorIndex index) const { return m_visionRange[index]; }
	[[nodiscard]] bool vision_canSeeActor(ActorIndex index, ActorIndex other) const;
	[[nodiscard]] bool vision_canSeeAnything(ActorIndex index) const;
	[[nodiscard]] bool vision_hasFacade(ActorIndex actor) const;
	[[nodiscard]] std::pair<VisionFacade*, VisionFacadeIndex> vision_getFacadeWithIndex(ActorIndex actor) const;
	// To be used by VisionFacade only.
	[[nodiscard]] HasVisionFacade& vision_getHasVisionFacade(ActorIndex index) { return m_hasVisionFacade[index]; }
	// For testing.
	[[nodiscard]] VisionFacade& vision_getFacadeBucket(ActorIndex index);
	// -Combat.
	void combat_attackMeleeRange(ActorIndex index, ActorIndex target);
	void combat_attackLongRange(ActorIndex index, ActorIndex target, ItemIndex weapon, ItemIndex ammo);
	void combat_coolDownCompleted(ActorIndex index);
	void combat_update(ActorIndex index);
	void combat_setTarget(ActorIndex index, ActorIndex actor);
	void combat_recordTargetedBy(ActorIndex index, ActorIndex actor);
	void combat_removeTargetedBy(ActorIndex index, ActorIndex actor);
	void combat_onMoveFrom(ActorIndex index, BlockIndex previous);
	void combat_onDeath(ActorIndex index);
	void combat_onLeaveArea(ActorIndex index);
	void combat_noLongerTargetable(ActorIndex index);
	void combat_targetNoLongerTargetable(ActorIndex index);
	void combat_onTargetMoved(ActorIndex index);
	void combat_freeHit(ActorIndex index, ActorIndex actor);
	//TODO: Combat vs. items?
	void combat_getIntoRangeAndLineOfSightOfActor(ActorIndex index, ActorIndex target, DistanceInBlocksFractional range);
	[[nodiscard]] CombatScore combat_getCurrentMeleeCombatScore(ActorIndex index);
	[[nodiscard]] bool combat_isOnCoolDown(ActorIndex index) const;
	[[nodiscard]] bool combat_inRange(ActorIndex index, const ActorIndex target) const;
	[[nodiscard]] bool combat_doesProjectileHit(ActorIndex index, Attack& attack, const ActorIndex target) const;
	[[nodiscard]] Percent combat_projectileHitPercent(ActorIndex index, const Attack& attack, const ActorIndex target) const;
	[[nodiscard]] DistanceInBlocksFractional combat_getMaxMeleeRange(ActorIndex index) const { return m_maxMeleeRange[index]; }
	[[nodiscard]] DistanceInBlocksFractional combat_getMaxRange(ActorIndex index) const { return m_maxRange[index] != DistanceInBlocksFractional::create(0) ? m_maxRange[index] : m_maxMeleeRange[index]; }
	[[nodiscard]] CombatScore combat_getCombatScoreForAttack(ActorIndex index, const Attack& attack) const;
	[[nodiscard]] const Attack& combat_getAttackForCombatScoreDifference(ActorIndex index, CombatScore scoreDifference) const;
	[[nodiscard]] float combat_getQualityModifier(ActorIndex index, Quality quality) const;
	[[nodiscard]] bool combat_blockIsValidPosition(ActorIndex index, BlockIndex block, DistanceInBlocksFractional attackRangeSquared) const;
	AttackTypeId combat_getRangedAttackType(ActorIndex index, ItemIndex weapon);
	//for degbugging combat
	[[nodiscard]] std::vector<std::pair<CombatScore, Attack>>& combat_getAttackTable(ActorIndex index) { return m_meleeAttackTable[index]; }
	[[nodiscard]] float combat_getCoolDownDurationModifier(ActorIndex index) { return m_coolDownDurationModifier[index]; }
	[[nodiscard, maybe_unused]] CombatScore combat_getCombatScore(ActorIndex index) const { return m_combatScore[index]; }
	[[nodiscard]] std::vector<std::pair<CombatScore, Attack>>& combat_getMeleeAttacks(ActorIndex index);
	// -Body.
	[[nodiscard]] Percent body_getImpairMovePercent(ActorIndex index);
	//TODO: change to getImpairDextarityPercent?
	[[nodiscard]] Percent body_getImpairManipulationPercent(ActorIndex index);
	[[nodiscard]] Step body_getStepsTillWoundsClose(ActorIndex index);
	[[nodiscard]] Step body_getStepsTillBleedToDeath(ActorIndex index);
	[[nodiscard]] bool body_hasBleedEvent(ActorIndex index) const;
	[[nodiscard]] bool body_isInjured(ActorIndex index) const;
	[[nodiscard]] BodyPart& body_pickABodyPartByVolume(ActorIndex index) const;
	[[nodiscard]] BodyPart& body_pickABodyPartByType(ActorIndex index, BodyPartTypeId bodyPartType) const;
	// -Move.
	void move_updateIndividualSpeed(ActorIndex index);
	void move_updateActualSpeed(ActorIndex index);
	void move_setType(ActorIndex index, MoveTypeId moveType);
	void move_setPath(ActorIndex index, BlockIndices& path);
	void move_setMoveSpeedActualForLeading(ActorIndex index, Speed speed);
	void move_clearPath(ActorIndex index);
	void move_callback(ActorIndex index);
	void move_schedule(ActorIndex index);
	void move_setDestination(ActorIndex index, BlockIndex destination, bool detour = false, bool adjacent = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToLocation(ActorIndex index, BlockIndex destination, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToActor(ActorIndex index, ActorIndex other, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToItem(ActorIndex index, ItemIndex item, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToPolymorphic(ActorIndex index, ActorOrItemIndex actorOrItemIndex, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToPlant(ActorIndex index, PlantIndex plant, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToFluidType(ActorIndex index, FluidTypeId fluidType, bool detour = false, bool unreserved = false, bool reserve = false, DistanceInBlocks maxRange = DistanceInBlocks::null());
	void move_setDestinationAdjacentToDesignation(ActorIndex index, BlockDesignation designation, bool detour = false, bool unreserved = false, bool reserve = false, DistanceInBlocks maxRange = DistanceInBlocks::null());
	void move_setDestinationToEdge(ActorIndex index, bool detour = false);
	void move_clearAllEventsAndTasks(ActorIndex index);
	void move_onDeath(ActorIndex index);
	void move_onLeaveArea(ActorIndex index);
	void move_pathRequestCallback(ActorIndex index, BlockIndices path, bool useCurrentLocation, bool reserveDestination);
	void move_pathRequestMaybeCancel(ActorIndex index);
	void move_pathRequestRecord(ActorIndex index, std::unique_ptr<PathRequest> pathRequest);
	void move_updatePathRequestTerrainFacadeIndex(const ActorIndex& index, const PathRequestIndex& newPathRequestIndex);
	[[nodiscard]] bool move_tryToReserveProposedDestination(ActorIndex index, BlockIndices& path);
	[[nodiscard]] bool move_tryToReserveOccupied(ActorIndex index);
	[[nodiscard]] Speed move_getIndividualSpeedWithAddedMass(ActorIndex index, Mass mass) const;
	[[nodiscard]] Speed move_getSpeed(ActorIndex index) const { return m_speedActual[index]; }
	[[nodiscard]] bool move_canMove(ActorIndex index) const;
	[[nodiscard]] Step move_delayToMoveInto(ActorIndex index, const BlockIndex block) const;
	[[nodiscard]] std::unique_ptr<PathRequest> move_movePathRequestData(ActorIndex index) { auto output = std::move(m_pathRequest[index]); m_pathRequest[index] = nullptr; return output; }
	// For debugging move.
	[[nodiscard]] PathRequest& move_getPathRequest(ActorIndex index) { return *m_pathRequest[index].get(); }
	[[nodiscard]] BlockIndices& move_getPath(ActorIndex index) { return m_path[index]; }
	[[nodiscard]] BlockIndex move_getDestination(ActorIndex index) { return m_destination[index]; }
	[[nodiscard]] bool move_hasEvent(ActorIndex index) const { return m_moveEvent.exists(index); }
	[[nodiscard]] bool move_hasPathRequest(ActorIndex index) const { return m_pathRequest[index].get() != nullptr; }
	[[nodiscard]] Step move_stepsTillNextMoveEvent(ActorIndex index) const;
	[[nodiscard]] uint8_t move_getRetries(ActorIndex index) const { return m_moveRetries[index]; }
	// -CanPickUp.
	void canPickUp_pickUpItem(ActorIndex index, ItemIndex item);
	void canPickUp_pickUpItemQuantity(ActorIndex index, ItemIndex item, Quantity quantity);
	void canPickUp_pickUpActor(ActorIndex index, ActorIndex actor);
	void canPickUp_pickUpPolymorphic(ActorIndex index, ActorOrItemIndex actorOrItemIndex, Quantity quantity);
	void canPickUp_removeFluidVolume(ActorIndex index, CollisionVolume volume);
	void canPickUp_add(ActorIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void canPickUp_removeItem(ActorIndex index, ItemIndex item);
	void canPickUp_updateActorIndex(ActorIndex index, ActorIndex oldIndex, ActorIndex newIndex);
	void canPickUp_updateItemIndex(ActorIndex index, ItemIndex oldIndex, ItemIndex newIndex);
	void canPickUp_updateUnencomberedCarryMass(ActorIndex index);
	[[nodiscard]] ActorIndex canPickUp_tryToPutDownActor(ActorIndex index, BlockIndex location, DistanceInBlocks maxRange = DistanceInBlocks::create(1));
	[[nodiscard]] ItemIndex canPickUp_tryToPutDownItem(ActorIndex index, BlockIndex location, DistanceInBlocks maxRange = DistanceInBlocks::create(1));
	[[nodiscard]] ActorOrItemIndex canPickUp_tryToPutDownIfAny(ActorIndex index, BlockIndex location, DistanceInBlocks maxRange = DistanceInBlocks::create(1));
	[[nodiscard]] ActorOrItemIndex canPickUp_tryToPutDownPolymorphic(ActorIndex index, BlockIndex location, DistanceInBlocks maxRange = DistanceInBlocks::create(1));
	[[nodiscard]] ItemIndex canPickUp_getItem(ActorIndex index) const;
	[[nodiscard]] ActorIndex canPickUp_getActor(ActorIndex index) const;
	[[nodiscard]] ActorOrItemIndex canPickUp_getPolymorphic(ActorIndex index) const;
	[[nodiscard]] bool canPickUp_polymorphic(ActorIndex index, ActorOrItemIndex target) const;
	[[nodiscard]] bool canPickUp_singleItem(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool canPickUp_item(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool canPickUp_itemQuantity(ActorIndex index, ItemIndex item, Quantity quantity) const;
	[[nodiscard]] bool canPickUp_actor(ActorIndex index, ActorIndex other) const;
	[[nodiscard]] bool canPickUp_anyWithMass(ActorIndex index, Mass mass) const;
	[[nodiscard]] bool canPickUp_polymorphicUnencombered(ActorIndex index, ActorOrItemIndex target) const;
	[[nodiscard]] bool canPickUp_itemUnencombered(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool canPickUp_actorUnencombered(ActorIndex index, ActorIndex actor) const;
	[[nodiscard]] bool canPickUp_anyWithMassUnencombered(ActorIndex index, Mass mass) const;
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
	[[nodiscard]] Speed canPickUp_speedIfCarryingQuantity(ActorIndex index, Mass mass, Quantity quantity) const;
	[[nodiscard]] Quantity canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(ActorIndex index, Mass mass, Speed minimumSpeed) const;
	[[nodiscard]] bool canPickUp_canPutDown(ActorIndex index, BlockIndex block);
	// Objectives.
	void objective_addTaskToStart(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_addTaskToEnd(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_addNeed(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_replaceTasks(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_canNotCompleteSubobjective(ActorIndex index);
	void objective_canNotCompleteObjective(ActorIndex index, Objective& objective);
	void objective_canNotFulfillNeed(ActorIndex index, Objective& objective);
	void objective_maybeDoNext(ActorIndex index);
	void objective_setPriority(ActorIndex index, ObjectiveTypeId objectiveType, Priority priority);
	void objective_reset(ActorIndex index);
	void objective_projectCannotReserve(ActorIndex index);
	void objective_complete(ActorIndex index, Objective& objective);
	void objective_subobjectiveComplete(ActorIndex index);
	void objective_cancel(ActorIndex, Objective& objective);
	void objective_execute(ActorIndex);
	[[nodiscard]] bool objective_exists(ActorIndex index) const;
	[[nodiscard]] bool objective_hasTask(ActorIndex index, ObjectiveTypeId objectiveTypeId) const;
	[[nodiscard]] bool objective_hasNeed(ActorIndex index, NeedType needType) const;
	[[nodiscard]] Priority objective_getPriorityFor(ActorIndex index, ObjectiveTypeId objectiveType) const;
	[[nodiscard]] std::string objective_getCurrentName(ActorIndex index) const;
	template<typename T>
	T& objective_getCurrent(ActorIndex index) { return static_cast<T&>(m_hasObjectives[index]->getCurrent()); }
	// For testing.
	[[nodiscard]] bool objective_queuesAreEmpty(ActorIndex index) const;
	[[nodiscard]] bool objective_isOnDelay(ActorIndex index, ObjectiveTypeId objectiveTypeId) const;
	[[nodiscard]] Step objective_getDelayEndFor(ActorIndex index, ObjectiveTypeId objectiveTypeId) const;
	// CanReserve.
	void canReserve_clearAll(ActorIndex index);
	void canReserve_setFaction(ActorIndex index, FactionId faction);
	// Default dishonor callback is canNotCompleteCurrentObjective.
	void canReserve_reserveLocation(ActorIndex index, BlockIndex block, std::unique_ptr<DishonorCallback> callback = nullptr);
	void canReserve_reserveItem(ActorIndex index, ItemIndex item, Quantity quantity, std::unique_ptr<DishonorCallback> callback = nullptr);
	[[nodiscard]] bool canReserve_tryToReserveLocation(ActorIndex index, BlockIndex block, std::unique_ptr<DishonorCallback> callback = nullptr);
	[[nodiscard]] bool canReserve_tryToReserveItem(ActorIndex index, ItemIndex item, Quantity quantity, std::unique_ptr<DishonorCallback> callback = nullptr);
	[[nodiscard]] bool canReserve_hasReservationWith(ActorIndex index, Reservable& reservable) const;
	[[nodiscard]] bool canReserve_canReserveLocation(const ActorIndex& index, const BlockIndex& block, const Facing& facing) const;
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
	[[nodiscard]] ItemIndex equipment_getWeaponToAttackAtRange(ActorIndex index, DistanceInBlocksFractional range) const;
	[[nodiscard]] ItemIndex equipment_getAmmoForRangedWeapon(ActorIndex index, ItemIndex weapon) const;
	[[nodiscard]] const auto& equipment_getAll(ActorIndex index) const { return m_equipmentSet[index]->getAll(); }
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
	void sleep_setSpot(ActorIndex index, BlockIndex location);
	void sleep_makeTired(ActorIndex index);
	void sleep_clearObjective(ActorIndex index);
	[[nodiscard]] BlockIndex sleep_getSpot(ActorIndex index) const;
	[[nodiscard]] bool sleep_isAwake(ActorIndex index) const;
	[[nodiscard]] Percent sleep_getPercentDoneSleeping(ActorIndex index) const;
	[[nodiscard]] Percent sleep_getPercentTired(ActorIndex index) const;
	// For testing.
	[[nodiscard]] bool sleep_hasTiredEvent(ActorIndex index) const;
	// Drink.
	void drink_do(ActorIndex index, CollisionVolume volume);
	void drink_setNeedsFluid(ActorIndex index);
	[[nodiscard]] CollisionVolume drink_getVolumeOfFluidRequested(ActorIndex index) const;
	[[nodiscard]] bool drink_isThirsty(ActorIndex index) const;
	[[nodiscard]] FluidTypeId drink_getFluidType(ActorIndex index) const;
	// For Testing.
	[[nodiscard]] bool drink_hasThristEvent(ActorIndex index) const;
	// Eat.
	void eat_do(ActorIndex index, Mass mass);
	[[nodiscard]] bool eat_isHungry(ActorIndex index) const;
	[[nodiscard]] bool eat_canEatActor(ActorIndex index, ActorIndex other) const;
	[[nodiscard]] bool eat_canEatItem(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool eat_canEatPlant(ActorIndex index, PlantIndex plant) const;
	[[nodiscard]] Percent eat_getPercentStarved(ActorIndex index) const;
	[[nodiscard]] BlockIndex eat_getAdjacentBlockWithTheMostDesiredFood(ActorIndex index) const;
	// For Testing.
	[[nodiscard]] Mass eat_getMassFoodRequested(ActorIndex index) const;
	[[nodiscard]] uint32_t eat_getDesireToEatSomethingAt(ActorIndex index, BlockIndex block) const;
	[[nodiscard]] uint32_t eat_getMinimumAcceptableDesire(ActorIndex index) const;
	[[nodiscard]] bool eat_hasObjective(ActorIndex index) const;
	[[nodiscard]] Step eat_getHungerEventStep(ActorIndex index) const;
	[[nodiscard]] bool eat_hasHungerEvent(ActorIndex index) const;
	// Temperature.
	void temperature_onChange(ActorIndex index);
	[[nodiscard]] bool temperature_isSafe(ActorIndex index, Temperature temperature) const;
	[[nodiscard]] bool temperature_isSafeAtCurrentLocation(ActorIndex index) const;
	// Attributes.
	void attributes_onUpdateGrowthPercent(ActorIndex index);
	void addStrengthBonusOrPenalty(ActorIndex index, AttributeLevelBonusOrPenalty bonusOrPenalty);
	void addStrengthModifier(ActorIndex index, float modifer);
	void onStrengthChanged(ActorIndex index);
	void updateStrength(ActorIndex index);
	void addDextarityBonusOrPenalty(ActorIndex index, AttributeLevelBonusOrPenalty bonusOrPenalty);
	void addDextarityModifier(ActorIndex index, float modifer);
	void onDextarityChanged(ActorIndex index);
	void updateDextarity(ActorIndex index);
	void addAgilityBonusOrPenalty(ActorIndex index, AttributeLevelBonusOrPenalty bonusOrPenalty);
	void addAgilityModifier(ActorIndex index, float modifer);
	void onAgilityChanged(ActorIndex index);
	void updateAgility(ActorIndex index);
	void addIntrinsicMassBonusOrPenalty(ActorIndex index, uint32_t bonusOrPenalty);
	void addIntrinsicMassModifier(ActorIndex index, float modifer);
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
	[[nodiscard]] int32_t getIntrinsicMassBonusOrPenalty(ActorIndex index) const;
	[[nodiscard]] float getIntrinsicMassModifier(ActorIndex index) const;
	// Skills.
	[[nodiscard]] SkillLevel skill_getLevel(ActorIndex index, SkillTypeId skillType) const;
	// Growth.
	void grow_maybeStart(ActorIndex index);
	void grow_stop(ActorIndex index);
	void grow_updateGrowingStatus(ActorIndex index);
	void grow_setPercent(ActorIndex index, Percent percentGrown);
	[[nodiscard]] bool grow_isGrowing(ActorIndex index) const;
	[[nodiscard]] Percent grow_getPercent(ActorIndex index) const;
	// For Line leader.
	[[nodiscard]] BlockIndices lineLead_getPath(ActorIndex index) const;
	[[nodiscard]] BlockIndices lineLead_getOccupiedBlocks(ActorIndex index) const;
	[[nodiscard]] bool lineLead_pathEmpty(ActorIndex index) const;
	[[nodiscard]] ShapeId lineLead_getLargestShape(ActorIndex index) const;
	[[nodiscard]] MoveTypeId lineLead_getMoveType(ActorIndex index) const;
	void lineLead_clearPath(ActorIndex index);
	void lineLead_appendToPath(ActorIndex index, BlockIndex block);
	void lineLead_pushFront(ActorIndex index, BlockIndex block);
	void lineLead_popBackUnlessOccupiedByFollower(ActorIndex index);
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
	MoveEvent(Step delay, Area& area, ActorIndex actor, const Step start = Step::null());
	MoveEvent(Simulation& simulation, const Json& data);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	void onMoveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_actor == oldIndex.toActor()); m_actor = ActorIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class AttackCoolDownEvent final : public ScheduledEvent
{
	ActorIndex m_actor;
public:
	AttackCoolDownEvent(Area& area, ActorIndex index, Step duration, const Step start = Step::null());
	AttackCoolDownEvent(Simulation& simulation, const Json& data);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	void onMoveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_actor == oldIndex.toActor()); m_actor = ActorIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class GetIntoAttackPositionPathRequest final : public PathRequest
{
	ActorIndex m_actor;
	ActorReference m_target;
	DistanceInBlocksFractional m_attackRangeSquared = DistanceInBlocksFractional::null();
public:
	GetIntoAttackPositionPathRequest(Area& area, ActorIndex a, ActorIndex t, DistanceInBlocksFractional ar);
	GetIntoAttackPositionPathRequest(Area& area, const Json& data);
	void callback(Area& area, FindPathResult&);
	void onMoveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_actor == oldIndex.toActor()); m_actor = ActorIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() { return "attack"; }
};
inline void to_json(Json& data, const std::unique_ptr<GetIntoAttackPositionPathRequest>& pathRequest) { data = pathRequest->toJson(); }
struct Attack final
{
	AttackTypeId attackType;
	MaterialTypeId materialType;
	ItemIndex item; // Can be null for natural weapons.
};
