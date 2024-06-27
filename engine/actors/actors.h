#pragma once


#include "../datetime.h"
#include "../portables.h"
#include "../types.h"
#include "../visionFacade.h"
#include "../body.h"
#include "../objective.h"
#include "../equipment.h"
#include "../attributes.h"
#include "../actorOrItemIndex.h"
#include "../findsPath.h"
#include "../skill.h"
#include "../terrainFacade.h"
#include "../pathRequest.h"

struct MoveType;
struct AnimalSpecies;
struct Faction;
struct ItemType;
struct FluidType;
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
class ActorHasUniform;

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
	void generateEquipment(Area& area, ActorIndex actor);
};
class Actors final : public Portables
{
	BloomHashMap<ActorIndex, CanReserve> m_canReserve;
	BloomHashMap<ActorIndex, ActorHasUniform> m_hasUniform;
	BloomHashMap<ActorIndex, EquipmentSet> m_equipmentSet;
	std::vector<ActorId> m_id;
	std::vector<std::wstring> m_name;
	std::vector<const AnimalSpecies*> m_species;
	std::vector<Project*> m_project;
	std::vector<Step> m_birthStep;
	std::vector<CauseOfDeath> m_causeOfDeath;
	std::vector<Mass> m_unencomberedCarryMass;
	std::vector<Attributes> m_attributes;
	std::vector<HasObjectives> m_hasObjectives;
	std::vector<std::unique_ptr<Body>> m_body;
	std::vector<std::unique_ptr<MustSleep>> m_mustSleep;
	std::vector<std::unique_ptr<MustDrink>> m_mustDrink;
	std::vector<std::unique_ptr<MustEat>> m_mustEat; 
	std::vector<std::unique_ptr<CanGrow>> m_canGrow;
	std::vector<std::unique_ptr<ActorNeedsSafeTemperature>> m_needsSafeTemperature;
	std::vector<SkillSet> m_skillSet;
	// CanPickUp.
	std::vector<ActorOrItemIndex> m_carrying;
	// Stamina.
	std::vector<uint32_t> m_stamina;
	// Vision.
	std::vector<std::vector<ActorIndex>> m_canSee;
	std::vector<DistanceInBlocks> m_visionRange;
	std::vector<HasVisionFacade> m_hasVisionFacade;
	// Combat.
	HasScheduledEvents<AttackCoolDownEvent> m_coolDownEvent;
	std::vector<std::unique_ptr<GetIntoAttackPositionPathRequest>> m_getIntoAttackPositionPathRequest;
	std::vector<std::vector<std::pair<uint32_t, Attack>>> m_meleeAttackTable;
	//TODO: Should be a vector.
	std::vector<std::unordered_set<ActorIndex>> m_targetedBy;
	std::vector<ActorIndex> m_target;
	std::vector<Step> m_onMissCoolDownMelee;
	std::vector<float> m_maxMeleeRange;
	std::vector<float> m_maxRange;
	std::vector<float> m_coolDownDurationModifier;
	std::vector<uint32_t> m_combatScore;
	// Move.
	HasScheduledEvents<MoveEvent> m_moveEvent;
	std::vector<std::unique_ptr<PathRequest>> m_pathRequest;
	std::vector<std::vector<BlockIndex>> m_path;
	std::vector<std::vector<BlockIndex>::iterator> m_pathIter;
	std::vector<BlockIndex> m_destination;
	std::vector<Speed> m_speedIndividual;
	std::vector<Speed> m_speedActual;
	std::vector<uint8_t> m_moveRetries;
	void resize(HasShapeIndex newSize);
	void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	[[nodiscard]] bool indexCanBeMoved(HasShapeIndex index) const;
	Area& m_area;
public:
	Actors(Area& area);
	Actors(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void onChangeAmbiantSurfaceTemperature();
	ActorIndex create(ActorParamaters params);
	void sharedConstructor(Area& area, ActorIndex index);
	void scheduleNeeds(ActorIndex index);
	void resetNeeds(ActorIndex index);
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
	void setFaction(ActorIndex index, Faction* faction);
	void reserveAllBlocksAtLocationAndFacing(ActorIndex index, BlockIndex location, Facing facing);
	void unreserveAllBlocksAtLocationAndFacing(ActorIndex index, BlockIndex location, Facing facing);
	void setBirthStep(ActorIndex index, Step step);
	[[nodiscard]] ActorId getId(ActorIndex index) const { return m_id.at(index); }
	[[nodiscard]] bool isAlive(ActorIndex index) const { return m_causeOfDeath.at(index) == CauseOfDeath::none; }
	[[nodiscard]] Percent getPercentGrown(ActorIndex index) const;
	[[nodiscard]] CauseOfDeath getCauseOfDeath(ActorIndex index) const { assert(!isAlive(index)); return m_causeOfDeath.at(index); }
	[[nodiscard]] bool isEnemy(ActorIndex actor, ActorIndex other) const;
	[[nodiscard]] bool isAlly(ActorIndex actor, ActorIndex other) const;
	//TODO: Zombies are not sentient.
	[[nodiscard]] bool isSentient(ActorIndex index) const { return m_species.at(index)->sentient; }
	[[nodiscard]] bool isInjured(ActorIndex index) const;
	[[nodiscard]] bool canMove(ActorIndex index) const;
	[[nodiscard]] Mass getMass(ActorIndex index) const;
	[[nodiscard]] Volume getVolume(ActorIndex index) const;
	[[nodiscard]] Quantity getAgeInYears(ActorIndex index) const;
	[[nodiscard]] Step getAge(ActorIndex index) const;
	[[nodiscard]] Step getBirthStep(ActorIndex index) const { return m_birthStep.at(index); }
	[[nodiscard]] std::wstring getActionDescription(ActorIndex index) const;
	[[nodiscard]] EquipmentSet& getEquipmentSet(ActorIndex index) { return m_equipmentSet.at(index); }
	[[nodiscard]] const AnimalSpecies& getSpecies(ActorIndex index) const { return *m_species.at(index); }
	[[nodiscard]] Mass getUnencomberedCarryMass(ActorIndex index) const { return m_unencomberedCarryMass.at(index); }
	// -Stamina.
	void stamina_recover(ActorIndex index);
	void stamina_spend(ActorIndex index, uint32_t stamina);
	void stamina_setFull(ActorIndex index);
	bool stamina_hasAtLeast(ActorIndex index, uint32_t stamina) const;
	bool stamina_isFull(ActorIndex index) const;
	[[nodiscard]] uint32_t stamina_getMax(ActorIndex index) const;
	[[nodiscard]] uint32_t stamina_get(ActorIndex index) const { return m_stamina.at(index); }
	// -Vision.
	void vision_do(ActorIndex index, std::vector<ActorIndex>& actors);
	void vision_setRange(ActorIndex index, DistanceInBlocks range);
	void vision_createFacadeIfCanSee(ActorIndex index);
	void vision_recordFacade(ActorIndex actor, VisionFacade& facade, VisionFacadeIndex facadeIndex);
	void vision_clearFacade(ActorIndex actor);
	void vision_updateFacadeIndex(ActorIndex actor, VisionFacadeIndex);
	void vision_swap(ActorIndex actor, std::vector<ActorIndex>& toSwap);
	[[nodiscard]] std::vector<ActorIndex>& vision_getCanSee(ActorIndex index) { return m_canSee.at(index); }
	[[nodiscard]] DistanceInBlocks vision_getRange(ActorIndex index) const { return m_visionRange.at(index); }
	[[nodiscard]] bool vision_canSeeActor(ActorIndex index, ActorIndex other) const;
	[[nodiscard]] bool vision_canSeeAnything(ActorIndex index) const;
	[[nodiscard]] bool vision_hasFacade(ActorIndex actor) const;
	[[nodiscard]] std::pair<VisionFacade*, VisionFacadeIndex> vision_getFacadeWithIndex(ActorIndex actor) const;
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
	[[nodiscard]] uint32_t combat_getCurrentMeleeCombatScore(ActorIndex index);
	[[nodiscard]] bool combat_isOnCoolDown(ActorIndex index) const;
	[[nodiscard]] bool combat_inRange(ActorIndex index, const ActorIndex target) const;
	[[nodiscard]] bool combat_doesProjectileHit(ActorIndex index, Attack& attack, const ActorIndex target) const;
	[[nodiscard]] Percent combat_projectileHitPercent(ActorIndex index, const Attack& attack, const ActorIndex target) const;
	[[nodiscard]] const float& combat_getMaxMeleeRange(ActorIndex index) const { return m_maxMeleeRange.at(index); }
	[[nodiscard]] const float& combat_getMaxRange(ActorIndex index) const { return m_maxRange.at(index) != 0 ? m_maxRange.at(index) : m_maxMeleeRange.at(index); }
	[[nodiscard]] uint32_t combat_getCombatScoreForAttack(ActorIndex index, const Attack& attack) const;
	[[nodiscard]] const Attack& combat_getAttackForCombatScoreDifference(ActorIndex index, const uint32_t scoreDifference) const;
	[[nodiscard]] float combat_getQualityModifier(ActorIndex index, uint32_t quality) const;
	[[nodiscard]] bool combat_blockIsValidPosition(ActorIndex index, const BlockIndex block, uint32_t attackRangeSquared) const;
	AttackType& combat_getRangedAttackType(ActorIndex index, ItemIndex weapon);
	//for degbugging combat
	[[nodiscard]] std::vector<std::pair<uint32_t, Attack>>& combat_getAttackTable(ActorIndex index) { return m_meleeAttackTable.at(index); }
	[[nodiscard]] bool combat_hasThreadedTask(ActorIndex index) { return m_getIntoAttackPositionPathRequest.at(index) != nullptr; }
	[[nodiscard]] float combat_getCoolDownDurationModifier(ActorIndex index) { return m_coolDownDurationModifier.at(index); }
	[[nodiscard, maybe_unused]] uint32_t combat_getCombatScore(ActorIndex index) const { return m_combatScore.at(index); }
	// -Move.
	void move_updateIndividualSpeed(ActorIndex index);
	void move_updateActualSpeed(ActorIndex index);
	void move_setType(ActorIndex index, const MoveType& moveType);
	void move_setPath(ActorIndex index, std::vector<BlockIndex>& path);
	void move_clearPath(ActorIndex index);
	void move_callback(ActorIndex index);
	void move_schedule(ActorIndex index);
	void move_setDestination(ActorIndex index, BlockIndex destination, bool detour = false, bool adjacent = false, bool unreserved = true, bool reserve = true);
	void move_setDestinationAdjacentToLocation(ActorIndex index, BlockIndex destination, bool detour = false, bool unreserved = true, bool reserve = true);
	void move_setDestinationAdjacentToActor(ActorIndex index, ActorIndex other, bool detour = false, bool unreserved = true, bool reserve = true);
	void move_setDestinationAdjacentToItem(ActorIndex index, ItemIndex item, bool detour = false, bool unreserved = true, bool reserve = true);
	void move_setDestinationAdjacentToPlant(ActorIndex index, PlantIndex plant, bool detour = false, bool unreserved = true, bool reserve = true);
	void move_setDestinationAdjacentToFluidType(ActorIndex index, const FluidType& fluidType, bool detour = false, bool unreserved = true, bool reserve = true, DistanceInBlocks maxRange = BLOCK_INDEX_MAX);
	void move_setDestinationAdjacentToDesignation(ActorIndex index, BlockDesignation designation, bool detour = false, bool unreserved = true, bool reserve = true, DistanceInBlocks maxRange = BLOCK_INDEX_MAX);
	void move_setDestinationToEdge(ActorIndex index, bool detour = false);
	void move_clearAllEventsAndTasks(ActorIndex index);
	void move_onDeath(ActorIndex index);
	void move_onLeaveArea(ActorIndex index);
	void move_pathRequestCallback(ActorIndex index, std::vector<BlockIndex>, bool useCurrentLocation, bool reserveDestination);
	void move_pathRequestMaybeCancel(ActorIndex index);
	void move_pathRequestRecord(ActorIndex index, std::unique_ptr<PathRequest> pathRequest);
	[[nodiscard]] bool move_tryToReserveProposedDestination(ActorIndex index, std::vector<BlockIndex>& path);
	[[nodiscard]] bool move_tryToReserveOccupied(ActorIndex index);
	[[nodiscard]] const MoveType& move_getType(ActorIndex index) const { return *m_moveType.at(index); }
	[[nodiscard]] Speed move_getIndividualSpeedWithAddedMass(ActorIndex index, Mass mass) const;
	[[nodiscard]] Speed move_getSpeed(ActorIndex index) const { return m_speedActual.at(index); }
	[[nodiscard]] bool move_canMove(ActorIndex index) const;
	[[nodiscard]] Step move_delayToMoveInto(ActorIndex index, const BlockIndex block) const;
	// For debugging move.
	[[nodiscard]] PathRequest& move_getPathRequest(ActorIndex index) { return *m_pathRequest.at(index).get(); }
	[[nodiscard]] std::vector<BlockIndex>& move_getPath(ActorIndex index) { return m_path.at(index); }
	[[nodiscard]] BlockIndex move_getDestination(ActorIndex index) { return m_destination.at(index); }
	[[nodiscard]] bool move_hasEvent(ActorIndex index) const { return m_moveEvent.exists(index); }
	[[nodiscard]] bool move_hasPathRequest(ActorIndex index) const { return m_pathRequest.at(index).get() != nullptr; }
	[[nodiscard]] Step move_stepsTillNextMoveEvent(ActorIndex index) const;
	// -Uniform.
	void uniform_set(ActorIndex index, Uniform& uniform);
	void uniform_unset(ActorIndex index);
	[[nodiscard]] bool uniform_exists(ActorIndex index) const;
	[[nodiscard]] Uniform& uniform_get(ActorIndex index);
	[[nodiscard]] const Uniform& uniform_get(ActorIndex index) const;
	// -CanPickUp.
	void canPickUp_pickUpItem(ActorIndex index, ItemIndex item);
	void canPickUp_pickUpItemQuantity(ActorIndex index, ItemIndex item, Quantity quantity);
	void canPickUp_pickUpActor(ActorIndex index, ItemIndex actor);
	ActorIndex canPickUp_putDownActor(ActorIndex index, BlockIndex location);
	// Returns a reference to has shape, which may be newly created or pre-existing due to generic items.
	ItemIndex canPickUp_putDownItem(ItemIndex index, BlockIndex location);
	ActorOrItemIndex canPickUp_putDownIfAny(ActorIndex index, BlockIndex location);
	void canPickUp_removeFluidVolume(ActorIndex index, CollisionVolume volume);
	void canPickUp_add(ActorIndex index, const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	void canPickUp_remove(ActorIndex index, ItemIndex item);
	ItemIndex canPickUp_getItem(ActorIndex index);
	ActorIndex canPickUp_getActor(ActorIndex index);
	[[nodiscard]] bool canPickUp_actor(ActorIndex index, ActorIndex other) const;
	[[nodiscard]] Quantity canPickUp_quantityWhichCanBePickedUpUnencombered(ActorIndex index, const ItemType& itemType, const MaterialType& materialType) const;
	[[nodiscard]] bool canPickUp_any(ActorIndex index, ActorOrItemIndex target) const;
	[[nodiscard]] bool canPickUp_anyUnencombered(ActorIndex index, ActorOrItemIndex target) const;
	[[nodiscard]] bool canPickUp_anyUnencomberedItem(ActorIndex index, ItemIndex target) const;
	[[nodiscard]] bool canPickUp_anyWithMass(ActorIndex index, Mass mass) const;
	[[nodiscard]] bool canPickUp_exists(ActorIndex index) const { return m_carrying.at(index).exists(); }
	[[nodiscard]] bool canPickUp_isCarryingActor(ActorIndex index, ActorIndex actor) const { return m_carrying.at(index).exists() && m_carrying.at(index).isActor() && m_carrying.at(index).get() == actor; }
	[[nodiscard]] bool canPickUp_isCarryingItem(ActorIndex index, ItemIndex item) const { return m_carrying.at(index).exists() && m_carrying.at(index).isItem() && m_carrying.at(index).get() == item; }
	[[nodiscard]] bool canPickUp_isCarryingItemGeneric(ActorIndex index, const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const;
	[[nodiscard]] bool canPickUp_isCarryingFluidType(ActorIndex index, const FluidType& fluidType) const;
	[[nodiscard]] CollisionVolume canPickUp_getFluidVolume(ActorIndex index) const;
	[[nodiscard]] const FluidType& canPickUp_getFluidType(ActorIndex index) const;
	[[nodiscard]] bool canPickUp_isCarryingEmptyContainerWhichCanHoldFluid(ActorIndex index) const;
	[[nodiscard]] Mass canPickUp_getMass(ActorIndex index) const;
	[[nodiscard]] Speed canPickUp_speedIfCarryingQuantity(ActorIndex index, Mass mass, Quantity quantity) const;
	[[nodiscard]] Quantity canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(ActorIndex index, Mass mass, Speed minimumSpeed) const;
	// Objectives.
	void objective_addTaskToStart(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_addTaskToEnd(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_addNeed(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_replaceTasks(ActorIndex index, std::unique_ptr<Objective> objective);
	void objective_canNotCompleteSubobjective(ActorIndex index);
	void objective_canNotCompleteObjective(ActorIndex index, Objective& objective);
	void objective_canNotFulfillNeed(ActorIndex index, Objective& objective);
	void objective_maybeDoNext(ActorIndex index);
	void objective_setPriority(ActorIndex index, const ObjectiveType& objectiveType, uint8_t priority);
	void objective_reset(ActorIndex index);
	void objective_projectCannotReserve(ActorIndex index);
	void objective_complete(ActorIndex index, Objective& objective);
	void objective_subobjectiveComplete(ActorIndex index);
	[[nodiscard]] bool objective_exists(ActorIndex index);
	[[nodiscard]] std::string objective_getCurrentName(ActorIndex index);
	template<typename T>
	T& objective_getCurrent(ActorIndex index) { return static_cast<T&>(m_hasObjectives.at(index).getCurrent()); }
	// For testing.
	[[nodiscard]] bool objective_queuesAreEmpty(ActorIndex index);
	// CanReserve.
	void canReserve_clearAll(ActorIndex index);
	void canReserve_setFaction(ActorIndex index, Faction* faction);
	void canReserve_reserveLocation(ActorIndex index, BlockIndex block);
	void canReserve_reserveItem(ActorIndex index, ItemIndex item);
	[[nodiscard]] bool canReserve_tryToReserveLocation(ActorIndex index, BlockIndex block);
	[[nodiscard]] bool canReserve_tryToReserveItem(ActorIndex index, ItemIndex item);
	//TODO: this method name makes no sense.
	[[nodiscard]] bool canReserve_factionExists(ActorIndex index) const;
	[[nodiscard]] bool canReserve_hasReservationWith(ActorIndex index, Reservable& reservable) const;
	[[nodiscard]] bool canReserve_hasReservations(ActorIndex index) const;
	// Project.
	[[nodiscard]] bool project_exists(ActorIndex index) const { return m_project.at(index) != nullptr; }
	[[nodiscard]] Project* project_get(ActorIndex index) const { return m_project.at(index); }
	void project_set(ActorIndex index, Project& project) { m_project[index] = &project; }
	void project_unset(ActorIndex index) { m_project[index] = nullptr; }
	// Sleep.
	void sleep(ActorIndex actor);
	void setSleepSpot(ActorIndex index, BlockIndex location);
	[[nodiscard]] BlockIndex getSleepSpot(ActorIndex index) const ;
	[[nodiscard]] bool isAwake(ActorIndex index) const;
	// Drink.
	void drink_do(ActorIndex index, CollisionVolume volume);
	void drink_setNeedsFluid(ActorIndex index);
	[[nodiscard]] CollisionVolume drink_getVolumeOfFluidRequested(ActorIndex index);
	[[nodiscard]] bool drink_needsFluid(ActorIndex index) const;
	[[nodiscard]] const FluidType& drink_getFluidType(ActorIndex index) const;
	// Eat.
	void eat_do(ActorIndex index, Mass mass);
	[[nodiscard]] bool eat_canEatActor(ActorIndex index, ActorIndex actor) const;
	[[nodiscard]] bool eat_canEatItem(ActorIndex index, ItemIndex item) const;
	[[nodiscard]] bool eat_canEatPlant(ActorIndex index, PlantIndex plant) const;
	[[nodiscard]] Percent eat_getPercentStarved(ActorIndex index) const;
	// Temperature.
	[[nodiscard]] bool temperature_isSafe(ActorIndex index, Temperature temperature) const;
	[[nodiscard]] bool temperature_isSafeAtCurrentLocation(ActorIndex index) const;
	// Attributes.
	[[nodiscard]] uint32_t getStrength(ActorIndex index) const;
	[[nodiscard]] uint32_t getDextarity(ActorIndex index) const;
	[[nodiscard]] uint32_t getAgility(ActorIndex index) const;
	// Skills.
	[[nodiscard]] uint32_t skill_getLevel(ActorIndex index, const SkillType& skillType) const;
	// Growth.
	void grow_maybeStart(ActorIndex index);
	void grow_stop(ActorIndex index);
	void grow_updateGrowingStatus(ActorIndex index);
	void grow_updatePercent(ActorIndex index, Percent percentGrown);
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
};
class MoveEvent final : public ScheduledEvent
{
	ActorIndex m_actor = ACTOR_INDEX_MAX;
public:
	MoveEvent(Step delay, Area& area, ActorIndex actor, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class AttackCoolDownEvent final : public ScheduledEvent
{
	Area& m_area;
	ActorIndex m_index = ACTOR_INDEX_MAX;
public:
	AttackCoolDownEvent(Area& area, ActorIndex index, Step duration, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class GetIntoAttackPositionPathRequest final : public PathRequest
{
	ActorIndex m_actor = ACTOR_INDEX_MAX;
	ActorIndex m_target = ACTOR_INDEX_MAX;
	DistanceInBlocks m_attackRangeSquared = BLOCK_DISTANCE_MAX;
public:
	GetIntoAttackPositionPathRequest(Area& area, ActorIndex a, ActorIndex t, float ar);
	void callback(Area& area, FindPathResult);
};
struct Attack final
{
	// Use pointers rather then references to allow move.
	const AttackType* attackType = nullptr;
	const MaterialType* materialType = nullptr;
	ItemIndex item = ITEM_INDEX_MAX; // Can be ITEM_INDEX_MAX for natural weapons.
};
