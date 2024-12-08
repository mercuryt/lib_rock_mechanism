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
	void generateEquipment(Area& area, const ActorIndex& actor);
};
class Actors final : public Portables<Actors, ActorIndex, ActorReferenceIndex>
{
	ActorIndexSet m_onSurface;
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
	void resize(const ActorIndex& newSize);
	void moveIndex(const ActorIndex& oldIndex, const ActorIndex& newIndex);
	void destroy(const ActorIndex& index);
	[[nodiscard]] bool indexCanBeMoved(const HasShapeIndex& index) const;
public:
	Actors(Area& area);
	~Actors();
	void load(const Json& data);
	void loadObjectivesAndReservations(const Json& data);
	void onChangeAmbiantSurfaceTemperature();
	ActorIndex create(ActorParamaters params);
	void sharedConstructor(const ActorIndex& index);
	void scheduleNeeds(const ActorIndex& index);
	void resetNeeds(const ActorIndex& index);
	void setShape(const ActorIndex& index, const ShapeId& shape);
	void setLocation(const ActorIndex& index, const BlockIndex& block);
	void setLocationAndFacing(const ActorIndex& index, const BlockIndex& block, const Facing& facing);
	void exit(const ActorIndex& index);
	void removeMassFromCorpse(const ActorIndex& index, const Mass& mass);
	void die(const ActorIndex& index, CauseOfDeath causeOfDeath);
	void passout(const ActorIndex& index, const Step& duration);
	void leaveArea(const ActorIndex& index);
	void wait(const ActorIndex& index, const Step& duration);
	void takeHit(const ActorIndex& index, Hit& hit, BodyPart& bodyPart);
	// May be null.
	void setFaction(const ActorIndex& index, const FactionId& faction);
	void reserveAllBlocksAtLocationAndFacing(const ActorIndex& index, const BlockIndex& location, const Facing& facing);
	void unreserveAllBlocksAtLocationAndFacing(const ActorIndex& index, const BlockIndex& location, const Facing& facing);
	void setBirthStep(const ActorIndex& index, const Step& step);
	[[nodiscard]] ActorIndices getAll() const;
	[[nodiscard]] ActorIndexSet getOnSurface() const { return m_onSurface; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ActorId getId(const ActorIndex& index) const { return m_id[index]; }
	[[nodiscard]] std::wstring getName(const ActorIndex& index) const { return m_name[index]; }
	[[nodiscard]] bool isAlive(const ActorIndex& index) const { return m_causeOfDeath[index] == CauseOfDeath::none; }
	[[nodiscard]] Percent getPercentGrown(const ActorIndex& index) const;
	[[nodiscard]] CauseOfDeath getCauseOfDeath(const ActorIndex& index) const { assert(!isAlive(index)); return m_causeOfDeath[index]; }
	[[nodiscard]] bool isEnemy(const ActorIndex& actor, const ActorIndex& other) const;
	[[nodiscard]] bool isAlly(const ActorIndex& actor, const ActorIndex& other) const;
	[[nodiscard]] bool isSentient(const ActorIndex& index) const;
	[[nodiscard]] bool isInjured(const ActorIndex& index) const;
	[[nodiscard]] bool canMove(const ActorIndex& index) const;
	[[nodiscard]] Volume getVolume(const ActorIndex& index) const;
	[[nodiscard]] Mass getMass(const ActorIndex& index) const;
	[[nodiscard]] Quantity getAgeInYears(const ActorIndex& index) const;
	[[nodiscard]] Step getAge(const ActorIndex& index) const;
	[[nodiscard]] Step getBirthStep(const ActorIndex& index) const { return m_birthStep[index]; }
	[[nodiscard]] std::wstring getActionDescription(const ActorIndex& index) const;
	[[nodiscard]] AnimalSpeciesId getSpecies(const ActorIndex& index) const { return m_species[index]; }
	[[nodiscard]] Mass getUnencomberedCarryMass(const ActorIndex& index) const { return m_unencomberedCarryMass[index]; }
	// -Stamina.
	void stamina_recover(const ActorIndex& index);
	void stamina_spend(const ActorIndex& index, const Stamina& stamina);
	void stamina_setFull(const ActorIndex& index);
	bool stamina_hasAtLeast(const ActorIndex& index, const Stamina& stamina) const;
	bool stamina_isFull(const ActorIndex& index) const;
	[[nodiscard]] Stamina stamina_getMax(const ActorIndex& index) const;
	[[nodiscard]] Stamina stamina_get(const ActorIndex& index) const { return m_stamina[index]; }
	// -Vision.
	void vision_do(const ActorIndex& index, ActorIndices& actors);
	void vision_setRange(const ActorIndex& index, const DistanceInBlocks& range);
	void vision_createFacadeIfCanSee(const ActorIndex& index);
	void vision_clearFacade(const ActorIndex& index);
	void vision_swap(const ActorIndex& index, ActorIndices& toSwap);
	[[nodiscard]] ActorIndices& vision_getCanSee(const ActorIndex& index) { return m_canSee[index]; }
	[[nodiscard]] DistanceInBlocks vision_getRange(const ActorIndex& index) const { return m_visionRange[index]; }
	[[nodiscard]] bool vision_canSeeActor(const ActorIndex& index, const ActorIndex& other) const;
	[[nodiscard]] bool vision_canSeeAnything(const ActorIndex& index) const;
	[[nodiscard]] bool vision_hasFacade(const ActorIndex& actor) const;
	[[nodiscard]] std::pair<VisionFacade*, VisionFacadeIndex> vision_getFacadeWithIndex(const ActorIndex& actor) const;
	// To be used by VisionFacade only.
	[[nodiscard]] HasVisionFacade& vision_getHasVisionFacade(const ActorIndex& index) { return m_hasVisionFacade[index]; }
	// For testing.
	[[nodiscard]] VisionFacade& vision_getFacadeBucket(const ActorIndex& index);
	// -Combat.
	void combat_attackMeleeRange(const ActorIndex& index, const ActorIndex& target);
	void combat_attackLongRange(const ActorIndex& index, const ActorIndex& target, const ItemIndex& weapon, const ItemIndex& ammo);
	void combat_coolDownCompleted(const ActorIndex& index);
	void combat_update(const ActorIndex& index);
	void combat_setTarget(const ActorIndex& index, const ActorIndex& actor);
	void combat_recordTargetedBy(const ActorIndex& index, const ActorIndex& actor);
	void combat_removeTargetedBy(const ActorIndex& index, const ActorIndex& actor);
	void combat_onMoveFrom(const ActorIndex& index, const BlockIndex& previous);
	void combat_onDeath(const ActorIndex& index);
	void combat_onLeaveArea(const ActorIndex& index);
	void combat_noLongerTargetable(const ActorIndex& index);
	void combat_targetNoLongerTargetable(const ActorIndex& index);
	void combat_onTargetMoved(const ActorIndex& index);
	void combat_freeHit(const ActorIndex& index, const ActorIndex& actor);
	//TODO: Combat vs. items?
	void combat_getIntoRangeAndLineOfSightOfActor(const ActorIndex& index, const ActorIndex& target, const DistanceInBlocksFractional& range);
	[[nodiscard]] CombatScore combat_getCurrentMeleeCombatScore(const ActorIndex& index);
	[[nodiscard]] bool combat_isOnCoolDown(const ActorIndex& index) const;
	[[nodiscard]] bool combat_inRange(const ActorIndex& index, const ActorIndex& target) const;
	[[nodiscard]] bool combat_doesProjectileHit(const ActorIndex& index, Attack& attack, const ActorIndex& target) const;
	[[nodiscard]] Percent combat_projectileHitPercent(const ActorIndex& index, const Attack& attack, const ActorIndex& target) const;
	[[nodiscard]] DistanceInBlocksFractional combat_getMaxMeleeRange(const ActorIndex& index) const { return m_maxMeleeRange[index]; }
	[[nodiscard]] DistanceInBlocksFractional combat_getMaxRange(const ActorIndex& index) const { return m_maxRange[index] != DistanceInBlocksFractional::create(0) ? m_maxRange[index] : m_maxMeleeRange[index]; }
	[[nodiscard]] CombatScore combat_getCombatScoreForAttack(const ActorIndex& index, const Attack& attack) const;
	[[nodiscard]] const Attack& combat_getAttackForCombatScoreDifference(const ActorIndex& index, CombatScore scoreDifference) const;
	[[nodiscard]] float combat_getQualityModifier(const ActorIndex& index, const Quality& quality) const;
	[[nodiscard]] bool combat_blockIsValidPosition(const ActorIndex& index, const BlockIndex& block, const DistanceInBlocksFractional& attackRangeSquared) const;
	AttackTypeId combat_getRangedAttackType(const ActorIndex& index, const ItemIndex& weapon);
	//for degbugging combat
	[[nodiscard]] std::vector<std::pair<CombatScore, Attack>>& combat_getAttackTable(const ActorIndex& index) { return m_meleeAttackTable[index]; }
	[[nodiscard]] float combat_getCoolDownDurationModifier(const ActorIndex& index) { return m_coolDownDurationModifier[index]; }
	[[nodiscard, maybe_unused]] CombatScore combat_getCombatScore(const ActorIndex& index) const { return m_combatScore[index]; }
	[[nodiscard]] std::vector<std::pair<CombatScore, Attack>>& combat_getMeleeAttacks(const ActorIndex& index);
	// -Body.
	[[nodiscard]] Percent body_getImpairMovePercent(const ActorIndex& index);
	//TODO: change to getImpairDextarityPercent?
	[[nodiscard]] Percent body_getImpairManipulationPercent(const ActorIndex& index);
	[[nodiscard]] Step body_getStepsTillWoundsClose(const ActorIndex& index);
	[[nodiscard]] Step body_getStepsTillBleedToDeath(const ActorIndex& index);
	[[nodiscard]] bool body_hasBleedEvent(const ActorIndex& index) const;
	[[nodiscard]] bool body_isInjured(const ActorIndex& index) const;
	[[nodiscard]] BodyPart& body_pickABodyPartByVolume(const ActorIndex& index) const;
	[[nodiscard]] BodyPart& body_pickABodyPartByType(const ActorIndex& index, const BodyPartTypeId& bodyPartType) const;
	// -Move.
	void move_updateIndividualSpeed(const ActorIndex& index);
	void move_updateActualSpeed(const ActorIndex& index);
	void move_setType(const ActorIndex& index, const MoveTypeId& moveType);
	void move_setPath(const ActorIndex& index, const BlockIndices& path);
	void move_setMoveSpeedActualForLeading(const ActorIndex& index, Speed speed);
	void move_clearPath(const ActorIndex& index);
	void move_callback(const ActorIndex& index);
	void move_schedule(const ActorIndex& index);
	void move_setDestination(const ActorIndex& index, const BlockIndex& destination, bool detour = false, bool adjacent = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToLocation(const ActorIndex& index, const BlockIndex& destination, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToActor(const ActorIndex& index, const ActorIndex& other, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToItem(const ActorIndex& index, const ItemIndex& item, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToPolymorphic(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToPlant(const ActorIndex& index, const PlantIndex& plant, bool detour = false, bool unreserved = false, bool reserve = false);
	void move_setDestinationAdjacentToFluidType(const ActorIndex& index, const FluidTypeId& fluidType, bool detour = false, bool unreserved = false, bool reserve = false, const DistanceInBlocks maxRange = DistanceInBlocks::max());
	void move_setDestinationAdjacentToDesignation(const ActorIndex& index, const BlockDesignation& designation, bool detour = false, bool unreserved = false, bool reserve = false, const DistanceInBlocks maxRange = DistanceInBlocks::max());
	void move_setDestinationToEdge(const ActorIndex& index, bool detour = false);
	void move_clearAllEventsAndTasks(const ActorIndex& index);
	void move_onDeath(const ActorIndex& index);
	void move_onLeaveArea(const ActorIndex& index);
	void move_pathRequestCallback(const ActorIndex& index, BlockIndices path, bool useCurrentLocation, bool reserveDestination);
	void move_pathRequestMaybeCancel(const ActorIndex& index);
	void move_pathRequestRecord(const ActorIndex& index, std::unique_ptr<PathRequest> pathRequest);
	void move_updatePathRequestTerrainFacadeIndex(const ActorIndex& index, const PathRequestIndex& newPathRequestIndex);
	[[nodiscard]] bool move_destinationIsAdjacentToLocation(const ActorIndex& index, const BlockIndex& location);
	[[nodiscard]] bool move_tryToReserveProposedDestination(const ActorIndex& index, const BlockIndices& path);
	[[nodiscard]] bool move_tryToReserveOccupied(const ActorIndex& index);
	[[nodiscard]] Speed move_getIndividualSpeedWithAddedMass(const ActorIndex& index, const Mass& mass) const;
	[[nodiscard]] Speed move_getSpeed(const ActorIndex& index) const { return m_speedActual[index]; }
	[[nodiscard]] bool move_canMove(const ActorIndex& index) const;
	[[nodiscard]] Step move_delayToMoveInto(const ActorIndex& index, const BlockIndex& block) const;
	[[nodiscard]] std::unique_ptr<PathRequest> move_movePathRequestData(const ActorIndex& index) { auto output = std::move(m_pathRequest[index]); m_pathRequest[index] = nullptr; return output; }
	// For debugging move.
	[[nodiscard]] PathRequest& move_getPathRequest(const ActorIndex& index) { return *m_pathRequest[index].get(); }
	[[nodiscard]] BlockIndices& move_getPath(const ActorIndex& index) { return m_path[index]; }
	[[nodiscard]] BlockIndex move_getDestination(const ActorIndex& index) { return m_destination[index]; }
	[[nodiscard]] bool move_hasEvent(const ActorIndex& index) const { return m_moveEvent.exists(index); }
	[[nodiscard]] bool move_hasPathRequest(const ActorIndex& index) const { return m_pathRequest[index].get() != nullptr; }
	[[nodiscard]] Step move_stepsTillNextMoveEvent(const ActorIndex& index) const;
	[[nodiscard]] uint8_t move_getRetries(const ActorIndex& index) const { return m_moveRetries[index]; }
	[[nodiscard]] bool move_canPathTo(const ActorIndex& index, const BlockIndex& destination);
	[[nodiscard]] bool move_canPathFromTo(const ActorIndex& index, const BlockIndex& start, const Facing& startFacing, const BlockIndex& destination);
	// -CanPickUp.
	void canPickUp_pickUpItem(const ActorIndex& index, const ItemIndex& item);
	void canPickUp_pickUpItemQuantity(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity);
	void canPickUp_pickUpActor(const ActorIndex& index, const ActorIndex& actor);
	ActorOrItemIndex canPickUp_pickUpPolymorphic(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex, const Quantity& quantity);
	void canPickUp_removeFluidVolume(const ActorIndex& index, const CollisionVolume& volume);
	void canPickUp_add(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity);
	void canPickUp_removeItem(const ActorIndex& index, const ItemIndex& item);
	void canPickUp_removeActor(const ActorIndex &index, const ActorIndex &actor);
	void canPickUp_remove(const ActorIndex &index, const ActorOrItemIndex& actorOrItem);
	void canPickUp_destroyItem(const ActorIndex& index, const ItemIndex& item);
	void canPickUp_updateActorIndex(const ActorIndex& index, const ActorIndex& oldIndex, const ActorIndex& newIndex);
	void canPickUp_updateItemIndex(const ActorIndex& index, const ItemIndex& oldIndex, const ItemIndex& newIndex);
	void canPickUp_updateUnencomberedCarryMass(const ActorIndex& index);
	[[nodiscard]] ActorIndex canPickUp_tryToPutDownActor(const ActorIndex& index, const BlockIndex& location, const DistanceInBlocks maxRange = DistanceInBlocks::create(1));
	[[nodiscard]] ItemIndex canPickUp_tryToPutDownItem(const ActorIndex& index, const BlockIndex& location, const DistanceInBlocks maxRange = DistanceInBlocks::create(1));
	[[nodiscard]] ActorOrItemIndex canPickUp_tryToPutDownIfAny(const ActorIndex& index, const BlockIndex& location, const DistanceInBlocks maxRange = DistanceInBlocks::create(1));
	[[nodiscard]] ActorOrItemIndex canPickUp_tryToPutDownPolymorphic(const ActorIndex& index, const BlockIndex& location, const DistanceInBlocks maxRange = DistanceInBlocks::create(1));
	[[nodiscard]] ItemIndex canPickUp_getItem(const ActorIndex& index) const;
	[[nodiscard]] ActorIndex canPickUp_getActor(const ActorIndex& index) const;
	[[nodiscard]] ActorOrItemIndex canPickUp_getPolymorphic(const ActorIndex& index) const;
	[[nodiscard]] bool canPickUp_polymorphic(const ActorIndex& index, ActorOrItemIndex target) const;
	[[nodiscard]] bool canPickUp_singleItem(const ActorIndex& index, const ItemIndex& item) const;
	[[nodiscard]] bool canPickUp_item(const ActorIndex& index, const ItemIndex& item) const;
	[[nodiscard]] bool canPickUp_itemQuantity(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity) const;
	[[nodiscard]] bool canPickUp_actor(const ActorIndex& index, const ActorIndex& other) const;
	[[nodiscard]] bool canPickUp_anyWithMass(const ActorIndex& index, const Mass& mass) const;
	[[nodiscard]] bool canPickUp_polymorphicUnencombered(const ActorIndex& index, ActorOrItemIndex target) const;
	[[nodiscard]] bool canPickUp_itemUnencombered(const ActorIndex& index, const ItemIndex& item) const;
	[[nodiscard]] bool canPickUp_actorUnencombered(const ActorIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] bool canPickUp_anyWithMassUnencombered(const ActorIndex& index, const Mass& mass) const;
	[[nodiscard]] Quantity canPickUp_quantityWhichCanBePickedUpUnencombered(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType) const;
	[[nodiscard]] bool canPickUp_exists(const ActorIndex& index) const { return m_carrying[index].exists(); }
	[[nodiscard]] bool canPickUp_isCarryingActor(const ActorIndex& index, const ActorIndex& actor) const { return m_carrying[index].exists() && m_carrying[index].isActor() && m_carrying[index].get().toActor() == actor; }
	[[nodiscard]] bool canPickUp_isCarryingItem(const ActorIndex& index, const ItemIndex& item) const { return m_carrying[index].exists() && m_carrying[index].isItem() && m_carrying[index].get().toItem() == item; }
	[[nodiscard]] bool canPickUp_isCarryingItemGeneric(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity) const;
	[[nodiscard]] bool canPickUp_isCarryingFluidType(const ActorIndex& index, FluidTypeId fluidType) const;
	[[nodiscard]] bool canPickUp_isCarryingPolymorphic(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex) const;
	[[nodiscard]] CollisionVolume canPickUp_getFluidVolume(const ActorIndex& index) const;
	[[nodiscard]] FluidTypeId canPickUp_getFluidType(const ActorIndex& index) const;
	[[nodiscard]] bool canPickUp_isCarryingEmptyContainerWhichCanHoldFluid(const ActorIndex& index) const;
	[[nodiscard]] Mass canPickUp_getMass(const ActorIndex& index) const;
	[[nodiscard]] Speed canPickUp_speedIfCarryingQuantity(const ActorIndex& index, const Mass& mass, const Quantity& quantity) const;
	[[nodiscard]] Quantity canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(const ActorIndex& index, const Mass& mass, Speed minimumSpeed) const;
	[[nodiscard]] bool canPickUp_canPutDown(const ActorIndex& index, const BlockIndex& block);
	// Objectives.
	void objective_addTaskToStart(const ActorIndex& index, std::unique_ptr<Objective> objective);
	void objective_addTaskToEnd(const ActorIndex& index, std::unique_ptr<Objective> objective);
	void objective_addNeed(const ActorIndex& index, std::unique_ptr<Objective> objective);
	void objective_replaceTasks(const ActorIndex& index, std::unique_ptr<Objective> objective);
	void objective_canNotCompleteSubobjective(const ActorIndex& index);
	void objective_canNotCompleteObjective(const ActorIndex& index, Objective& objective);
	void objective_canNotFulfillNeed(const ActorIndex& index, Objective& objective);
	void objective_maybeDoNext(const ActorIndex& index);
	void objective_setPriority(const ActorIndex& index, const ObjectiveTypeId& objectiveType, Priority priority);
	void objective_reset(const ActorIndex& index);
	void objective_projectCannotReserve(const ActorIndex& index);
	void objective_complete(const ActorIndex& index, Objective& objective);
	void objective_subobjectiveComplete(const ActorIndex& index);
	void objective_cancel(const ActorIndex& index, Objective& objective);
	void objective_execute(const ActorIndex& index);
	[[nodiscard]] bool objective_exists(const ActorIndex& index) const;
	[[nodiscard]] bool objective_hasTask(const ActorIndex& index, const ObjectiveTypeId& objectiveTypeId) const;
	[[nodiscard]] bool objective_hasNeed(const ActorIndex& index, NeedType needType) const;
	[[nodiscard]] bool objective_hasSupressedNeed(const ActorIndex &index, NeedType needType) const;
	[[nodiscard]] Priority objective_getPriorityFor(const ActorIndex& index, const ObjectiveTypeId& objectiveType) const;
	[[nodiscard]] std::string objective_getCurrentName(const ActorIndex& index) const;
	template<typename T>
	T& objective_getCurrent(const ActorIndex& index) { return static_cast<T&>(m_hasObjectives[index]->getCurrent()); }
	// For testing.
	[[nodiscard]] bool objective_queuesAreEmpty(const ActorIndex& index) const;
	[[nodiscard]] bool objective_isOnDelay(const ActorIndex& index, const ObjectiveTypeId& objectiveTypeId) const;
	[[nodiscard]] Step objective_getDelayEndFor(const ActorIndex& index, const ObjectiveTypeId& objectiveTypeId) const;
	[[nodiscard]] Step objective_getNeedDelayRemaining(const ActorIndex& index, NeedType objectiveTypeId) const;
	// CanReserve.
	void canReserve_clearAll(const ActorIndex& index);
	void canReserve_setFaction(const ActorIndex& index, FactionId faction);
	// Default dishonor callback is canNotCompleteCurrentObjective.
	void canReserve_reserveLocation(const ActorIndex& index, const BlockIndex& block, std::unique_ptr<DishonorCallback> callback = nullptr);
	void canReserve_reserveItem(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity, std::unique_ptr<DishonorCallback> callback = nullptr);
	[[nodiscard]] bool canReserve_tryToReserveLocation(const ActorIndex& index, const BlockIndex& block, std::unique_ptr<DishonorCallback> callback = nullptr);
	[[nodiscard]] bool canReserve_tryToReserveItem(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity, std::unique_ptr<DishonorCallback> callback = nullptr);
	[[nodiscard]] bool canReserve_hasReservationWith(const ActorIndex& index, Reservable& reservable) const;
	[[nodiscard]] bool canReserve_canReserveLocation(const ActorIndex& index, const BlockIndex& block, const Facing& facing) const;
private:
	[[nodiscard]] CanReserve& canReserve_get(const ActorIndex& index);
	// Project.
public:
	[[nodiscard]] bool project_exists(const ActorIndex& index) const { return m_project[index] != nullptr; }
	[[nodiscard]] Project* project_get(const ActorIndex& index) const { assert(m_project[index] != nullptr); return m_project[index]; }
	void project_set(const ActorIndex& index, Project& project) { assert(m_project[index] == nullptr); m_project[index] = &project; }
	void project_unset(const ActorIndex& index) { assert(m_project[index] != nullptr); m_project[index] = nullptr; }
	void project_maybeUnset(const ActorIndex& index) { m_project[index] = nullptr; }
	// -Equipment.
	void equipment_add(const ActorIndex& index, const ItemIndex& item);
	void equipment_addGeneric(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materalType, const Quantity& quantity);
	void equipment_remove(const ActorIndex& index, const ItemIndex& item);
	void equipment_removeGeneric(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materalType, const Quantity& quantity);
	[[nodiscard]] bool equipment_canEquipCurrently(const ActorIndex& index, const ItemIndex& item) const;
	[[nodiscard]] bool equipment_containsItem(const ActorIndex& index, const ItemIndex& item) const;
	[[nodiscard]] bool equipment_containsItemType(const ActorIndex& index, const ItemTypeId& type) const;
	[[nodiscard]] Mass equipment_getMass(const ActorIndex& index) const;
	[[nodiscard]] ItemIndex equipment_getWeaponToAttackAtRange(const ActorIndex& index, const DistanceInBlocksFractional& range) const;
	[[nodiscard]] ItemIndex equipment_getAmmoForRangedWeapon(const ActorIndex& index, const ItemIndex& weapon) const;
	[[nodiscard]] const auto& equipment_getAll(const ActorIndex& index) const { return m_equipmentSet[index]->getAll(); }
	// -Uniform.
	void uniform_set(const ActorIndex& index, Uniform& uniform);
	void uniform_unset(const ActorIndex& index);
	[[nodiscard]] bool uniform_exists(const ActorIndex& index) const;
	[[nodiscard]] Uniform& uniform_get(const ActorIndex& index);
	[[nodiscard]] const Uniform& uniform_get(const ActorIndex& index) const;
	// Sleep.
	void sleep_do(const ActorIndex& actor);
	void sleep_wakeUp(const ActorIndex& actor);
	void sleep_wakeUpEarly(const ActorIndex& actor);
	void sleep_setSpot(const ActorIndex& index, const BlockIndex& location);
	void sleep_makeTired(const ActorIndex& index);
	void sleep_clearObjective(const ActorIndex& index);
	[[nodiscard]] BlockIndex sleep_getSpot(const ActorIndex& index) const;
	[[nodiscard]] bool sleep_isAwake(const ActorIndex& index) const;
	[[nodiscard]] Percent sleep_getPercentDoneSleeping(const ActorIndex& index) const;
	[[nodiscard]] Percent sleep_getPercentTired(const ActorIndex& index) const;
	// For testing.
	[[nodiscard]] bool sleep_hasTiredEvent(const ActorIndex& index) const;
	// Drink.
	void drink_do(const ActorIndex& index, const CollisionVolume& volume);
	void drink_setNeedsFluid(const ActorIndex& index);
	[[nodiscard]] CollisionVolume drink_getVolumeOfFluidRequested(const ActorIndex& index) const;
	[[nodiscard]] bool drink_isThirsty(const ActorIndex& index) const;
	[[nodiscard]] FluidTypeId drink_getFluidType(const ActorIndex& index) const;
	[[nodiscard]] Percent drink_getPercentDead(const ActorIndex& index) const;
	[[nodiscard]] Step drink_getStepsTillDead(const ActorIndex& index) const;
	// For Testing.
	[[nodiscard]] bool drink_hasThristEvent(const ActorIndex& index) const;
	// Eat.
	void eat_do(const ActorIndex& index, const Mass& mass);
	void eat_setIsHungry(const ActorIndex& index);
	[[nodiscard]] bool eat_isHungry(const ActorIndex& index) const;
	[[nodiscard]] bool eat_canEatActor(const ActorIndex& index, const ActorIndex& other) const;
	[[nodiscard]] bool eat_canEatItem(const ActorIndex& index, const ItemIndex& item) const;
	[[nodiscard]] bool eat_canEatPlant(const ActorIndex& index, const PlantIndex& plant) const;
	[[nodiscard]] Percent eat_getPercentStarved(const ActorIndex& index) const;
	[[nodiscard]] BlockIndex eat_getAdjacentBlockWithTheMostDesiredFood(const ActorIndex& index) const;
	// For Testing.
	[[nodiscard]] Mass eat_getMassFoodRequested(const ActorIndex& index) const;
	[[nodiscard]] uint32_t eat_getDesireToEatSomethingAt(const ActorIndex& index, const BlockIndex& block) const;
	[[nodiscard]] uint32_t eat_getMinimumAcceptableDesire(const ActorIndex& index) const;
	[[nodiscard]] bool eat_hasObjective(const ActorIndex& index) const;
	[[nodiscard]] Step eat_getHungerEventStep(const ActorIndex& index) const;
	[[nodiscard]] bool eat_hasHungerEvent(const ActorIndex& index) const;
	// Temperature.
	void temperature_onChange(const ActorIndex& index);
	[[nodiscard]] bool temperature_isSafe(const ActorIndex& index, const Temperature& temperature) const;
	[[nodiscard]] bool temperature_isSafeAtCurrentLocation(const ActorIndex& index) const;
	// Attributes.
	void attributes_onUpdateGrowthPercent(const ActorIndex& index);
	void addStrengthBonusOrPenalty(const ActorIndex& index, const AttributeLevelBonusOrPenalty& bonusOrPenalty);
	void addStrengthModifier(const ActorIndex& index, float modifer);
	void onStrengthChanged(const ActorIndex& index);
	void updateStrength(const ActorIndex& index);
	void addDextarityBonusOrPenalty(const ActorIndex& index, const AttributeLevelBonusOrPenalty& bonusOrPenalty);
	void addDextarityModifier(const ActorIndex& index, float modifer);
	void onDextarityChanged(const ActorIndex& index);
	void updateDextarity(const ActorIndex& index);
	void addAgilityBonusOrPenalty(const ActorIndex& index, const AttributeLevelBonusOrPenalty& bonusOrPenalty);
	void addAgilityModifier(const ActorIndex& index, float modifer);
	void onAgilityChanged(const ActorIndex& index);
	void updateAgility(const ActorIndex& index);
	void addIntrinsicMassBonusOrPenalty(const ActorIndex& index, uint32_t bonusOrPenalty);
	void addIntrinsicMassModifier(const ActorIndex& index, float modifer);
	void onIntrinsicMassChanged(const ActorIndex& index);
	void updateIntrinsicMass(const ActorIndex& index);
	[[nodiscard]] AttributeLevel getStrength(const ActorIndex& index) const;
	[[nodiscard]] AttributeLevelBonusOrPenalty getStrengthBonusOrPenalty(const ActorIndex& index) const;
	[[nodiscard]] float getStrengthModifier(const ActorIndex& index) const;
	[[nodiscard]] AttributeLevel getDextarity(const ActorIndex& index) const;
	[[nodiscard]] AttributeLevelBonusOrPenalty getDextarityBonusOrPenalty(const ActorIndex& index) const;
	[[nodiscard]] float getDextarityModifier(const ActorIndex& index) const;
	[[nodiscard]] AttributeLevel getAgility(const ActorIndex& index) const;
	[[nodiscard]] AttributeLevelBonusOrPenalty getAgilityBonusOrPenalty(const ActorIndex& index) const;
	[[nodiscard]] float getAgilityModifier(const ActorIndex& index) const;
	[[nodiscard]] Mass getIntrinsicMass(const ActorIndex& index) const;
	[[nodiscard]] int32_t getIntrinsicMassBonusOrPenalty(const ActorIndex& index) const;
	[[nodiscard]] float getIntrinsicMassModifier(const ActorIndex& index) const;
	// Skills.
	[[nodiscard]] SkillLevel skill_getLevel(const ActorIndex& index, const SkillTypeId& skillType) const;
	// Growth.
	void grow_maybeStart(const ActorIndex& index);
	void grow_stop(const ActorIndex& index);
	void grow_updateGrowingStatus(const ActorIndex& index);
	void grow_setPercent(const ActorIndex& index, const Percent& percentGrown);
	[[nodiscard]] bool grow_isGrowing(const ActorIndex& index) const;
	[[nodiscard]] Percent grow_getPercent(const ActorIndex& index) const;
	// For Line leader.
	[[nodiscard]] BlockIndices lineLead_getPath(const ActorIndex& index) const;
	[[nodiscard]] OccupiedBlocksForHasShape lineLead_getOccupiedBlocks(const ActorIndex& index) const;
	[[nodiscard]] bool lineLead_pathEmpty(const ActorIndex& index) const;
	[[nodiscard]] ShapeId lineLead_getLargestShape(const ActorIndex& index) const;
	[[nodiscard]] MoveTypeId lineLead_getMoveType(const ActorIndex& index) const;
	void lineLead_clearPath(const ActorIndex& index);
	void lineLead_appendToPath(const ActorIndex& index, const BlockIndex& block);
	void lineLead_pushFront(const ActorIndex& index, const BlockIndex& block);
	void lineLead_popBackUnlessOccupiedByFollower(const ActorIndex& index);
	// For testing.
	[[nodiscard]] bool grow_getEventExists(const ActorIndex& index) const;
	[[nodiscard]] Percent grow_getEventPercent(const ActorIndex& index) const;
	[[nodiscard]] Step grow_getEventStep(const ActorIndex& index) const;
	[[nodiscard]] bool grow_eventIsPaused(const ActorIndex& index) const;
	// For UI.
	[[nodiscard]] ActorOrItemIndex canPickUp_getCarrying(const ActorIndex& index) const;
	
	// For debugging.
	void log(const ActorIndex& index) const;
	void satisfyNeeds(const ActorIndex& index);
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
	MoveEvent(const Step& delay, Area& area, const ActorIndex& actor, const Step start = Step::null());
	MoveEvent(Simulation& simulation, const Json& data);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	void onMoveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_actor == oldIndex.toActor()); m_actor = ActorIndex::cast(newIndex); }
	void updateIndex(const ActorIndex& oldIndex, const ActorIndex& newIndex)
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
	AttackCoolDownEvent(Area& area, const ActorIndex& index, const Step& duration, const Step start = Step::null());
	AttackCoolDownEvent(Simulation& simulation, const Json& data);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	void onMoveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_actor == oldIndex.toActor()); m_actor = ActorIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class GetIntoAttackPositionPathRequest final : public PathRequest
{
	ActorIndex m_actor;
	ActorReference m_target;
	DistanceInBlocksFractional m_attackRangeSquared = DistanceInBlocksFractional::null();
public:
	GetIntoAttackPositionPathRequest(Area& area, const ActorIndex& a, const ActorIndex& t, const DistanceInBlocksFractional& ar);
	GetIntoAttackPositionPathRequest(const Json& data, Area& area);
	void callback(Area& area, const FindPathResult&);
	void onMoveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_actor == oldIndex.toActor()); m_actor = ActorIndex::cast(newIndex); }
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
