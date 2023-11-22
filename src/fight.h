#pragma once

#include "eventSchedule.h"
#include "item.h"
#include "hit.h"
#include "attackType.h"
#include "materialType.h"
#include "threadedTask.h"
#include "eventSchedule.hpp"
#include "findsPath.h"

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <unordered_set>
#include <utility>

class Actor;
class Block;
class AttackCoolDown;
class GetIntoAttackPositionThreadedTask;

struct Attack final
{
	// Use pointers rather then references to allow move.
	const AttackType* attackType;
	const MaterialType* materialType;
	Item* item; // Can be nullptr for natural weapons.
};
class CanFight final
{
	Actor& m_actor;
	float m_maxMeleeRange;
	float m_maxRange;
	float m_coolDownDurationModifier;
	Step m_onMissCoolDownMelee;
	uint32_t m_combatScore;
	HasScheduledEvent<AttackCoolDown> m_coolDown;
	HasThreadedTask<GetIntoAttackPositionThreadedTask> m_getIntoAttackPositionThreadedTask;
	std::vector<std::pair<uint32_t, Attack>> m_meleeAttackTable;
	Actor* m_target;
	//TODO: Should be a vector.
	std::unordered_set<Actor*> m_targetedBy;
public:
	CanFight(Actor& a);
	void attackMeleeRange(Actor& target);
	void attackLongRange(Actor& target, Item* weapon, Item* ammo);
	uint32_t getCurrentMeleeCombatScore();
	void coolDownCompleted();
	void update();
	void setTarget(Actor& actor);
	void recordTargetedBy(Actor& actor);
	void removeTargetedBy(Actor& actor);
	void onMoveFrom(Block& previous);
	void onDeath();
	void onLeaveArea();
	void noLongerTargetable();
	void targetNoLongerTargetable();
	void onTargetMoved();
	void freeHit(Actor& actor);
	[[nodiscard]] bool isOnCoolDown() const;
	[[nodiscard]] bool inRange(const Actor& target) const;
	[[nodiscard]] bool doesProjectileHit(Attack& attack, const Actor& target) const;
	[[nodiscard]] Percent chanceOfProjectileHit(const Attack& attack, const Actor& target) const;
	[[nodiscard]] const float& getMaxRange() const { return m_maxMeleeRange; }
	[[nodiscard]] uint32_t getCombatScoreForAttack(const Attack& attack) const;
	[[nodiscard]] const Attack& getAttackForCombatScoreDifference(const uint32_t scoreDifference) const;
	[[nodiscard]] float getQualityModifier(uint32_t quality) const;
	[[nodiscard]] bool blockIsValidPosition(const Block& block, uint32_t attackRangeSquared) const;
	[[nodiscard]] bool predicate(const Block& block, Facing facing, uint32_t attackRangeSquared) const;
	AttackType& getRangedAttackType(Item& weapon);
	friend class AttackCoolDown;
	friend class GetIntoAttackPositionThreadedTask;
	// For testing.
	[[maybe_unused, nodiscard]] std::vector<std::pair<uint32_t, Attack>>& getAttackTable() { return m_meleeAttackTable; }
	[[maybe_unused, nodiscard]] bool hasThreadedTask() { return m_getIntoAttackPositionThreadedTask.exists(); }
	[[maybe_unused, nodiscard]] float getCoolDownDurationModifier() { return m_coolDownDurationModifier; }
	[[nodiscard, maybe_unused]] uint32_t getCombatScore() const { return m_combatScore; }
};
class AttackCoolDown final : public ScheduledEventWithPercent
{
	CanFight& m_canFight;
public:
	AttackCoolDown(CanFight& cf, Step duration);
	void execute() { m_canFight.coolDownCompleted(); }
	void clearReferences() { m_canFight.m_coolDown.clearPointer(); }
};
class GetIntoAttackPositionThreadedTask final : public ThreadedTask
{
	Actor& m_actor;
	Actor& m_target;
	uint32_t m_attackRangeSquared;
	FindsPath m_findsPath;
public:
	GetIntoAttackPositionThreadedTask(Actor& a, Actor& t, float ar);
	void readStep();
	void writeStep();
	void clearReferences();
};
