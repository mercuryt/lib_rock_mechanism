#pragma once
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../uniform.h"
#include "../reference.h"
class Area;
class UniformObjective;
class RTreeBoolean;

class UniformObjective final : public Objective
{
	std::vector<UniformElement> m_elementsCopy;
	ItemReference m_item;
public:
	UniformObjective(Area& area, ActorIndex actor);
	UniformObjective(const Json& data, Area& area, ActorIndex actor, DeserializationMemo& deserializationMemo);
	void execute(Area&, ActorIndex actor);
	void cancel(Area&, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "uniform"; }
	// non virtual.
	void equip(Area& area, const ItemIndex item, ActorIndex actor);
	void select(Area& area, const ItemIndex item);
	Point3D getLocationOfItemInCuboid(Area& area, const Cuboid cuboid) const;
	ItemIndex getItemAtLocation(Area& area, const Cuboid cuboid);
	// For testing.
	[[nodiscard]] ItemReference getItem() { return m_item; }
	friend class UniformThreadedTask;

};
class UniformPathRequest final : public PathRequest
{
	UniformObjective& m_objective;
public:
	UniformPathRequest(Area& area, UniformObjective& objective, ActorIndex actor);
	UniformPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "uniform"; }
};