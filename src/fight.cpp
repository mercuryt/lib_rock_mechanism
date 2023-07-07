#include "attack.h"
// CanFight.
CanFight::CanFight(Actor& a) : m_actor(a), m_maxRange(0), m_coolDownDuration(0) { }
void CanFight::attack(Actor& target)
{
	assert(!coolDown());
	m_coolDown.schedule(m_coolDownDuration, *this);
	uint32_t attackerCombatScore = getCombatScore(m_actor);
	uint32_t targetCombatScore = getCombatScore(target);
	if(attackerCombatScore > targetCombatScore)
	{
		const Attack& attack = getAttack(attackerCombatScore - targetCombatScore);
		uint32_t attackForce = attack.attackType.baseForce * attacker.m_attackForceModifier; 
		const BodyPart& bodyPart = target.body.pickABodyPartByVolume();
		Hit hit(attack.attackType.area, attackForce);
		// hit is an in-out paramater.
		target.m_body.getHitDepth(hit, bodyPart, attack.materialType);
		target.m_equipmentSet.modifyImpact(hit, attack.materialType, bodyPart.bodyPartType);
		Wound& wound = target.m_body.addWound(hit, bodyPart);
	}
	//TODO: Skill growth.
}
void CanFight::updateCoolDownDuration()
{
	m_coolDownDuration = Config::attackCoolDownDurationBase - (m_actor.m_attributes.dextarity * Config::stepsAttackCoolDownReductionPerPointOfDextarity) * m_actor.m_hasItems.getAttackCoolDownDurationModifier;
}
void CanFight::update()
{
	m_combatScore = 0;
	m_maxRange = 0;
	// Collect attacks and combat scores from body and equipment.
	m_attackTable.clear();
	for(Attack& attack : actor.m_body.getAttacks())
		m_attackTable.emplace_back(getCombatScoreForAttack(attack), attack);
	for(Attack& attack : actor.m_equipment.getAttacks())
		m_attackTable.emplace_back(getCombatScoreForAttack(attack), attack);
	// Sort by combat score, low to high.
	std::ranges::sort(m_attackTable, [&](std::pair<uint32_t, Attack> a, std::pair<uint32_t, Attack> b){ return a.first > b.first; });
	// Iterate attacks low to high, add running total to each score.
	for(auto& pair : m_attackTable)
	{
		pair->first += m_combatScore;
		combatScore += pair->first;
		m_maxRange = std::min(m_maxRange, pair->second.range);
	}
}
//TODO: Grasps cannot be used for both armed and unarmed attacks at the same time?
uint32_t CanFight::getCombatScoreForAttack(Attack& attack) const
{
	uint32_t itemTypeCombatScore = attack.item->m_itemType.combatScore;
	uint32_t itemSkill = actor.m_skills.get(item->m_itemType.combatSkill);
	uint32_t itemQuality = attack.item->m_quality;
	uint32_t itemWear = attack.item->m_wear;
	return  (
			(itemTypeCombatScore * Config::itemTypeCombatModifier) + 
			(itemSkill * Config::itemSkillCombatModifier) + 
			(itemQuality * Config::itemQualityCombatModifier) + 
			(itemWear * Config::itemWearCombatModifier) + 
		);
}
Attack& CanFight::getAttackForCombatScoreDifference(uint32_t scoreDifference)
{
	for(auto& pair : m_attackTable)
		if(pair.first > scoreDifference)
			return pair.second;
	return m_attackTable.back().second;
}
void CanFight::setTarget(Actor& actor)
{
	m_target = actor;
	actor.m_canFight.recordTargetedBy(m_actor);
}
void CanFight::recordTargetedBy(Actor& actor)
{
	assert(actor.m_target == &m_actor);
	assert(!m_targetedBy.contains(&actor));
	m_targetedBy.insert(m_actor);
}
void CanFight::removeTargetedBy(Actor& actor)
{
	assert(m_targetedBy.contains(&m_actor));
	m_targetedBy.remove(&actor);
}
void CanFight::onMoveFrom(Block* previous)
{
	// Notify all targeting actors of move so they may reroute.
	for(Actor& actor : m_targetedBy)
		actor.m_canFight.targetMoved(*m_actor.m_location);
	// Give all directly adjacent enemies a free hit against this actor.
	for(Block* block : previous.m_adjacentsVector)
		for(Actor& actor : block.hasActors.get())
			if(actor.faction.isEnemy(m_actor.faction))
				actor.m_canFight.freeHit(m_actor);
}
void CanFight::noLongerTargetable()
{
	for(Actor& actor : m_targetedBy)
	{
		assert(actor.m_canFight.m_target == this);
		actor.m_canFight.targetNoLongerTargetable();
		actor.m_hasObjectives.taskComplete();
	}
	m_targetedBy.clear();
}
void CanFight::targetNoLongerTargetable()
{
	assert(m_target != nullptr);
	m_target = nullptr;
	m_actor.m_hasObjectives.taskComplete();
}
bool CanFight::isOnCoolDown() const { return !m_coolDown.empty(); }
bool CanFight::inRange(Actor& target) { return m_actor.m_location->distance(*target.m_location) <= m_maxRange; }

AttackCoolDown::execute() { m_canFight.coolDownCompleted(); }
AttackCoolDown::~AttackCoolDown() { m_canFight.m_coolDown.clearPointer(); }

void GetIntoAttackPositionThreadedTask::readStep()
{
	auto destinatonCondition = [&](Block* block)
	{
		return block->taxiDistance(m_target.m_location) <= m_range && block.hasLineOfSightTo(m_target.m_location);
	}
	m_route = path::getForActorToPredicate(m_actor, destinatonCondition);
}
void GetIntoAttackPositionThreadedTask::writeStep()
{
	if(m_route == nullptr)
		m_actor.m_hasObjectives.cannotCompleteTask();
	else
		m_actor.setPath(m_route);
}
