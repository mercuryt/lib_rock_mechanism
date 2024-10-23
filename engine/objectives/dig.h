#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
#include "../config.h"
#include "callbackTypes.h"
#include "deserializationMemo.h"
#include "vectorContainers.h"
class Area;
class DigProject;
class DigObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	DigObjectiveType() = default;
	DigObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
	[[nodiscard]] std::string name() const { return "dig"; }
};
class DigObjective final : public Objective
{
	DigProject* m_project = nullptr;
	SmallSet<DigProject*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	DigObjective() : Objective(Config::digObjectivePriority) { }
	DigObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	void onProjectCannotReserve(Area& area, const ActorIndex& actor);
	void joinProject(DigProject& project, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] DigProject* getJoinableProjectAt(Area& area, BlockIndex block, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "dig"; }
	friend class DigPathRequest;
	friend class DigProject;
	//For testing.
	[[nodiscard]] DigProject* getProject() { return m_project; }
};
// Find a place to dig.
class DigPathRequest final : public PathRequest
{
	DigObjective& m_digObjective;
	// Result is the block which will be the actors location while doing the digging.
public:
	DigPathRequest(Area& area, DigObjective& digObjective, const ActorIndex& actor);
	DigPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, const FindPathResult& result);
	void clearReferences(Simulation&, Area* area);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() { return "dig"; }
};
