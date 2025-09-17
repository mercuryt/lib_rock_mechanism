#pragma once
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../uniform.h"
#include "../reference.h"
class Area;
class UniformObjective;

class UniformObjective final : public Objective
{
	std::vector<UniformElement> m_elementsCopy;
	ItemReference m_item;
public:
	UniformObjective(Area& area, const ActorIndex& actor);
	UniformObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	void execute(Area&, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "uniform"; }
	// non virtual.
	void equip(Area& area, const ItemIndex& item, const ActorIndex& actor);
	void select(Area& area, const ItemIndex& item);
	Point3D getLocationOfItemInCuboid(Area& area, const Cuboid& cuboid) const;
	ItemIndex getItemAtLocation(Area& area, const Cuboid& cuboid);
	// For testing.
	[[nodiscard]] ItemReference getItem() { return m_item; }
	friend class UniformThreadedTask;

};
class UniformPathRequest final : public PathRequestBreadthFirst
{
	UniformObjective& m_objective;
public:
	UniformPathRequest(Area& area, UniformObjective& objective, const ActorIndex& actor);
	UniformPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "uniform"; }
};