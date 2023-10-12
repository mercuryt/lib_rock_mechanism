#include "fight.h"

#include "util.h"
#include "actor.h"
#include "weaponType.h"
#include "block.h"

#include <utility>
#include <algorithm>
// CanFight.
CanFight::CanFight(Actor& a) : m_actor(a), m_maxRange(0), m_coolDownDuration(0), m_coolDown(m_actor.getEventSchedule()), m_getIntoAttackPositionThreadedTask(m_actor.getThreadedTaskEngine()) 
{ 
	update();
}
void CanFight::attack(Actor& target)
{
	assert(!m_coolDown.exists());
	m_coolDown.schedule(*this);
	uint32_t attackerCombatScore = m_combatScore;
	uint32_t targetCombatScore = target.m_canFight.m_combatScore;
	if(attackerCombatScore > targetCombatScore)
	{
		const Attack& attack = getAttackForCombatScoreDifference(attackerCombatScore - targetCombatScore);
		Force attackForce = attack.attackType->baseForce + (m_actor.m_attributes.getStrength() * Config::unitsOfAttackForcePerUnitOfStrength);
		BodyPart& bodyPart = target.m_body.pickABodyPartByVolume();
		Hit hit(attack.attackType->area, attackForce, *attack.materialType, attack.attackType->woundType);
		target.takeHit(hit, bodyPart);
	}
	//TODO: Skill growth.
}
void CanFight::coolDownCompleted()
{
	if(m_target == nullptr)
		return;
	if(m_actor.m_location->taxiDistance(*m_target->m_location) <= m_maxRange && m_actor.m_location->hasLineOfSightTo(*m_target->m_location))
		attack(*m_target);
}
void CanFight::update()
{
	m_combatScore = 0;
	m_maxRange = 0;
	// Collect attacks and combat scores from body and equipment.
	m_attackTable.clear();
	for(Attack& attack : m_actor.m_body.getAttacks())
		m_attackTable.emplace_back(getCombatScoreForAttack(attack), attack);
	for(Attack& attack : m_actor.m_equipmentSet.getAttacks())
		m_attackTable.emplace_back(getCombatScoreForAttack(attack), attack);
	// Sort by combat score, low to high.
	std::sort(m_attackTable.begin(), m_attackTable.end(), [](auto& a, auto& b){ return a.first < b.first; });
	// Iterate attacks low to high, add running total to each score.
	for(auto& pair : m_attackTable)
	{
		pair.first += m_combatScore;
		m_combatScore += pair.first;
		m_maxRange = std::min(m_maxRange, pair.second.attackType->range);
	}
	// Reduce combat score if manipulation is impaired.
	m_combatScore = util::scaleByInversePercent(m_combatScore, m_actor.m_body.getImpairManipulationPercent());
	// Update cool down duration.
	// TODO: Manipulation impairment should apply to these as well?
	uint32_t coolDownDurationDextarityBonus = m_actor.m_attributes.getDextarity();
	float coolDownDurationEquipmentMassModifier = m_actor.m_equipmentSet.empty() ? 1u : 
		std::max(1u, m_actor.m_attributes.getUnencomberedCarryMass() / m_actor.m_equipmentSet.getMass());
	m_coolDownDuration = std::max(Step(1), Config::attackCoolDownDurationBase - coolDownDurationDextarityBonus) * coolDownDurationEquipmentMassModifier;
}
//TODO: Grasps cannot be used for both armed and unarmed attacks at the same time?
uint32_t CanFight::getCombatScoreForAttack(const Attack& attack) const
{
	if(attack.item == nullptr)
	{
		// unarmed attack
		static const SkillType& unarmedSkillType = SkillType::byName("unarmed");
		uint32_t unarmedSkill = m_actor.m_skillSet.get(unarmedSkillType);
		return Config::unarmedCombatScoreBase + unarmedSkill;
	}
	assert(attack.item->m_itemType.weaponData != nullptr);
	uint32_t itemTypeCombatScore = attack.item->m_itemType.weaponData->combatScoreBonus;
	uint32_t itemSkill = m_actor.m_skillSet.get(*attack.item->m_itemType.weaponData->combatSkill);
	uint32_t itemQuality = attack.item->m_quality;
	Percent percentItemWear = attack.item->m_percentWear;
	return  (
			(itemTypeCombatScore * Config::itemTypeCombatModifier) + 
			(itemSkill * Config::itemSkillCombatModifier) + 
			(itemQuality * Config::itemQualityCombatModifier) + 
			(percentItemWear * Config::itemWearCombatModifier)
		);
}
const Attack& CanFight::getAttackForCombatScoreDifference(uint32_t scoreDifference) const
{
	for(auto& pair : m_attackTable)
		if(pair.first > scoreDifference)
			return pair.second;
	return m_attackTable.back().second;
}
void CanFight::setTarget(Actor& actor)
{
	m_target = &actor;
	actor.m_canFight.recordTargetedBy(m_actor);
}
void CanFight::recordTargetedBy(Actor& actor)
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
	for(Block* block : previous.m_adjacentsVector)
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
		actor->m_hasObjectives.taskComplete();
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
	m_actor.m_hasObjectives.taskComplete();
}
void CanFight::onTargetMoved()
{
	if(!m_getIntoAttackPositionThreadedTask.exists())
		m_getIntoAttackPositionThreadedTask.create(m_actor, *m_target, m_maxRange);
}
void CanFight::freeHit(Actor& actor)
{
	m_coolDown.maybeUnschedule();
	attack(actor);
}
bool CanFight::isOnCoolDown() const { return m_coolDown.exists(); }
bool CanFight::inRange(const Actor& target) const { return m_actor.m_location->distance(*target.m_location) <= m_maxRange; }

AttackCoolDown::AttackCoolDown(CanFight& cf) : ScheduledEventWithPercent(cf.m_actor.getSimulation(), cf.m_coolDownDuration), m_canFight(cf) { }

GetIntoAttackPositionThreadedTask::GetIntoAttackPositionThreadedTask(Actor& a, Actor& t, uint32_t r) : ThreadedTask(a.getThreadedTaskEngine()), m_actor(a), m_target(t), m_range(r), m_findsPath(a) {}
void GetIntoAttackPositionThreadedTask::readStep()
{
	std::function<bool(const Block&, Facing)> destinatonCondition = [&](const Block& location, Facing facing)
	{
		std::function<bool(const Block&)> occupiedCondition = [&](const Block& block)
		{
			return block.taxiDistance(*m_target.m_location) <= m_range && block.hasLineOfSightTo(*m_target.m_location);
		};
		return m_actor.getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(location, facing, occupiedCondition) != nullptr;
	};
	//TODO: Range attack units should use a different path priority condition to avoid getting too close.
	m_findsPath.pathToPredicateWithHuristicDestination(destinatonCondition, *m_target.m_location);
}
void GetIntoAttackPositionThreadedTask::writeStep()
{
	if(!m_findsPath.found())
		m_actor.m_hasObjectives.cannotCompleteTask();
	else
		m_actor.m_canMove.setPath(m_findsPath.getPath());
}
void GetIntoAttackPositionThreadedTask::clearReferences() { m_actor.m_canFight.m_getIntoAttackPositionThreadedTask.clearPointer(); }
