#pragma once
#include "../pathRequest.h"
#include "../objective.h"
#include "../types.h"
class Area;
class InstallItemObjective;
class InstallItemProject;
// Search for nearby item that needs installing.
class InstallItemPathRequest final : public PathRequest
{
	InstallItemObjective& m_installItemObjective;
public:
	InstallItemPathRequest(Area& area, InstallItemObjective& iio);
	void callback(Area& area, FindPathResult& result);
};
class InstallItemObjective final : public Objective
{
	InstallItemProject* m_project;
public:
	InstallItemObjective();
	InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, ActorIndex actor); 
	void cancel(Area& area, ActorIndex actor);
	void delay(Area&, ActorIndex) { }
	void reset(Area& area, ActorIndex actor);
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::InstallItem; }
	[[nodiscard]] std::string name() const { return "install item"; }
	[[nodiscard]] Json toJson() const;
	friend class InstallItemPathRequest;
};
