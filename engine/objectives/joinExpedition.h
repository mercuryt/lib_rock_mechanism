#pragma once
#include "../config/config.h"
#include "../objective.h"
#include "../path/pathRequest.h"
struct DeserializationMemo;
class JoinExpeditionPathRequest;
struct PathResult;
class JoinExpeditionObjective final : public Objective
{
	ExpeditionId m_expedition;
public:
	JoinExpeditionObjective(Priority priority, ExpeditionId expedition);
	// No need to overide default to/from json.
	void execute(Area&, const ActorIndex);
	void cancel(Area&, const ActorIndex) { }
	void delay(Area&, const ActorIndex) { }
	void reset(Area&, const ActorIndex) { }
	[[nodiscard]] std::string name() const { return "join expedition"; }
	friend class JoinExpeditionPathRequest;
};
class JoinExpeditionPathRequest final : public PathRequest
{
	JoinExpeditionObjective& m_objective;
public:
	JoinExpeditionPathRequest(Area& area, JoinExpeditionObjective& objective, ActorIndex actor);
	JoinExpeditionPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	[[nodiscard]] std::string name() const { return "join expedition"; }
	[[nodiscard]] Json toJson() const;
};
