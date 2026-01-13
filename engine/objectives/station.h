#pragma once
#include "objective.h"
#include "config/config.h"
#include "numericTypes/types.h"
struct DeserializationMemo;
class StationObjective final : public Objective
{
	Point3D m_location;
public:
	StationObjective(const Point3D& l, const Priority& priority = Config::stationPriority) : Objective(priority), m_location(l) { }
	StationObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex&) { }
	void delay(Area&, const ActorIndex&) { }
	void reset(Area& area, const ActorIndex& actor);
	std::string name() const { return "station"; }
};
/*
class StationInputAction final : public InputAction
{
public:
	Point3D& m_point;
	StationInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, Point3D& b);
	void execute();
};
*/
