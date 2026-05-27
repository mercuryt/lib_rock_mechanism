#pragma once
#include "../objective.h"
#include "../path/areaHasPaths.h"
#include "../path/pathRequest.h"
#include "../numericTypes/types.h"

struct DeserializationMemo;
class Area;
class WanderObjective;

class WanderPathRequest final : public PathRequest
{
	WanderObjective& m_objective;
	Point3D m_lastPoint;
	int m_pointCounter = 0;
public:
	WanderPathRequest(Area& area, WanderObjective& objective, const ActorIndex actor);
	WanderPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "wander"; }
};
class WanderObjective final : public Objective
{
	Point3D m_destination;
public:
	WanderObjective();
	WanderObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex actor);
	void cancel(Area& area, const ActorIndex actor);
	void delay(Area& area, const ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex actor);
	std::string name() const { return "wander"; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canResume() const { return false; }
	// For testing.
	[[nodiscard]] bool hasPathRequest(const Area& area, const ActorIndex actor) const;
	friend class WanderPathRequest;
};
