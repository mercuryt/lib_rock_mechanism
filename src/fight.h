#pragma once

#include "eventSchedule.h"
#include "item.h"
#include "hit.h"
#include "attackType.h"
#include "materialType.h"
#include "threadedTask.h"

#include <string>
#include <unordered_set>
#include <utility>

class Actor;
class Block;
class AttackCoolDown;
class GetIntoAttackPositionThreadedTask;

struct Attack final
{
	const AttackType& attackType;
	const MaterialType& materialType;
	Item* item; // Can be nullptr for natural weapons.
};
class CanFight final
{
	Actor& m_actor;
	uint32_t m_maxRange;
	uint32_t m_coolDownDuration;
	uint32_t m_combatScore;
	HasScheduledEvent<AttackCoolDown> m_coolDown;
	HasThreadedTask<GetIntoAttackPositionThreadedTask> m_getIntoAttackPositionThreadedTask;
	std::vector<std::pair<uint32_t, Attack>> m_attackTable;
	Actor* m_target;
	std::unordered_set<Actor*> m_targetBy;
public:
	CanFight(Actor& a) : m_actor(a), m_maxRange(0), m_coolDownDuration(0) { }
	void attack(Actor& target);
	void coolDownCompleted();
	void updateMaxRange();
	void updateCoolDownDuration();
	void updateAttackTable();
	void setTarget(Actor& actor);
	void recordTargetedBy(Actor& actor);
	void removeTargetedBy(Actor& actor);
	void onMoveFrom(Block* previous);
	void onDie();
	bool isOnCoolDown() const;
	bool inRange(Actor& target) const;
	friend class AttackCoolDown;
	friend class GetIntoAttackPositionThreadedTask;
};
class AttackCoolDown final : ScheduledEvent
{
	CanFight& m_canFight;
public:
	AttackCoolDown(CanFight& cf) : ScheduledEvent(cf.m_coolDownDuration), m_canFight(cf) { }
	void execute();
	~AttackCoolDown();
};
class GetIntoAttackPositionThreadedTask : ThreadedTask
{
	Actor& m_actor;
	Actor& m_target;
	uint32_t m_range;
	std::vector<Block*> m_route;
public:
	GetIntoAttackPositionThreadedTask(Actor& a, Actor& t, uint32_t r) : m_actor(a), m_target(t), m_range(r) {}
	void readStep();
	void writeStep();
};
