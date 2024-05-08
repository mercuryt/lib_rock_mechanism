#include "fight.h"

#include "attackType.h"
#include "simulation.h"
#include "skill.h"
#include "util.h"
#include "actor.h"
#include "weaponType.h"
#include "block.h"
#include "config.h"
#include "random.h"

#include <cstdint>
#include <sys/types.h>
#include <utility>
#include <algorithm>
// CanFight.
CanFight::CanFight(Actor& a) : m_actor(a), m_maxMeleeRange(0), m_coolDownDurationModifier(1.f), m_coolDown(m_actor.getEventSchedule()), m_getIntoAttackPositionThreadedTask(m_actor.getThreadedTaskEngine()), m_target(nullptr)
{ 
	update();
}
CanFight::CanFight(const Json& data, Actor& a) : m_actor(a), m_maxMeleeRange(0), m_coolDownDurationModifier(1.f), m_coolDown(m_actor.getEventSchedule()), m_getIntoAttackPositionThreadedTask(m_actor.getThreadedTaskEngine()), m_target(nullptr)
{
	update();
	if(data.contains("coolDownStart"))
		m_coolDown.schedule(*this, data["coolDownDuration"].get<Step>(), data["coolDownStart"].get<Step>());
	if(data.contains("target"))
		setTarget(m_actor.getSimulation().getActorById(data["target"].get<ActorId>()));
}
Json CanFight::toJson() const
{
	Json data;
	if(m_coolDown.exists())
		data["coolDownStart"] = m_coolDown.getStartStep();
	if(m_target != nullptr)
		data["target"] = m_target->m_id;
	return data;
}
void CanFight::attackMeleeRange(Actor& target)
{
	assert(!m_coolDown.exists());
	uint32_t attackerCombatScore = getCurrentMeleeCombatScore();
	uint32_t targetCombatScore = target.m_canFight.getCurrentMeleeCombatScore();
	Step coolDownDuration = m_onMissCoolDownMelee;
	if(attackerCombatScore > targetCombatScore)
	{
		// Attack hits.
		const Attack& attack = getAttackForCombatScoreDifference(attackerCombatScore - targetCombatScore);
		Force attackForce = attack.attackType->baseForce + (m_actor.m_attributes.getStrength() * Config::unitsOfAttackForcePerUnitOfStrength);
		// TODO: Higher skill selects more important body parts to hit.
		BodyPart& bodyPart = target.m_body.pickABodyPartByVolume();
		Hit hit(attack.attackType->area, attackForce, *attack.materialType, attack.attackType->woundType);
		target.takeHit(hit, bodyPart);
		// If there is a weapon being used take the cool down from it, otherwise use onMiss cool down.
		if(attack.item != nullptr)
		{
			coolDownDuration = attack.attackType->coolDown == 0 ? attack.item->m_itemType.weaponData->coolDown : attack.attackType->coolDown;
			coolDownDuration *= m_coolDownDurationModifier;
		}
	}
	m_coolDown.schedule(*this, coolDownDuration);
	//TODO: Skill growth.
}
void CanFight::attackLongRange(Actor& target, Item* weapon, Item* ammo)
{
	//TODO: unarmed ranged attack?
	const AttackType& attackType = getRangedAttackType(*weapon);
	Step coolDown = attackType.coolDown == 0 ? weapon->m_itemType.weaponData->coolDown : attackType.coolDown;
	Attack attack(&attackType, &ammo->m_materialType, weapon);
	if(doesProjectileHit(attack, target))
	{
		// Attack hits.
		// TODO: Higher skill selects more important body parts to hit.
		BodyPart& bodyPart = target.m_body.pickABodyPartByVolume();
		Hit hit(attackType.area, attackType.baseForce, *attack.materialType, attack.attackType->woundType);
		target.takeHit(hit, bodyPart);
	}
	m_coolDown.schedule(*this, coolDown);
}
uint32_t CanFight::getCurrentMeleeCombatScore()
{
	const Faction* faction = m_actor.getFaction();
	uint32_t blocksContainingNonAllies = 0;
	// Apply bonuses and penalties based on relative locations.
	uint32_t output = m_combatScore;
	for(HasShape* hasShape : m_actor.getAdjacentHasShapes())
	{
		uint32_t highestAllyCombatScore = 0;
		bool nonAllyFound = false;
		if(hasShape->isActor())
		{
			Actor& actor = static_cast<Actor&>(*hasShape);
			if(actor.getFaction() && (actor.getFaction() == faction || actor.getFaction()->allies.contains(const_cast<Faction*>(faction))))
			{
				if(actor.m_canFight.m_combatScore > highestAllyCombatScore)
					highestAllyCombatScore = actor.m_canFight.m_combatScore;
			}
			else
				nonAllyFound = true;
		}
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
void CanFight::coolDownCompleted()
{
	if(m_target == nullptr)
		return;
	//TODO: Move line of sight check to threaded task?
	if(m_actor.m_location->taxiDistance(*m_target->m_location) <= m_maxMeleeRange && m_actor.m_location->hasLineOfSightTo(*m_target->m_location))
		attackMeleeRange(*m_target);
}
void CanFight::update()
{
	m_combatScore = 0;
	m_maxMeleeRange = 0.f;
	m_maxRange = 0.f;
	// Collect attacks and combat scores from body and equipment.
	m_meleeAttackTable.clear();
	for(Attack& attack : m_actor.m_body.getMeleeAttacks())
		m_meleeAttackTable.emplace_back(getCombatScoreForAttack(attack), attack);
	for(Attack& attack : m_actor.m_equipmentSet.getMeleeAttacks())
		m_meleeAttackTable.emplace_back(getCombatScoreForAttack(attack), attack);
	// Sort by combat score, low to high.
	std::sort(m_meleeAttackTable.begin(), m_meleeAttackTable.end(), [](auto& a, auto& b){ return a.first < b.first; });
	// Iterate attacks low to high, add running total to each score.
	// Also find max melee attack range.
	for(auto& pair : m_meleeAttackTable)
	{
		pair.first += m_combatScore;
		m_combatScore = pair.first;
		float range = pair.second.attackType->range;
		if(range > m_maxMeleeRange)
			m_maxMeleeRange = range;
	}
	// Base stats give combat score.
	m_combatScore += m_actor.m_attributes.getBaseCombatScore();
	// Reduce combat score if manipulation is impaired.
	m_combatScore = util::scaleByInversePercent(m_combatScore, m_actor.m_body.getImpairManipulationPercent());
	// Update cool down duration.
	// TODO: Manipulation impairment should apply to cooldown as well?
	m_coolDownDurationModifier = std::max(1.f, (float)m_actor.m_equipmentSet.getMass() / (float)m_actor.m_attributes.getUnencomberedCarryMass() );
	if(m_actor.m_attributes.getDextarity() > Config::attackCoolDownDurationBaseDextarity)
	{
		float reduction = (m_actor.m_attributes.getDextarity() - Config::attackCoolDownDurationBaseDextarity) * Config::fractionAttackCoolDownReductionPerPointOfDextarity;
		m_coolDownDurationModifier = std::max(m_coolDownDurationModifier - reduction, Config::minimumAttackCoolDownModifier);
	}
	// Find the on miss cool down.
	Step baseOnMissCoolDownDuration = m_actor.m_equipmentSet.hasWeapons() ?
	       	m_actor.m_equipmentSet.getLongestMeleeWeaponCoolDown() : 
		Config::attackCoolDownDurationBaseSteps;
	m_onMissCoolDownMelee = m_coolDownDurationModifier * baseOnMissCoolDownDuration;
	//Find max range.
	m_maxRange = m_maxMeleeRange;
	for(Item* item : m_actor.m_equipmentSet.getRangedWeapons())
	{
		AttackType* attackType = item->m_itemType.getRangedAttackType();
		if(attackType->range > m_maxRange)
			m_maxRange = attackType->range;
	}
}
//TODO: Grasps cannot be used for both armed and unarmed attacks at the same time?
uint32_t CanFight::getCombatScoreForAttack(const Attack& attack) const
{
	uint32_t output = attack.attackType->combatScore;
	const SkillType& skill = attack.item == nullptr ?
		SkillType::byName("unarmed") :
		attack.attackType->skillType;
	uint32_t skillValue = m_actor.m_skillSet.get(skill);
	output += (skillValue * Config::attackSkillCombatModifier);
	uint32_t quality = attack.item == nullptr ? Config::averageItemQuality : attack.item->m_quality;
	output *= getQualityModifier(quality);
	Percent percentItemWear = attack.item == nullptr ? 0u : attack.item->m_percentWear;
	output -= percentItemWear * Config::itemWearCombatModifier;
	return output;
}
const Attack& CanFight::getAttackForCombatScoreDifference(uint32_t scoreDifference) const
{
	for(auto& pair : m_meleeAttackTable)
		if(pair.first > scoreDifference)
			return pair.second;
	return m_meleeAttackTable.back().second;
}
void CanFight::setTarget(Actor& actor)
{
	m_target = &actor;
	actor.m_canFight.recordTargetedBy(m_actor);
	m_getIntoAttackPositionThreadedTask.create(m_actor, *m_target, m_maxRange);
}
void CanFight::recordTargetedBy([[maybe_unused]] Actor& actor)
{
	assert(actor.m_canFight.m_target == &m_actor);
	assert(!m_targetedBy.contains(&actor));
	m_targetedBy.insert(&m_actor);
}
void CanFight::removeTargetedBy(Actor& actor)
{
	assert(m_targetedBy.contains(&m_actor));
	m_targetedBy.erase(&actor);
}
void CanFight::onMoveFrom(Block& previous)
{
	// Notify all targeting actors of move so they may reroute.
	for(Actor* actor : m_targetedBy)
		actor->m_canFight.onTargetMoved();
	// Give all directly adjacent enemies a free hit against this actor.
	for(Block* block : previous.m_adjacents)
		if(block)
			for(Actor* actor : block->m_hasActors.getAll())
				if(actor->getFaction()->enemies.contains(const_cast<Faction*>(m_actor.getFaction())))
					actor->m_canFight.freeHit(m_actor);
}
void CanFight::noLongerTargetable()
{
	for(Actor* actor : m_targetedBy)
	{
		assert(actor->m_canFight.m_target == &m_actor);
		actor->m_canFight.targetNoLongerTargetable();
		actor->m_hasObjectives.subobjectiveComplete();
	}
	m_targetedBy.clear();
}
void CanFight::onDeath()
{
	noLongerTargetable();
}
void CanFight::onLeaveArea()
{
	noLongerTargetable();
}
void CanFight::targetNoLongerTargetable()
{
	assert(m_target != nullptr);
	m_target = nullptr;
	m_actor.m_hasObjectives.subobjectiveComplete();
}
void CanFight::onTargetMoved()
{
	if(!m_getIntoAttackPositionThreadedTask.exists())
		m_getIntoAttackPositionThreadedTask.create(m_actor, *m_target, m_actor.m_canFight.m_maxRange);
}
void CanFight::freeHit(Actor& actor)
{
	m_coolDown.maybeUnschedule();
	attackMeleeRange(actor);
}
bool CanFight::isOnCoolDown() const { return m_coolDown.exists(); }
bool CanFight::inRange(const Actor& target) const { return m_actor.m_location->distance(*target.m_location) <= m_maxRange; }
Percent CanFight::projectileHitPercent(const Attack& attack, const Actor& target) const
{
	Percent chance = 100 - pow(m_actor.distanceTo(target), Config::projectileHitChanceFallsOffWithRangeExponent);
	chance += m_actor.m_skillSet.get(attack.attackType->skillType) * Config::projectileHitPercentPerSkillPoint;
	chance += ((int)target.getVolume() - (int)Config::projectileMedianTargetVolume) * Config::projectileHitPercentPerUnitVolume;
	chance += m_actor.m_attributes.getDextarity() * Config::projectileHitPercentPerPointDextarity;
	if(attack.item != nullptr)
	{
		chance += ((int)attack.item->m_quality - Config::averageItemQuality) * Config::projectileHitPercentPerPointQuality;
		chance -= attack.item->m_percentWear * Config::projectileHitPercentPerPointWear;
	}
	chance -= target.m_canFight.getCombatScore() * Config::projectileHitPercentPerPointTargetCombatScore;
	chance += attack.attackType->combatScore * Config::projectileHitPercentPerPointAttackTypeCombatScore;
	return chance;
}
bool CanFight::doesProjectileHit(Attack& attack, const Actor& target) const
{
	Percent chance = projectileHitPercent(attack, target);
	return m_actor.getSimulation().m_random.percentChance(chance);
}
float CanFight::getQualityModifier(uint32_t quality) const
{
	int32_t adjusted = (int32_t)quality - (int32_t)Config::averageItemQuality;
	return 1.f + (adjusted * Config::itemQualityCombatModifier);
}
bool CanFight::blockIsValidPosition(const Block& block, uint32_t attackRangeSquared) const
{
	if(m_target->m_blocks.contains(const_cast<Block*>(&block)))
		return true;
	if(block.squareOfDistanceIsMoreThen(*m_target->m_location, attackRangeSquared))
		return false;
	return block.hasLineOfSightTo(*m_target->m_location);
}
bool CanFight::predicate(const Block& block, Facing facing, uint32_t attackRangeSquared) const
{
	std::function<bool(const Block&)> occupiedCondition = [&](const Block& block) { return blockIsValidPosition(block, attackRangeSquared); };
	return m_actor.getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(block, facing, occupiedCondition) != nullptr;
}
AttackType& CanFight::getRangedAttackType(Item& weapon)
{
	// Each ranged weapon has only one ranged attack type to pick.
	assert(weapon.m_itemType.weaponData != nullptr);
	for(const AttackType& attackType : weapon.m_itemType.weaponData->attackTypes)
		if(attackType.projectile)
			return const_cast<AttackType&>(attackType);
	assert(false);
	return const_cast<AttackType&>(weapon.m_itemType.weaponData->attackTypes.front());
}

AttackCoolDown::AttackCoolDown(CanFight& cf, Step duration, const Step start) : ScheduledEvent(cf.m_actor.getSimulation(), duration, start), m_canFight(cf) { }

GetIntoAttackPositionThreadedTask::GetIntoAttackPositionThreadedTask(Actor& a, Actor& t, float ar) : ThreadedTask(a.getThreadedTaskEngine()), m_actor(a), m_target(t), m_attackRangeSquared(ar * ar), m_findsPath(a, true) {}
void GetIntoAttackPositionThreadedTask::readStep()
{
	std::function<bool(const Block&, Facing)> destinationCondition = [&](const Block& location, Facing facing)
	{
		return m_actor.m_canFight.predicate(location, facing, m_attackRangeSquared);
	};
	// TODO: Range attack actors should use a different path priority condition to avoid getting too close.
	m_findsPath.pathToPredicateWithHuristicDestination(destinationCondition, *m_target.m_location);
}
void GetIntoAttackPositionThreadedTask::writeStep()
{
	if(!m_findsPath.found())
	{
		if(m_findsPath.m_useCurrentLocation)
		{
			if(!m_actor.m_canFight.m_coolDown.exists())
			{
				float range = m_actor.distanceTo(m_target);
				if(range <= m_actor.m_canFight.m_maxMeleeRange)
					// Melee range attack.
					m_actor.m_canFight.attackMeleeRange(m_target);
				else
				{
					// Long range attack.
					// TODO: Unarmed long range attack, needs to prodive the material type somehow, maybe in body part?
					Item* weapon = m_actor.m_equipmentSet.getWeaponToAttackAtRange(range);
					Item* ammo = m_actor.m_equipmentSet.getAmmoForRangedWeapon(*weapon);
					m_actor.m_canFight.attackLongRange(m_target, weapon, ammo);
				}
			}
		}
		else
			m_actor.m_hasObjectives.cannotCompleteSubobjective();
	}
	else
		m_actor.m_canMove.setPath(m_findsPath.getPath());
}
void GetIntoAttackPositionThreadedTask::clearReferences() { m_actor.m_canFight.m_getIntoAttackPositionThreadedTask.clearPointer(); }
