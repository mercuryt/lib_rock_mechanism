#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "findsPath.h"
#include "threadedTask.hpp"

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <unordered_set>
#include <utility>

class Actor;
class Item;
class AttackCoolDown;
class GetIntoAttackPositionThreadedTask;
struct MaterialType;
struct AttackType;

struct Attack final
{
	// Use pointers rather then references to allow move.
	const AttackType* attackType;
	const MaterialType* materialType;
	Item* item; // Can be nullptr for natural weapons.
};
class CanFight final
{
	HasScheduledEvent<AttackCoolDown> m_coolDown;
	HasThreadedTask<GetIntoAttackPositionThreadedTask> m_getIntoAttackPositionThreadedTask;
	std::vector<std::pair<uint32_t, Attack>> m_meleeAttackTable;
	//TODO: Should be a flat set.
	std::unordered_set<Actor*> m_targetedBy;
	Actor& m_actor;
	Actor* m_target = nullptr;
	Step m_onMissCoolDownMelee = 0;
	float m_maxMeleeRange = 0;
	float m_maxRange = 0;
	float m_coolDownDurationModifier = 0;
	uint32_t m_combatScore = 0;
public:
	CanFight(Actor& a, Simulation& s);
	CanFight(const Json& data, Actor& a, Simulation& s);
	void attackMeleeRange(Actor& target);
	void attackLongRange(Actor& target, Item* weapon, Item* ammo);
	void coolDownCompleted();
	void update();
	void setTarget(Actor& actor);
	void recordTargetedBy(Actor& actor);
	void removeTargetedBy(Actor& actor);
	void onMoveFrom(BlockIndex previous);
	void onDeath();
	void onLeaveArea();
	void noLongerTargetable();
	void targetNoLongerTargetable();
	void onTargetMoved();
	void freeHit(Actor& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] uint32_t getCurrentMeleeCombatScore();
	[[nodiscard]] bool isOnCoolDown() const;
	[[nodiscard]] bool inRange(const Actor& target) const;
	[[nodiscard]] bool doesProjectileHit(Attack& attack, const Actor& target) const;
	[[nodiscard]] Percent projectileHitPercent(const Attack& attack, const Actor& target) const;
	[[nodiscard]] const float& getMaxRange() const { return m_maxMeleeRange; }
	[[nodiscard]] uint32_t getCombatScoreForAttack(const Attack& attack) const;
	[[nodiscard]] const Attack& getAttackForCombatScoreDifference(const uint32_t scoreDifference) const;
	[[nodiscard]] float getQualityModifier(uint32_t quality) const;
	[[nodiscard]] bool blockIsValidPosition(BlockIndex block, uint32_t attackRangeSquared) const;
	[[nodiscard]] bool predicate(BlockIndex block, Facing facing, uint32_t attackRangeSquared) const;
	AttackType& getRangedAttackType(Item& weapon);
	friend class AttackCoolDown;
	friend class GetIntoAttackPositionThreadedTask;
	// For testing.
	[[maybe_unused, nodiscard]] std::vector<std::pair<uint32_t, Attack>>& getAttackTable() { return m_meleeAttackTable; }
	[[maybe_unused, nodiscard]] bool hasThreadedTask() { return m_getIntoAttackPositionThreadedTask.exists(); }
	[[maybe_unused, nodiscard]] float getCoolDownDurationModifier() { return m_coolDownDurationModifier; }
	[[nodiscard, maybe_unused]] uint32_t getCombatScore() const { return m_combatScore; }
};
class AttackCoolDown final : public ScheduledEvent
{
	CanFight& m_canFight;
public:
	AttackCoolDown(CanFight& cf, Step duration, const Step start = 0);
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
