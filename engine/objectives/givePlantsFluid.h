#pragma once

#include "../objective.h"
#include "../eventSchedule.hpp"
#include "../pathRequest.h"
#include "../reservable.h"
#include "../terrainFacade.h"
#include "../types.h"

#include <memory>

class GivePlantsFluidObjective;
struct DeserializationMemo;
struct FindPathResult;

class GivePlantsFluidEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	GivePlantsFluidObjective& m_objective;
public:
	GivePlantsFluidEvent(const Step& step, Area& area, GivePlantsFluidObjective& gpfo, const ActorIndex& actor, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	void onCancel(Simulation& simulation, Area* area);
};
// Path to an empty water proof container or somewhere to fill an empty container or a container with the correct type of fluid or a plant which needs fluid.
class GivePlantsFluidPathRequest final : public PathRequest
{
	GivePlantsFluidObjective& m_objective;
public:
	GivePlantsFluidPathRequest(Area& area, GivePlantsFluidObjective& objective, const ActorIndex& actor);
	GivePlantsFluidPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, const FindPathResult& result);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "give plants fluid"; }
};
class GivePlantsFluidObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	GivePlantsFluidObjectiveType() = default;
	GivePlantsFluidObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
	[[nodiscard]] std::string name() const { return "give plants fluid"; }
};
class GivePlantsFluidObjective final : public Objective
{
	BlockIndex m_plantLocation;
	ItemReference m_fluidHaulingItem;
	HasScheduledEvent<GivePlantsFluidEvent> m_event;
public:
	GivePlantsFluidObjective(Area& area);
	GivePlantsFluidObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void fillContainer(Area& area, const BlockIndex& fillLocation, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void selectPlantLocation(Area& area, const BlockIndex& block, const ActorIndex& actor);
	void selectItem(Area& area, const ItemIndex& item, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	void makePathRequest(Area& area, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "give plants fluid"; }
	[[nodiscard]] bool canFillAt(Area& area, const BlockIndex& block) const;
	[[nodiscard]] ItemIndex getItemToFillFromAt(Area& area, const BlockIndex& block) const;
	[[nodiscard]] bool canGetFluidHaulingItemAt(Area& area, const BlockIndex& location, const ActorIndex& actor) const;
	[[nodiscard]] ItemIndex getFluidHaulingItemAt(Area& area, const BlockIndex& location, const ActorIndex& actor) const;
	friend class GivePlantsFluidEvent;
	friend class GivePlantsFluidPathRequest;
	// For testing.
	[[nodiscard]] BlockIndex getPlantLocation() const { return m_plantLocation; }
	[[nodiscard]] bool itemExists() const { return m_fluidHaulingItem.exists(); }
	[[nodiscard]] ItemIndex getItem() const { return m_fluidHaulingItem.getIndex(); }
};
