#pragma once

#include "objective.h"
#include "eventSchedule.hpp"
#include "pathRequest.h"
#include "reservable.h"
#include "terrainFacade.h"
#include "types.h"

#include <memory>
#include <vector>

struct FluidType;
class Plant;
class Item;
class GivePlantsFluidObjective;
struct DeserializationMemo;
struct FindPathResult;

class GivePlantsFluidEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	GivePlantsFluidObjective& m_objective;
public:
	GivePlantsFluidEvent(Step step, Area& area, GivePlantsFluidObjective& gpfo, ActorIndex actor, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	void onCancel(Simulation& simulation, Area* area);
};
// Path to an empty water proof container or somewhere to fill an empty container or a container with the correct type of fluid or a plant which needs fluid.
class GivePlantsFluidPathRequest final : public PathRequest
{
	GivePlantsFluidObjective& m_objective;
public:
	GivePlantsFluidPathRequest(Area& area, GivePlantsFluidObjective& objective);
	void callback(Area& area, FindPathResult& result);
};
class GivePlantsFluidObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Area& area, ActorIndex actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GivePlantsFluid; }
	GivePlantsFluidObjectiveType() = default;
	GivePlantsFluidObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class GivePlantsFluidObjective final : public Objective
{
	BlockIndex m_plantLocation = BLOCK_INDEX_MAX;
	ItemReference m_fluidHaulingItem;
	HasScheduledEvent<GivePlantsFluidEvent> m_event;
public:
	GivePlantsFluidObjective(Area& area);
	GivePlantsFluidObjective(const Json& data, Area& area);
	Json toJson() const;
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void fillContainer(Area& area, BlockIndex fillLocation, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void selectPlantLocation(Area& area, BlockIndex block, ActorIndex actor);
	void selectItem(Area& area, ItemIndex item, ActorIndex actor);
	void reset(Area& area, ActorIndex actor);
	void makePathRequest(Area& area, ActorIndex actor);
	std::string name() const { return "give plants fluid"; }
	bool canFillAt(Area& area, BlockIndex block) const;
	ItemIndex getItemToFillFromAt(Area& area, BlockIndex block);
	bool canGetFluidHaulingItemAt(Area& area, BlockIndex location, ActorIndex actor) const;
	ItemIndex getFluidHaulingItemAt(Area& area, BlockIndex location, ActorIndex actor);
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GivePlantsFluid; }
	friend class GivePlantsFluidEvent;
	friend class GivePlantsFluidPathRequest;
	// For testing.
	BlockIndex getPlantLocation() { return m_plantLocation; }
};
