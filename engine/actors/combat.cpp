#include "actors.h"

#include "../area.h"
#include "../attackType.h"
#include "../itemType.h"
#include "../simulation.h"
#include "../simulation/hasActors.h"
#include "../skill.h"
#include "../types.h"
#include "../util.h"
#include "../weaponType.h"
#include "../config.h"
#include "../random.h"
#include "../items/items.h"
#include "reference.h"

#include <cstdint>
#include <sys/types.h>
#include <utility>
#include <algorithm>
// CanFight.
void Actors::combat_attackMeleeRange(ActorIndex index, ActorIndex target)
{
	assert(!m_coolDownEvent.exists(index));
	uint32_t attackerCombatScore = combat_getCurrentMeleeCombatScore(index);
	uint32_t targetCombatScore = combat_getCurrentMeleeCombatScore(target);
	Step coolDownDuration = m_onMissCoolDownMelee.at(index);
	if(attackerCombatScore > targetCombatScore)
	{
		// Attack hits.
		const Attack& attack = combat_getAttackForCombatScoreDifference(index, attackerCombatScore - targetCombatScore);
		Force attackForce = attack.attackType->baseForce + (m_attributes.at(index)->getStrength() * Config::unitsOfAttackForcePerUnitOfStrength);
		// TODO: Higher skill selects more important body parts to hit.
		BodyPart& bodyPart = m_body.at(target)->pickABodyPartByVolume(m_area.m_simulation);
		Hit hit(attack.attackType->area, attackForce, *attack.materialType, attack.attackType->woundType);
		takeHit(target, hit, bodyPart);
		// If there is a weapon being used take the cool down from it, otherwise use onMiss cool down.
		if(attack.item != ITEM_INDEX_MAX)
		{
			Items& items = m_area.getItems();
			coolDownDuration = attack.attackType->coolDown == 0 ? items.getItemType(attack.item).weaponData->coolDown : attack.attackType->coolDown;
			coolDownDuration *= m_coolDownDurationModifier.at(index);
		}
	}
	m_coolDownEvent.schedule(index, m_area, index, coolDownDuration);
	//TODO: Skill growth.
}
void Actors::combat_attackLongRange(ActorIndex index, ActorIndex target, ItemIndex weapon, ItemIndex ammo)
{
	//TODO: unarmed ranged attack?
	const AttackType& attackType = combat_getRangedAttackType(index, weapon);
	Items& items = m_area.getItems();;
	Step coolDown = attackType.coolDown == 0 ? items.getItemType(weapon).weaponData->coolDown : attackType.coolDown;
	Attack attack(&attackType, &items.getMaterialType(ammo), weapon);
	if(combat_doesProjectileHit(index, attack, target))
	{
		// Attack hits.
		// TODO: Higher skill selects more important body parts to hit.
		BodyPart& bodyPart = m_body.at(target)->pickABodyPartByVolume(m_area.m_simulation);
		Hit hit(attackType.area, attackType.baseForce, *attack.materialType, attack.attackType->woundType);
		takeHit(target, hit, bodyPart);
	}
	m_coolDownEvent.schedule(index, m_area, index, coolDown);
}
uint32_t Actors::combat_getCurrentMeleeCombatScore(ActorIndex index)
{
	FactionId faction = getFactionId(index);
	uint32_t blocksContainingNonAllies = 0;
	// Apply bonuses and penalties based on relative locations.
	uint32_t output = m_combatScore.at(index);
	for(ActorIndex adjacent : getAdjacentActors(index))
	{
		uint32_t highestAllyCombatScore = 0;
		bool nonAllyFound = false;
		FactionId otherFaction = getFactionId(adjacent);
		if(otherFaction && (otherFaction == faction || m_area.m_simulation.m_hasFactions.isAlly(otherFaction, faction)))
		{
			if(m_combatScore.at(adjacent) > highestAllyCombatScore)
				highestAllyCombatScore = m_combatScore.at(adjacent);
		}
		else
				nonAllyFound = true;
		if(nonAllyFound)
		{
			if(highestAllyCombatScore != 0)
				continue;
			else
				++blocksContainingNonAllies;
		}
		else
			output += highestAllyCombatScore;
	}
	if(blocksContainingNonAllies > 1)
	{
		float flankingModifier =  1 + (blocksContainingNonAllies - 1) * Config::flankingModifier;
		output /= flankingModifier;
	}
	return output;
}
void Actors::combat_coolDownCompleted(ActorIndex index)
{
	if(m_target.at(index) == ACTOR_INDEX_MAX)
		return;
	Blocks& blocks = m_area.getBlocks();
	//TODO: Move line of sight check to threaded task?
	ActorIndex target = m_target.at(index);
	if(blocks.taxiDistance(m_location.at(index), target) <= m_maxMeleeRange.at(index) && blocks.hasLineOfSightTo(m_location.at(index), target))
		combat_attackMeleeRange(index, m_target.at(index));
}
void Actors::combat_update(ActorIndex index)
{
	m_combatScore.at(index) = 0;
	m_maxMeleeRange.at(index) = 0.f;
	m_maxRange.at(index) = 0.f;
	// Collect attacks and combat scores from body and equipment.
	m_meleeAttackTable.clear();
	Body& body = *m_body.at(index).get();
	for(Attack& attack : body.getMeleeAttacks())
		m_meleeAttackTable.at(index).emplace_back(combat_getCombatScoreForAttack(index, attack), attack);
	for(Attack& attack : m_equipmentSet.at(index)->getMeleeAttacks(m_area))
		m_meleeAttackTable.at(index).emplace_back(combat_getCombatScoreForAttack(index, attack), attack);
	// Sort by combat score, low to high.
	std::sort(m_meleeAttackTable.at(index).begin(), m_meleeAttackTable.at(index).end(), [](auto& a, auto& b){ return a.first < b.first; });
	// Iterate attacks low to high, add running total to each score.
	// Also find max melee attack range.
	for(auto& pair : m_meleeAttackTable.at(index))
	{
		pair.first += m_combatScore.at(index);
		m_combatScore.at(index) = pair.first;
		float range = pair.second.attackType->range;
		if(range > m_maxMeleeRange.at(index))
			m_maxMeleeRange.at(index) = range;
	}
	// Base stats give combat score.
	m_combatScore.at(index) += m_attributes.at(index)->getBaseCombatScore();
	// Reduce combat score if manipulation is impaired.
	m_combatScore.at(index) = util::scaleByInversePercent(m_combatScore.at(index), body.getImpairManipulationPercent());
	// Update cool down duration.
	// TODO: Manipulation impairment should apply to cooldown as well?
	m_coolDownDurationModifier.at(index) = std::max(1.f, (float)m_equipmentSet.at(index)->getMass() / (float)m_attributes.at(index)->getUnencomberedCarryMass() );
	if(m_attributes.at(index)->getDextarity() > Config::attackCoolDownDurationBaseDextarity)
	{
		float reduction = (m_attributes.at(index)->getDextarity() - Config::attackCoolDownDurationBaseDextarity) * Config::fractionAttackCoolDownReductionPerPointOfDextarity;
		m_coolDownDurationModifier.at(index) = std::max(m_coolDownDurationModifier.at(index) - reduction, Config::minimumAttackCoolDownModifier);
	}
	// Find the on miss cool down.
	Step baseOnMissCoolDownDuration = m_equipmentSet.at(index)->hasWeapons() ?
	       	m_equipmentSet.at(index)->getLongestMeleeWeaponCoolDown(m_area) : 
		Config::attackCoolDownDurationBaseSteps;
	m_onMissCoolDownMelee.at(index) = m_coolDownDurationModifier.at(index) * baseOnMissCoolDownDuration;
	//Find max range.
	m_maxRange.at(index) = m_maxMeleeRange.at(index);
	for(ItemReference item : m_equipmentSet.at(index)->getRangedWeapons())
	{
		AttackType* attackType = m_area.getItems().getItemType(item.getIndex()).getRangedAttackType();
		if(attackType->range > m_maxRange.at(index))
			m_maxRange.at(index) = attackType->range;
	}
}
std::vector<std::pair<uint32_t, Attack>>& Actors::combat_getMeleeAttacks(ActorIndex index) { return m_meleeAttackTable.at(index); }
//TODO: Grasps cannot be used for both armed and unarmed attacks at the same time?
uint32_t Actors::combat_getCombatScoreForAttack(ActorIndex index, const Attack& attack) const
{
	uint32_t output = attack.attackType->combatScore;
	const SkillType& skill = attack.item == ITEM_INDEX_MAX ?
		SkillType::byName("unarmed") :
		attack.attackType->skillType;
	uint32_t skillValue = m_skillSet.at(index)->get(skill);
	output += (skillValue * Config::attackSkillCombatModifier);
	Items& items = m_area.getItems();
	uint32_t quality = attack.item == ITEM_INDEX_MAX ? Config::averageItemQuality : items.getQuality(attack.item);
	output *= combat_getQualityModifier(index, quality);
	Percent percentItemWear = attack.item == ITEM_INDEX_MAX ? 0u : items.getWear(attack.item);
	output -= percentItemWear * Config::itemWearCombatModifier;
	return output;
}
const Attack& Actors::combat_getAttackForCombatScoreDifference(ActorIndex index, uint32_t scoreDifference) const
{
	for(auto& pair : m_meleeAttackTable.at(index))
		if(pair.first > scoreDifference)
			return pair.second;
	return m_meleeAttackTable.at(index).back().second;
}
void Actors::combat_setTarget(ActorIndex index, ActorIndex actor)
{
	m_target.at(index) = actor;
	combat_recordTargetedBy(actor, index);
	assert(m_getIntoAttackPositionPathRequest.at(index) == nullptr);
	m_getIntoAttackPositionPathRequest.at(index) = std::make_unique<GetIntoAttackPositionPathRequest>(m_area, index, actor, m_maxRange.at(index));
}
void Actors::combat_recordTargetedBy(ActorIndex index, ActorIndex actor)
{
	assert(m_target.at(index) == actor);
	assert(!m_targetedBy.at(index).contains(actor));
	m_targetedBy.at(index).insert(actor);
}
void Actors::combat_removeTargetedBy(ActorIndex index, ActorIndex actor)
{
	assert(m_targetedBy.at(index).contains(actor));
	m_targetedBy.at(index).erase(actor);
}
void Actors::combat_onMoveFrom(ActorIndex index, BlockIndex previous)
{
	// Notify all targeting actors of move so they may reroute.
	for(ActorIndex actor : m_targetedBy.at(index))
		combat_onTargetMoved(actor);
	// Give all directly adjacent enemies a free hit against this actor.
	Blocks& blocks = m_area.getBlocks();
	FactionId faction = m_faction.at(index);
	for(BlockIndex block : blocks.getDirectlyAdjacent(previous))
		if(block != BLOCK_INDEX_MAX)
			for(ActorIndex adjacent : blocks.actor_getAll(block))
			{
				FactionId otherFaction = m_faction.at(adjacent);
				if(m_area.m_simulation.m_hasFactions.isEnemy(faction, otherFaction))
					combat_freeHit(adjacent, index);
			}
}
void Actors::combat_noLongerTargetable(ActorIndex index)
{
	for(ActorIndex actor : m_targetedBy.at(index))
	{
		assert(m_targetedBy.at(actor).contains(index));
		combat_targetNoLongerTargetable(index);
		m_hasObjectives.at(actor)->subobjectiveComplete(m_area);
	}
	m_targetedBy.clear();
}
void Actors::combat_onDeath(ActorIndex index)
{
	combat_noLongerTargetable(index);
}
void Actors::combat_onLeaveArea(ActorIndex index)
{
	combat_noLongerTargetable(index);
}
void Actors::combat_targetNoLongerTargetable(ActorIndex index)
{
	assert(m_target.at(index) != ACTOR_INDEX_MAX);
	m_target.at(index) = ACTOR_INDEX_MAX;
	m_hasObjectives.at(index)->subobjectiveComplete(m_area);
}
void Actors::combat_onTargetMoved(ActorIndex index)
{
	if(m_getIntoAttackPositionPathRequest.at(index) == nullptr)
		m_getIntoAttackPositionPathRequest.at(index) = std::make_unique<GetIntoAttackPositionPathRequest>(m_area, index, m_target.at(index), m_maxRange.at(index));
}
void Actors::combat_freeHit(ActorIndex index, ActorIndex actor)
{
	m_coolDownEvent.maybeUnschedule(index);
	combat_attackMeleeRange(index, actor);
}
void Actors::combat_getIntoRangeAndLineOfSightOfActor(ActorIndex index, ActorIndex target, DistanceInBlocks range)
{
	move_pathRequestRecord(index, std::make_unique<GetIntoAttackPositionPathRequest>(m_area, index, target, range));
}
bool Actors::combat_isOnCoolDown(ActorIndex index) const { return m_coolDownEvent.exists(index); }
bool Actors::combat_inRange(ActorIndex index, const ActorIndex target) const 
{
	Blocks& blocks = m_area.getBlocks();
       	return blocks.distance(m_location.at(index), m_location.at(target)) <= m_maxRange.at(index);
}
Percent Actors::combat_projectileHitPercent(ActorIndex index, const Attack& attack, const ActorIndex target) const
{
	Percent chance = 100 - pow(distanceToActor(index, target), Config::projectileHitChanceFallsOffWithRangeExponent);
	chance += m_skillSet.at(index)->get(attack.attackType->skillType) * Config::projectileHitPercentPerSkillPoint;
	chance += ((int)getVolume(target) - (int)Config::projectileMedianTargetVolume) * Config::projectileHitPercentPerUnitVolume;
	chance += m_attributes.at(index)->getDextarity() * Config::projectileHitPercentPerPointDextarity;
	if(attack.item != ITEM_INDEX_MAX)
	{
		Items& items = m_area.getItems();
		chance += ((int)items.getQuality(attack.item) - Config::averageItemQuality) * Config::projectileHitPercentPerPointQuality;
		chance -= items.getWear(attack.item) * Config::projectileHitPercentPerPointWear;
	}
	chance -= combat_getCombatScore(index) * Config::projectileHitPercentPerPointTargetCombatScore;
	chance += attack.attackType->combatScore * Config::projectileHitPercentPerPointAttackTypeCombatScore;
	return chance;
}
bool Actors::combat_doesProjectileHit(ActorIndex index, Attack& attack, const ActorIndex target) const
{
	Percent chance = combat_projectileHitPercent(index, attack, target);
	return m_area.m_simulation.m_random.percentChance(chance);
}
float Actors::combat_getQualityModifier(ActorIndex, uint32_t quality) const
{
	int32_t adjusted = (int32_t)quality - (int32_t)Config::averageItemQuality;
	return 1.f + (adjusted * Config::itemQualityCombatModifier);
}
bool Actors::combat_blockIsValidPosition(ActorIndex index, BlockIndex block, uint32_t attackRangeSquared) const
{
	if(getBlocks(m_target.at(index)).contains(block))
		return true;
	Blocks& blocks = m_area.getBlocks();
	BlockIndex targetLocation = m_location.at(m_target.at(index));
	if(blocks.squareOfDistanceIsMoreThen(block, targetLocation, attackRangeSquared))
		return false;
	return blocks.hasLineOfSightTo(block, targetLocation);
}
AttackType& Actors::combat_getRangedAttackType(ActorIndex, ItemIndex weapon)
{
	// Each ranged weapon has only one ranged attack type to pick.
	const ItemType& itemType = m_area.getItems().getItemType(weapon);
	assert(itemType.weaponData != nullptr);
	for(const AttackType& attackType : itemType.weaponData->attackTypes)
		if(attackType.projectile)
			return const_cast<AttackType&>(attackType);
	assert(false);
	return const_cast<AttackType&>(itemType.weaponData->attackTypes.front());
}
AttackCoolDownEvent::AttackCoolDownEvent(Area& area, ActorIndex actor, Step duration, const Step start) :
	ScheduledEvent(area.m_simulation, duration, start), m_actor(actor) { }

GetIntoAttackPositionPathRequest::GetIntoAttackPositionPathRequest(Area& area, ActorIndex a, ActorIndex t, float ar) :
	m_actor(a), m_attackRangeSquared(ar * ar)
{
	m_target.setTarget(area.getActors().getReferenceTarget(t));
	std::function<bool(BlockIndex, Facing)> destinationCondition = [&area, this](BlockIndex location, Facing)
	{
		return area.getActors().combat_blockIsValidPosition(m_actor, location, m_attackRangeSquared);
	};
	// TODO: Range attack actors should use a different path priority condition to avoid getting too close.
	bool detour = true;
	bool unreserved = false;
	DistanceInBlocks maxRange = BLOCK_DISTANCE_MAX;
	createGoToCondition(area, m_actor, destinationCondition, detour, unreserved, maxRange, area.getActors().getLocation(t));
}
void GetIntoAttackPositionPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	if(result.path.empty())
	{
		if(result.useCurrentPosition)
		{
			if(!actors.combat_isOnCoolDown(m_actor))
			{
				ItemIndex target = m_target.getIndex();
				float range = actors.distanceToActor(m_actor, target);
				if(range <= actors.combat_getMaxMeleeRange(m_actor))
					// Melee range attack.
					actors.combat_attackMeleeRange(m_actor, target);
				else
				{
					// Long range attack.
					// TODO: Unarmed long range attack, needs to prodive the material type somehow, maybe in body part?
					ItemIndex weapon = actors.equipment_getWeaponToAttackAtRange(m_actor, range);
					ItemIndex ammo = actors.equipment_getAmmoForRangedWeapon(m_actor, weapon);
					actors.combat_attackLongRange(m_actor, target, weapon, ammo);
				}
			}
		}
		else
			actors.objective_canNotCompleteSubobjective(m_actor);
	}
	else
		actors.move_setPath(m_actor, result.path);
}
