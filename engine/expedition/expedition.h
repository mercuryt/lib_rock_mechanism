#pragma once
#include "../geometry/point3D.h"
#include "../dataStructures/smallSet.h"
#include "../dataStructures/smallMap.h"
#include "../dataStructures/rtreeBoolean.h"
#include "../numericTypes/idTypes.h"
#include "../numericTypes/index.h"
#include "../numericTypes/types.h"

class Simulation;
class World;
class Area;
struct ActorParamaters;

enum class ExpeditionType
{
	Hunt,
	Gather,
	Conquest,
	Recon,
	Trade,
	Migration,
	Pilgramage
};

struct Expedition
{
	// Area is used to store the actors and items in the expedition as well as their events but Space and all locations are left undefined.
	// TODO: Area's climate must be updated when expedition's location changes?
	// TODO: Store passenger and pilot vehile assignments for temporary stops.
	// The next step in the path is always the back of the vector.
	std::vector<Point3D> path = {};
	SmallSet<ActorId> actors = {};
	SmallSet<ActorId> waitingFor = {};
	std::string name;
	Step nextMove = {};
	Point3D location;
	Point3D destinationArea = {};
	AreaId area;
	FactionId faction;
	ExpeditionId id;
	Priority priority;
	MoveTypeId moveType = {};
	Speed moveSpeed = {};
	ExpeditionType type;
	bool enterDestination = false;
	void updateMoveTypeAndSpeed(Simulation& simulation);
	void setExpeditionIdForAllActors(Simulation& simulation);
	ActorIndex createActor(Simulation& simulation, const ActorParamaters& params);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Expedition, path, actors, waitingFor, name, nextMove, location, destinationArea, area, faction, id, priority, moveType, moveSpeed, type);
};

NLOHMANN_JSON_SERIALIZE_ENUM(ExpeditionType, {
	{ExpeditionType::Hunt, "Hunt"},
	{ExpeditionType::Gather, "Gather"},
	{ExpeditionType::Conquest,"Conquest"},
	{ExpeditionType::Recon, "Recon"},
	{ExpeditionType::Trade, "Trade"},
	{ExpeditionType::Migration, "Migration"},
	{ExpeditionType::Pilgramage, "Pilgramage"}
})
class ExpeditionsHaveScheduledMovements
{
	std::vector<std::pair<Step, std::vector<ExpeditionId>>> m_data;
public:
	void schedule(ExpeditionId id, Step step);
	void unschedule(ExpeditionId id, Step step);
	// Return ids for expeditions scheduled to move. Does not move them directly.
	std::vector<ExpeditionId> doStep(Step step);
	[[nodiscard]] bool empty() const;
	[[nodiscard]] Step getNextStep() const;
};

class ExpeditionsHaveArrivals
{
	struct ArrivalData
	{
		Point3D center;
		ExpeditionId expedition;
		AreaId area;
		bool operator==(const ArrivalData&) const = default;
	};
	SmallSet<ArrivalData> m_data;
public:
	// An expedition cannot enter an area from it's current location if there is no where on the prospective entry face to place it.
	// If a point is found which can accomidate all shapes in the expedition return it.
	[[nodiscard]] Point3D findEntryPoint(Area& area, Expedition& expedition);
	// Begin arrival.
	void arrive(Area& arrivingAt, Expedition& expedition);
	// Continue arrival processes.
	void doStep(Simulation& simulation,	Step step);
	void doStepArrival(Area& arrivingAt, Expedition& expedition, Point3D center);
};
class SimulationHasExpeditions
{
	ExpeditionsHaveArrivals m_arrivals;
	ExpeditionsHaveScheduledMovements m_moveSchedule;
	std::vector<Expedition> m_expeditions;
	SmallMap<MoveTypeId, RTreeBoolean> m_enterables;
	ExpeditionId m_nextId{0};
public:
	void doStep(Simulation& simulation, Step step);
	void setLocation(Expedition& expedition, Simulation& simulation, Point3D location);
	void scheduleMove(Simulation& simulation, Expedition& expedition);
	ExpeditionId create(Simulation& simulation, FactionId faction, Priority priority, Point3D location, ExpeditionType type, std::string name);
	void add(Area& area, ActorIndex actor, ExpeditionId expedition);
	void departAreaToJoinExpedition(Area& area, ActorIndex actor, Expedition& expedition);
	void regroup(Simulation& simulation, Expedition& expedition);
	void clearDestination(ExpeditionId id);
	void remove(Simulation& simulation, ExpeditionId expedition, ActorId id);
	[[nodiscard]] bool setDestination(Simulation& simulation, ExpeditionId id, Point3D location, bool enter);
	[[nodiscard]] Step getNextStep() const;
	[[nodiscard]] Expedition& byId(ExpeditionId id);
	[[nodiscard]] const Expedition& byId(ExpeditionId id) const;
	[[nodiscard]] const RTreeBoolean& getEnterable(World& world, MoveTypeId moveType);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasExpeditions, m_expeditions);
};