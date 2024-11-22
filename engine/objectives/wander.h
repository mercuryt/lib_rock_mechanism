

#include "../objective.h"
#include "../terrainFacade.h"
#include "../pathRequest.h"
#include "../types.h"

struct DeserializationMemo;
class Area;
class WanderObjective;

class WanderPathRequest final : public PathRequest
{
	WanderObjective& m_objective;
	BlockIndex m_lastBlock;
	uint16_t m_blockCounter = 0;
public:
	WanderPathRequest(Area& area, WanderObjective& objective, const ActorIndex& actor);
	WanderPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, const FindPathResult& result);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "wander"; }
};
class WanderObjective final : public Objective
{
	BlockIndex m_destination;
public:
	WanderObjective();
	WanderObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	std::string name() const { return "wander"; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canResume() const { return false; }
	// For testing.
	[[nodiscard]] bool hasPathRequest(const Area& area, const ActorIndex& actor) const;
	friend class WanderPathRequest;
};
