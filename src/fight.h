#include "combatScore.h"
#include "actor.h"
#include "item.h"
#include "body.h"
#include "hit.h"
#include <string>

struct Attack
{
	const AttackType& attackType;
	const MaterialType& materialType;
	Equipment* equipment; // Can be nullptr for natural weapons.
};
class CanFight final
{
	Actor& m_actor;
	uint32_t m_maxRange;
	uint32_t m_coolDownDuration;
	uint32_t m_combatScore;
	HasScheduledEvent<AttackCoolDown> m_coolDown;
	HasThreadedTask<GetIntoAttackPositionThreadedTask> m_getIntoAttackPositionThreadedTask;
	std::vector<std::pair<uint32_t Attack>> m_attackTable;
	Actor* m_target;
	std::unordered_set<Actor*> m_targetBy;
public:
	CanFight(Actor& a) : m_actor(a), m_maxRange(0), m_coolDownDuration(0) { }
	void attack(Actor& target);
	bool coolDown() const;
	bool inRange(Actor& target) const;
	void updateMaxRange();
	void updateCoolDownDuration();
	void updateAttackTable();
	void setTarget(Actor& actor);
	void recordTargetedBy(Actor& actor);
	void removeTargetedBy(Actor& actor);
	void onMoveFrom(Block* previous);
	void onDie();
};
