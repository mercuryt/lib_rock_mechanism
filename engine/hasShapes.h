/*
 * A non virtual base class for shareing code between Plant and CanMove.
 * CanMove is a nonvirtual base class for Actor and Item.
 */
#pragma once

#include "config/config.h"
#include "dataStructures/strongVector.h"
#include "definitions/shape.h"
#include "numericTypes/types.h"
#include "geometry/cuboidSet.h"

struct Shape;
class Area;
struct DeserializationMemo;
class CanReserve;
struct Faction;

template<class Derived, class Index>
class HasShapes
{
protected:
	StrongVector<ShapeId, Index> m_shape;
	// To be used for pathing shapes with other shapes onDeck / mounted.
	StrongVector<ShapeId, Index> m_compoundShape;
	StrongVector<Point3D, Index> m_location;
	StrongVector<Facing4, Index> m_facing;
	StrongVector<FactionId, Index> m_faction;
	StrongVector<CuboidSet, Index> m_occupied;
	StrongVector<MapWithCuboidKeys<CollisionVolume>, Index> m_occupiedWithVolume;
	StrongBitSet<Index> m_static;
	StrongBitSet<Index> m_onSurface;
	//TODO: Do we need m_underground?
	StrongBitSet<Index> m_underground;
	Area& m_area;
	HasShapes(Area& area);
	void create(Index index, const ShapeId shape, const FactionId faction, bool isStatic);
	std::vector<std::pair<int, Index>> getSortOrder(Index begin, Index end);
	void resize(Index newSize);
public:
	template<typename Action>
	void forEachDataHasShapes(Action&& action)
	{
		action(m_shape);
		action(m_compoundShape);
		action(m_location);
		action(m_facing);
		action(m_faction);
		action(m_occupied);
		action(m_occupiedWithVolume);
		action(m_static);
		action(m_underground);
		action(m_onSurface);
	}
	void setStatic(Index index);
	void maybeSetStatic(Index index);
	void unsetStatic(Index index);
	void maybeUnsetStatic(Index index);
	void setShape(Index index, const ShapeId shape);
	void setCompoundShape(Index index, const ShapeId shape) { m_compoundShape[index] = shape; }
	void addShapeToCompoundShape(Index index, const ShapeId id, const Point3D location, const Facing4 facing);
	void removeShapeFromCompoundShape(Index index, const ShapeId id, const Point3D location, const Facing4 facing);
	void log(Index index) const;
	void setOnSurface(Index index, const bool value);
	Index moveTo(HasShapes& other, Index index);
	[[nodiscard]] size_t size() const { return m_shape.size(); }
	[[nodiscard]] bool empty() const { return m_shape.size() == 0; }
	[[nodiscard]] ShapeId getShape(Index index) const { return m_shape[index]; }
	[[nodiscard]] ShapeId getCompoundShape(Index index) const { return m_compoundShape[index]; }
	[[nodiscard]] Point3D getLocation(Index index) const { return m_location[index]; }
	[[nodiscard]] bool hasLocation(Index index) const { return getLocation(index).exists(); }
	[[nodiscard]] Facing4 getFacing(Index index) const { return m_facing[index]; }
	[[nodiscard]] const auto& getOccupied(Index index) const { return m_occupied[index]; }
	[[nodiscard]] const auto& getOccupiedWithVolume(Index index) const { return m_occupiedWithVolume[index]; }
	[[nodiscard]] CuboidSet getCuboidsAbove(Index index) const;
	[[nodiscard]] FactionId getFaction(Index index) const { return m_faction[index]; }
	[[nodiscard]] bool hasFaction(Index index) const { return m_faction[index].exists(); }
	[[nodiscard]] bool isStatic(Index index) const { return m_static[index]; }
	[[nodiscard]] bool isAdjacentToLocation(Index index, const Point3D point) const;
	[[nodiscard]] bool isAdjacentToOrOccupies(Index index, const Point3D point) const;
	//TODO: change these into templates?
	[[nodiscard]] bool predicateForAnyOccupiedCuboid(Index index, std::function<bool(const Cuboid)> predicate) const;
	[[nodiscard]] bool predicateForAnyAdjacentCuboid(Index index, std::function<bool(const Cuboid)> predicate) const;
	[[nodiscard]] bool predicateForAnyOccupiedCuboidAtLocationAndFacing(Index index, std::function<bool(const Cuboid)> predicate, const Point3D location, const Facing4 facing) const;
	[[nodiscard]] CuboidSet getAdjacentCuboids(Index index) const;
	[[nodiscard]] CuboidSet getOccupiedAndAdjacentCuboids(Index index) const;
	[[nodiscard]] SmallSet<ItemIndex> getAdjacentItems(Index index) const;
	[[nodiscard]] SmallSet<ActorIndex> getAdjacentActors(Index index) const;
	[[nodiscard]] CuboidSet getAdjacentCuboidsAtLocationWithFacing(Index index, const Point3D point, const Facing4 facing) const;
	[[nodiscard]] CuboidSet getCuboidsWhichWouldBeOccupiedAtLocationAndFacing(Index index, const Point3D location, const Facing4 facing) const;
	[[nodiscard]] bool allPointsAtLocationAndFacingAreReservable(Index index, const Point3D location, const Facing4 facing, const FactionId faction) const;
	[[nodiscard]] bool allOccupiedPointsAreReservable(Index index, const FactionId faction) const;
	[[nodiscard]] bool isAdjacentToActor(Index index, ActorIndex actor) const;
	[[nodiscard]] bool isAdjacentToItem(Index index, const ItemIndex actor) const;
	[[nodiscard]] bool isAdjacentToPlant(Index index, const PlantIndex plant) const;
	[[nodiscard]] bool isAdjacentToActorAt(Index index, const Point3D location, const Facing4 facing, ActorIndex actor) const;
	[[nodiscard]] bool isAdjacentToItemAt(Index index, const Point3D location, const Facing4 facing, const ItemIndex item) const;
	[[nodiscard]] bool isAdjacentToPlantAt(Index index, const Point3D location, const Facing4 facing, const PlantIndex plant) const;
	[[nodiscard]] bool isOnEdgeAt(Index index, const Point3D location, const Facing4 facing) const;
	[[nodiscard]] bool isOnEdge(Index index) const;
	[[nodiscard]] bool isOnSurface(Index index) const;
	[[nodiscard]] bool isIntersectingOrAdjacentTo(Index index, const CuboidSet& cuboids) const;
	[[nodiscard]] bool isIntersectingOrAdjacentTo(Index index, ActorIndex actor) const;
	[[nodiscard]] bool isIntersectingOrAdjacentTo(Index index, const ItemIndex item) const;
	[[nodiscard]] const auto& getOnSurface() const { return m_onSurface; }
	[[nodiscard]] Distance distanceToActor(Index index, ActorIndex actor) const;
	[[nodiscard]] Distance distanceToItem(Index index, const ItemIndex item) const;
	[[nodiscard]] DistanceFractional distanceToActorFractional(Index index, ActorIndex actor) const;
	[[nodiscard]] DistanceFractional distanceToItemFractional(Index index, const ItemIndex item) const;
	[[nodiscard]] Point3D getPointWhichIsAdjacentAtLocationWithFacingAndPredicate(Index index, const Point3D location, const Facing4 facing, std::function<bool(const Point3D)>& predicate) const;
	[[nodiscard]] Point3D getPointWhichIsOccupiedAtLocationWithFacingAndPredicate(Index index, const Point3D location, const Facing4 facing, std::function<bool(const Point3D)>& predicate) const;
	[[nodiscard]] Point3D getPointWhichIsAdjacentWithPredicate(Index index, std::function<bool(const Point3D)>& predicate) const;
	[[nodiscard]] Point3D getPointWhichIsOccupiedWithPredicate(Index index, std::function<bool(const Point3D)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(Index index, const Point3D location, const Facing4 facing, std::function<bool(const ItemIndex)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentWithPredicate(Index index, std::function<bool(const ItemIndex)>& predicate) const;
	template<class PointCollection>
	bool isAdjacentToAny(Index index, const PointCollection& locations) const
	{
		const auto predicate = [&](const Cuboid cuboid) { return cuboid.containsAnyPoints(locations); };
		return predicateForAnyAdjacentCuboid(index, predicate);
	}
	bool isAdjacentToAnyCuboid(Index index, const CuboidSet& cuboids) const;
	[[nodiscard]] Cuboid boundry(Index index) const { return m_occupied[index].boundry(); }
	[[nodiscard]] Area& getArea() { return m_area; }
	[[nodiscard]] const Area& getArea() const { return m_area; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(HasShapes, m_shape, m_location, m_facing, m_faction, m_occupied, m_static, m_underground);
};
