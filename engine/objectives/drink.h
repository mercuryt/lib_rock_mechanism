#pragma once
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../numericTypes/types.h"
class Area;
class DrinkEvent;
class DrinkObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, const ActorIndex&) const { std::unreachable(); std::unreachable(); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, const ActorIndex&) const { std::unreachable(); std::unreachable(); }
	DrinkObjectiveType() = default;
	DrinkObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const override { return "drink"; }
};
class DrinkObjective final : public Objective
{
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
	bool m_noDrinkFound = false;
public:
	DrinkObjective(Area& area);
	DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const ActorIndex& actor);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	void makePathRequest(Area& area, const ActorIndex& actor);
	[[nodiscard]] ObjectiveTypeId getTypeId() const override { return ObjectiveType::getByName("drink").getId(); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const override { return "drink"; }
	[[nodiscard]] bool canDrinkAt(Area& area, const Point3D& point, const Facing4& facing, const ActorIndex& actor) const;
	[[nodiscard]] Point3D getAdjacentPointToDrinkAt(Area& area, const Point3D& point, const Facing4& facing, const ActorIndex& actor) const;
	[[nodiscard]] Point3D getPointToDrinkItemAt(Area& area, const Cuboid& cuboid, const ActorIndex& actor) const;
	[[nodiscard]] ItemIndex getItemToDrinkFromAt(Area& area, const Cuboid& cuboid, const ActorIndex& actor) const;
	[[nodiscard]] ItemIndex getItemToDrinkFromAt(Area& area, const Point3D& point, const ActorIndex& actor) const { return getItemToDrinkFromAt(area, {point, point}, actor); }
	[[nodiscard]] Point3D containsSomethingDrinkable(Area& area, const Cuboid& cuboid, const ActorIndex& actor) const;
	[[nodiscard]] bool isNeed() const { return true; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::drink; }
	friend class DrinkEvent;
	friend class DrinkPathRequest;
};
class DrinkPathRequest final : public PathRequestBreadthFirst
{
	DrinkObjective& m_drinkObjective;
public:
	DrinkPathRequest(Area& area, DrinkObjective& drob, const ActorIndex& actor);
	DrinkPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() { return "drink"; }
};
