#include "../types.h"
#include "../config.h"
#include <unordered_set>
class Actor;
class ActorCanSee final
{
	Actor& m_actor;
	std::unordered_set<Actor*> m_currently;
	DistanceInBlocks m_range;
public:
	ActorCanSee(Actor& actor, DistanceInBlocks range) : m_actor(actor), m_range(range) { }
	ActorCanSee(const Json& data, Actor& actor) : m_actor(actor), m_range(data["range"].get<DistanceInBlocks>()) { }
	[[nodiscard]] Json toJson() const;
	void doVision(std::unordered_set<Actor*>& actors);
	void setVisionRange(DistanceInBlocks range) { m_range = range; }
	[[nodiscard]] std::unordered_set<Actor*>& getCurrentlyVisibleActors() { return m_currently; }
	[[nodiscard]] DistanceInBlocks getRange() const { return m_range; }
	[[nodiscard]] bool isCurrentlyVisible(Actor& actor) const { return m_currently.contains(&actor); }
};