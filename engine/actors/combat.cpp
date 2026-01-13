#include "actors.h"

#include "../area/area.h"
#include "../definitions/attackType.h"
#include "../definitions/itemType.h"
#include "../simulation/simulation.h"
#include "../simulation/hasActors.h"
#include "../numericTypes/types.h"
#include "../util.h"
#include "../definitions/weaponType.h"
#include "../config/config.h"
#include "../random.h"
#include "../items/items.h"
#include "../reference.h"
#include "../path/terrainFacade.hpp"
#include "../objectives/flee.h"
#include "skill.h"

#include <cstdint>
#include <sys/types.h>
#include <utility>
#include <algorithm>
// CanFight.
void Actors::combat_attackMeleeRange(const ActorIndex& index, const ActorIndex& target)
{
	assert(!m_coolDownEvent.exists(index));
	CombatScore attackerCombatScore = combat_getCurrentMeleeCombatScore(index);
	CombatScore targetCombatScore = combat_getCurrentMeleeCombatScore(target);
	Step coolDownDuration;
	if(attackerCombatScore > targetCombatScore)
	{
		// Attack hits.
		const Attack& attack = combat_getAttackForCombatScoreDifference(index, attackerCombatScore - targetCombatScore);
		Force attackForce = Force::create(AttackType::getBaseForce(attack.attackType).get() + (m_strength[index].get() * Config::unitsOfAttackForcePerUnitOfStrength));
		// TODO: Higher skill selects more important body parts to hit.
		BodyPart& bodyPart = m_body[target]->pickABodyPartByVolume(m_area.m_simulation);
		Hit hit(AttackType::getArea(attack.attackType), attackForce, attack.materialType, AttackType::getWoundType(attack.attackType));
		takeHit(target, hit, bodyPart);
		// If there is a weapon being used take the cool down from it, otherwise use onMiss cool down.
		if(attack.item.exists())
			coolDownDuration = combat_getCoolDown(index, attack);
		else
			coolDownDuration = m_onMissCoolDownMelee[index];
	}
	else
		coolDownDuration = m_onMissCoolDownMelee[index];
	m_coolDownEvent.schedule(index, m_area, index, coolDownDuration * m_coolDownDurationModifier[index]);
	//TODO: Skill growth.
}
Step Actors::combat_attackMeleeRangeNonLethal(const ActorIndex& index, const ActorIndex& target)
{
	assert(!m_coolDownEvent.exists(index));
	//AcceptableDeficiency: Unlike lethal combat, non lethal does not have adjacent bonus.
	CombatScore attackerCombatScore = m_combatScoreNonLethal[index];
	CombatScore targetCombatScore = m_combatScoreNonLethal[target];
	Step coolDownDuration;
	if(attackerCombatScore > targetCombatScore)
	{
		// Attack hits.
		const Attack& attack = combat_getNonLethalAttackForCombatScoreDifference(index, attackerCombatScore - targetCombatScore);
		Force attackForce = Force::create(AttackType::getBaseForce(attack.attackType).get() + (m_strength[index].get() * Config::unitsOfAttackForcePerUnitOfStrength));
		// TODO: Higher skill selects more important body parts to hit.
		BodyPart& bodyPart = m_body[target]->pickABodyPartByVolume(m_area.m_simulation);
		Hit hit(AttackType::getArea(attack.attackType), attackForce, attack.materialType, AttackType::getWoundType(attack.attackType));
		takeHit(target, hit, bodyPart);
		// If there is a weapon being used take the cool down from it, otherwise use onMiss cool down.
		if(attack.item.exists())
			coolDownDuration = combat_getCoolDown(index, attack);
		else
			coolDownDuration = m_onMissCoolDownMelee[index];
	}
	else
		coolDownDuration = m_onMissCoolDownMelee[index];
	// No cooldown scheduled here: non lethal fight cooldowns are managed by the ConfrontationObjective in order to check if the fighters still want to continue.
	//TODO: Skill growth.
	return coolDownDuration;
}
void Actors::combat_attackLongRange(const ActorIndex& index, const ActorIndex& target, const ItemIndex& weapon, const ItemIndex& ammo)
{
	//TODO: unarmed ranged attack?
	AttackTypeId attackType = combat_getRangedAttackType(index, weapon);
	Items& items = m_area.getItems();;
	Step coolDown = AttackType::getCoolDown(attackType);
	if(coolDown == 0)
		coolDown = ItemType::getAttackCoolDownBase(items.getItemType(weapon));
	Attack attack(attackType, items.getMaterialType(ammo), weapon);
	if(combat_doesProjectileHit(index, attack, target))
	{
		// Attack hits.
		// TODO: Higher skill selects more important body parts to hit.
		BodyPart& bodyPart = m_body[target]->pickABodyPartByVolume(m_area.m_simulation);
		Hit hit(AttackType::getArea(attackType), AttackType::getBaseForce(attackType), attack.materialType, AttackType::getWoundType(attack.attackType));
		takeHit(target, hit, bodyPart);
	}
	m_coolDownEvent.schedule(index, m_area, index, coolDown);
}
CombatScore Actors::combat_getCurrentMeleeCombatScore(const ActorIndex& index)
{
	FactionId faction = getFaction(index);
	int pointsContainingNonAllies = 0;
	// Apply bonuses and penalties based on relative locations.
	CombatScore output = m_combatScore[index];
	for(const ActorIndex& adjacent : getAdjacentActors(index))
	{
		CombatScore highestAllyCombatScore = CombatScore::create(0);
		bool nonAllyFound = false;
		FactionId otherFaction = getFaction(adjacent);
		if(otherFaction.exists() && (otherFaction == faction || m_area.m_simulation.m_hasFactions.isAlly(otherFaction, faction)))
		{
			if(m_combatScore[adjacent] > highestAllyCombatScore)
				highestAllyCombatScore = m_combatScore[adjacent];
		}
		else
				nonAllyFound = true;
		if(nonAllyFound)
		{
			if(highestAllyCombatScore != 0)
				continue;
			else
				++pointsContainingNonAllies;
		}
		else
			output += highestAllyCombatScore;
	}
	if(pointsContainingNonAllies > 1)
	{
		float flankingModifier = 1 + (pointsContainingNonAllies - 1) * Config::flankingModifier;
		output /= flankingModifier;
	}
	return output;
}
void Actors::combat_coolDownCompleted(const ActorIndex& index)
{
	if(m_target[index].empty())
		return;
	Space& space = m_area.getSpace();
	//TODO: Move line of sight check to threaded task?
	const ActorIndex& target = m_target[index];
	Point3D location = m_location[index];
	Point3D targetLocation = m_location[target];
	if(location.distanceToFractional(targetLocation) <= m_maxMeleeRange[index] && space.hasLineOfSightTo(location, targetLocation))
		combat_attackMeleeRange(index, m_target[index]);
}
void Actors::combat_update(const ActorIndex& index)
{
	if(soldier_is(index))
		soldier_removeFromMaliceMap(index);
	m_combatScore[index] = CombatScore::create(0);
	m_maxMeleeRange[index] = DistanceFractional::create(0.f);
	m_maxRange[index] = DistanceFractional::create(0.f);
	// Collect attacks and combat scores from body and equipment.
	m_meleeAttackTable[index].clear();
	m_meleeAttackTableNonLethal[index].clear();
	Body& body = *m_body[index].get();
	for(Attack& attack : body.getMeleeAttacks())
	{
		const CombatScore score  = combat_getCombatScoreForAttack(index, attack);
		m_meleeAttackTable[index].add(score, attack);
		if(attack.isNonLethalAgainst(m_species[index]))
			m_meleeAttackTableNonLethal[index].add(score, attack);
	}
	for(Attack& attack : m_equipmentSet[index]->getMeleeAttacks(m_area))
	{
		const CombatScore score  = combat_getCombatScoreForAttack(index, attack);
		m_meleeAttackTable[index].add(score, attack);
		if(attack.isNonLethalAgainst(m_species[index]))
			m_meleeAttackTableNonLethal[index].add(score, attack);
	}
	const auto& [totalScore, maxRange] = m_meleeAttackTable[index].build();
	m_combatScore[index] = totalScore;
	m_maxMeleeRange[index] = maxRange;
	const auto& [totalScoreNonLethal, maxRangeNonLethal] = m_meleeAttackTable[index].build();
	m_combatScoreNonLethal[index] = totalScoreNonLethal;
	m_maxMeleeRangeNonLethal[index] = maxRangeNonLethal;
	// Base stats give combat score.
	const CombatScore attributeBonus = attributes_getCombatScore(index);
	m_combatScore[index] += attributeBonus;
	m_combatScoreNonLethal[index] += attributeBonus;
	// Reduce for impairment.
	const Percent imparment = body.getImpairManipulationPercent();
	m_combatScore[index] = CombatScore::create(util::scaleByInversePercent(m_combatScore[index].get(), imparment));
	m_combatScoreNonLethal[index] = CombatScore::create(util::scaleByInversePercent(m_combatScoreNonLethal[index].get(), imparment));
	// Update cool down duration.
	// TODO: Manipulation impairment should apply to cooldown as well?
	// TODO: Pain causes impairment.
	m_coolDownDurationModifier[index] = std::max(1.f, (float)m_equipmentSet[index]->getMass().get() / (float)m_unencomberedCarryMass[index].get() );
	if(m_dextarity[index] > Config::attackCoolDownDurationBaseDextarity)
	{
		float reduction = (float)(m_dextarity[index].get() - Config::attackCoolDownDurationBaseDextarity) * Config::fractionAttackCoolDownReductionPerPointOfDextarity;
		m_coolDownDurationModifier[index] = std::max(m_coolDownDurationModifier[index] - reduction, Config::minimumAttackCoolDownModifier);
	}
	// Find the on miss cool down.
	Step baseOnMissCoolDownDuration = m_equipmentSet[index]->hasWeapons() ?
	    	m_equipmentSet[index]->getLongestMeleeWeaponCoolDown(m_area) :
		Config::attackCoolDownDurationBaseSteps;
	m_onMissCoolDownMelee[index] = std::max(Step::create(1), Step::create((float)baseOnMissCoolDownDuration.get() * m_coolDownDurationModifier[index]));
	Items& items = m_area.getItems();
	//Find max range.
	m_maxRange[index] = m_maxMeleeRange[index];
	for(ItemReference item : m_equipmentSet[index]->getRangedWeapons())
	{
		ItemIndex itemIndex = item.getIndex(items.m_referenceData);
		AttackTypeId attackType = ItemType::getRangedAttackType(items.getItemType(itemIndex));
		auto range = AttackType::getRange(attackType);
		if(range > m_maxRange[index])
			m_maxRange[index] = range;
	}
	if(soldier_is(index))
		soldier_recordInMaliceMap(index);
}
//TODO: Grasps cannot be used for both armed and unarmed attacks at the same time?
CombatScore Actors::combat_getCombatScoreForAttack(const ActorIndex& index, const Attack& attack) const
{
	CombatScore output = AttackType::getCombatScore(attack.attackType);
	SkillTypeId skill = attack.item == ItemIndex::null() ?
		SkillType::byName("unarmed") :
		AttackType::getSkillType(attack.attackType);
	SkillLevel skillValue = m_skillSet[index].get(skill);
	output += (skillValue * Config::attackSkillCombatModifier).get();
	Items& items = m_area.getItems();
	Quality quality = attack.item == ItemIndex::null() ? Config::averageItemQuality : items.getQuality(attack.item);
	output *= combat_getQualityModifier(index, quality);
	Percent percentItemWear = attack.item == ItemIndex::null() ? Percent::create(0) : items.getWear(attack.item);
	output -= (percentItemWear * Config::itemWearCombatModifier).get();
	return output;
}
const Attack& Actors::combat_getAttackForCombatScoreDifference(const ActorIndex& index, const CombatScore& scoreDifference) const
{
	return m_meleeAttackTable[index].getForCombatScoreDifference(scoreDifference);
}
const Attack& Actors::combat_getNonLethalAttackForCombatScoreDifference(const ActorIndex& index, CombatScore scoreDifference) const
{
	return m_meleeAttackTableNonLethal[index].getForCombatScoreDifference(scoreDifference);
}
void Actors::combat_setTarget(const ActorIndex& index, const ActorIndex& actor)
{
	m_target[index] = actor;
	combat_recordTargetedBy(actor, index);
	move_pathRequestRecord(index, std::make_unique<GetIntoAttackPositionPathRequest>(m_area, index, actor, m_maxRange[index]));
}
void Actors::combat_recordTargetedBy(const ActorIndex& index, const ActorIndex& actor)
{
	assert(m_target[actor] == index);
	assert(!m_targetedBy[index].contains(actor));
	m_targetedBy[index].insert(actor);
}
void Actors::combat_removeTargetedBy(const ActorIndex& index, const ActorIndex& actor)
{
	assert(m_targetedBy[index].contains(actor));
	m_targetedBy[index].erase(actor);
}
void Actors::combat_onMoveFrom(const ActorIndex& index, const Point3D& previous)
{
	// Notify all targeting actors of move so they may reroute.
	for(const ActorIndex& actor : m_targetedBy[index])
		combat_onTargetMoved(actor);
	// Give all directly adjacent enemies a free hit against this actor.
	Space& space = m_area.getSpace();
	FactionId faction = m_faction[index];
	for(const Point3D& point : space.getDirectlyAdjacent(previous))
		for(const ActorIndex& adjacent : space.actor_getAll(point))
		{
			FactionId otherFaction = m_faction[adjacent];
			if(m_area.m_simulation.m_hasFactions.isEnemy(faction, otherFaction))
				combat_freeHit(adjacent, index);
		}
}
void Actors::combat_noLongerTargetable(const ActorIndex& index)
{
	for(const ActorIndex& actor : m_targetedBy[index])
	{
		assert(m_targetedBy[actor].contains(index));
		combat_targetNoLongerTargetable(index);
		m_hasObjectives[actor]->subobjectiveComplete(m_area);
	}
	m_targetedBy[index].clear();
}
void Actors::combat_onDeath(const ActorIndex& index)
{
	combat_noLongerTargetable(index);
	if(soldier_is(index))
		soldier_removeFromMaliceMap(index);
}
void Actors::combat_onLeaveArea(const ActorIndex& index)
{
	combat_noLongerTargetable(index);
}
void Actors::combat_targetNoLongerTargetable(const ActorIndex& index)
{
	assert(m_target[index].exists());
	m_target[index].clear();
	m_hasObjectives[index]->subobjectiveComplete(m_area);
}
void Actors::combat_onTargetMoved(const ActorIndex& index)
{
	if(!m_path[index].empty())
		combat_getIntoRangeAndLineOfSightOfActor(index, m_target[index], combat_getMaxRange(index));
}
void Actors::combat_freeHit(const ActorIndex& index, const ActorIndex& actor)
{
	m_coolDownEvent.maybeUnschedule(index);
	combat_attackMeleeRange(index, actor);
}
void Actors::combat_getIntoRangeAndLineOfSightOfActor(const ActorIndex& index, const ActorIndex& target, const DistanceFractional& range)
{
	move_pathRequestRecord(index, std::make_unique<GetIntoAttackPositionPathRequest>(m_area, index, target, range));
}
void Actors::combat_flee(const ActorIndex &index)
{
	objective_addNeed(index, std::make_unique<FleeObjective>());
}
bool Actors::combat_isFleeing(const ActorIndex &index)
{
	// TODO: Create an objective type id.
	static ObjectiveTypeId fleeObjectiveTypeId = ObjectiveType::getByName("flee").getId();
	return objective_getCurrentTypeId(index) == fleeObjectiveTypeId;
}
bool Actors::combat_isOnCoolDown(const ActorIndex& index) const { return m_coolDownEvent.exists(index); }
bool Actors::combat_inRange(const ActorIndex& index, const ActorIndex& target) const
{
	return m_location[index].distanceToFractional(m_location[target]) <= m_maxRange[index];
}
Percent Actors::combat_projectileHitPercent(const ActorIndex& index, const Attack& attack, const ActorIndex& target) const
{
	Percent chance = Percent::create(100 - std::pow(distanceToActorFractional(index, target).get(), Config::projectileHitChanceFallsOffWithRangeExponent));
	chance += m_skillSet[index].get(AttackType::getSkillType(attack.attackType)).get() * Config::projectileHitPercentPerSkillPoint;
	chance += (getVolume(target) * Config::projectileHitPercentPerUnitVolume).get();
	chance += m_dextarity[index].get() * Config::projectileHitPercentPerPointDextarity;
	if(attack.item.exists())
	{
		Items& items = m_area.getItems();
		chance += ((items.getQuality(attack.item) - Config::averageItemQuality) * Config::projectileHitPercentPerPointQuality).get();
		chance -= items.getWear(attack.item) * Config::projectileHitPercentPerPointWear;
	}
	chance -= combat_getCombatScore(index).get() * Config::projectileHitPercentPerPointTargetCombatScore;
	chance += AttackType::getCombatScore(attack.attackType).get() * Config::projectileHitPercentPerPointAttackTypeCombatScore;
	return chance;
}
bool Actors::combat_doesProjectileHit(const ActorIndex& index, Attack& attack, const ActorIndex& target) const
{
	Percent chance = combat_projectileHitPercent(index, attack, target);
	return m_area.m_simulation.m_random.percentChance(chance);
}
float Actors::combat_getQualityModifier(const ActorIndex&, const Quality& quality) const
{
	int adjusted = (int)quality.get() - (int)Config::averageItemQuality.get();
	return 1.f + (adjusted * Config::itemQualityCombatModifier);
}
bool Actors::combat_positionIsValid(const ActorIndex& index, const Point3D& point, const DistanceFractional& attackRangeSquared) const
{
	if(getOccupied(m_target[index]).contains(point))
		return true;
	Space& space = m_area.getSpace();
	Point3D targetLocation = m_location[m_target[index]];
	if(point.squareOfDistanceIsGreaterThen(targetLocation, attackRangeSquared))
		return false;
	return space.hasLineOfSightTo(point, targetLocation);
}
AttackTypeId Actors::combat_getRangedAttackType(const ActorIndex&, const ItemIndex& weapon) const
{
	// Each ranged weapon has only one ranged attack type to pick.
	ItemTypeId itemType = m_area.getItems().getItemType(weapon);
	assert(ItemType::getIsWeapon(itemType));
	for(AttackTypeId attackType : ItemType::getAttackTypes(itemType))
		if(AttackType::getProjectile(attackType))
			return attackType;
	std::unreachable();
	return ItemType::getAttackTypes(itemType).front();
}
CuboidSet Actors::combat_makeMalicePoints(const ActorIndex& index) const
{
	const Point3D& location = m_location[index];
	Cuboid zone = Cuboid::create(location, location);
	zone.inflate(Distance::create((float)m_speedActual[index].get() * Config::fractionOfMoveSpeedToMakeDistanceHuristicForCanMoveToSoon));
	// Get points in zone which are enterable by this move type.
	CuboidSet set = m_area.m_hasTerrainFacades.getForMoveType(m_moveType[index]).getEnterableIntersection(zone);
	// Trim parts of set which could be entered by this move type somewhere but are not directly connected to location.
	set = set.adjacentRecursive(location);
	// Expand set with weapon range.
	set.inflate(Distance::create(combat_getMaxRange(index).get()));
	// Remove points which block line of sight.
	// Armored glass does not exist so any line of sight is a line of attack.
	m_area.m_opacityFacade.removeFromCuboidSet(set);
	// Trim again.
	set = set.adjacentRecursive(location);
	return set;
}
Step Actors::combat_getCoolDown(const ActorIndex& index, const Attack& attack) const
{
	Step output;
	// If there is a weapon being used take the cool down from it, otherwise use onMiss cool down.
	if(attack.item.exists())
	{
		Items& items = m_area.getItems();
		output = AttackType::getCoolDown(attack.attackType);
		if(output.empty())
			output = ItemType::getAttackCoolDownBase(items.getItemType(attack.item));
		output = std::max(Step::create(1), Step::create(output.get() * m_coolDownDurationModifier[index]));
	}
	else
		// m_onMissCollDownMelee already has m_coolDownDurationModifier applied to it.
		// TODO: Rename to defaultCoolDownMelee.
		output = m_onMissCoolDownMelee[index];
	return output;
}
AttackCoolDownEvent::AttackCoolDownEvent(Area& area, const ActorIndex& actor, const Step& duration, const Step start) :
	ScheduledEvent(area.m_simulation, duration, start), m_actor(actor) { }

GetIntoAttackPositionPathRequest::GetIntoAttackPositionPathRequest(Area& area, const ActorIndex& actorIndex, const ActorIndex& targetIndex, const DistanceFractional& ar) :
	attackRangeSquared(ar * ar)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Distance::max();
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = true;
	adjacent = false;
	target.setIndex(targetIndex, area.getActors().m_referenceData);
}
GetIntoAttackPositionPathRequest::GetIntoAttackPositionPathRequest(const Json& data, Area& area) :
	PathRequestDepthFirst(data, area),
	target(data["target"], area.getActors().m_referenceData),
	attackRangeSquared(data["attackRangeSquared"].get<DistanceFractional>())
{ }
FindPathResult GetIntoAttackPositionPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo)
{
	Actors& actors = area.getActors();
	auto destinationCondition = [&actors, this](const Point3D& location, const Facing4&) ->std::pair<bool, Point3D>
	{
		if(actors.combat_positionIsValid(actor.getIndex(actors.m_referenceData), location, attackRangeSquared))
			return std::make_pair(true, location);
		return std::make_pair(false, Point3D::null());
	};
	// TODO: Range attack actors should use a different path priority condition to avoid getting too close.
	constexpr bool useAnyPoint = false;
	const Point3D& targetLocation = actors.getLocation(target.getIndex(actors.m_referenceData));
	return terrainFacade.findPathToConditionDepthFirst<decltype(destinationCondition), useAnyPoint, false>(destinationCondition, memo, start, facing, shape, targetLocation, detour, faction, maxRange);
}
void GetIntoAttackPositionPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	const ActorIndex& actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty())
	{
		if(result.useCurrentPosition)
		{
			if(!actors.combat_isOnCoolDown(actorIndex))
			{
				const ActorIndex& targetIndex = target.getIndex(actors.m_referenceData);
				DistanceFractional range = actors.distanceToActorFractional(actorIndex, targetIndex);
				if(range <= actors.combat_getMaxMeleeRange(actorIndex))
					// Melee range attack.
					actors.combat_attackMeleeRange(actorIndex, targetIndex);
				else
				{
					// Long range attack.
					// TODO: Unarmed long range attack, needs to prodive the material type somehow, maybe in body part?
					ItemIndex weapon = actors.equipment_getWeaponToAttackAtRange(actorIndex, range);
					ItemIndex ammo = actors.equipment_getAmmoForRangedWeapon(actorIndex, weapon);
					actors.combat_attackLongRange(actorIndex, targetIndex, weapon, ammo);
				}
			}
		}
		else
			actors.objective_canNotCompleteSubobjective(actorIndex);
	}
	else
		actors.move_setPath(actorIndex, result.path);
}
Json GetIntoAttackPositionPathRequest::toJson() const
{
	Json output = static_cast<const PathRequestDepthFirst&>(*this);
	output.update({
		{"target", target},
		{"attackRangeSquared", attackRangeSquared},
		{"type", "attack"}
	});
	return output;
}
