#include "combatScore.h"
#include <string>

template<class Actor>
struct AttackEvent : ScheduledEvent
{
	Actor& m_actor;
	AttackEvent(uint32_t s, Actor& a) : ScheduledEvent(s), m_actor(a) {}
	void execute()
       	{ 
		Actor* target = selectTarget(actor);
		if(target != nullptr)
			attack(m_actor, target);
       	}
	~AttackEvent()
	{
		if(m_actor.m_taskEvent != nullptr)
		{
			assert(m_actor.m_taskEvent == this);
			m_actor.m_taskEvent.cancel();
		}
	}
};
template<class AttackType>
struct Attack
{
	const AttackType& attackType;
	Equipment* equipment; // Can be nullptr for natural weapons.
	const MaterialType& materialType;
};
template<class Actor>
void attack(Actor& attacker, Actor& target)
{
	uint32_t attackerCombatScore = getCombatScore(attacker);
	uint32_t targetCombatScore = getCombatScore(target);
	if(attackerCombatScore > targetCombatScore)
	{
		const Attack& attack = attacker.m_equipmentSet.getAttack(attackerCombatScore - targetCombatScore);
		uint32_t attackForce = attack.attackType.baseForce * attacker.m_attackForceModifier; 
		const BodyPart& bodyPart = target.body.pickABodyPartByVolume();
		Hit hit(attack.attackType.area, attackForce);
		target.m_body.getHitDepth(hit, bodyPart, attack.materialType);
		target.m_equipmentSet.modifyImpact(hit, attack.materialType, bodyPart.bodyPartType);
		Wound& wound = target.m_body.addWound(hit, bodyPart);
		attacker.onHit(hit, wound, target);
	}
	else
		attacker.onMiss(target);
	m_actor.scheduleAttackEvent();
}
// Can return nullptr;
template<class Actor>
Actor* selectTarget(const Actor& actor)
{
	u_int32_t lowestCombatScore = 
	for(Block* block : actor.m_location->m_adjacentsVector)
		for(Actor* actor : block.m_actors)

}
