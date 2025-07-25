/*
 * A non virtual base class for shareing code between Plant and CanMove.
 * CanMove is a nonvirtual base class for Actor and Item.
 */
#pragma once

#include "config.h"
#include "dataStructures/strongVector.h"
#include "definitions/shape.h"
#include "numericTypes/types.h"
#include "hasShapeTypes.h"

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
	StrongVector<OccupiedSpaceForHasShape, Index> m_occupied;
	StrongBitSet<Index> m_static;
	StrongBitSet<Index> m_onSurface;
	//TODO: Do we need m_underground?
	StrongBitSet<Index> m_underground;
	Area& m_area;
	HasShapes(Area& area);
	void create(const Index& index, const ShapeId& shape, const FactionId& faction, bool isStatic);
	std::vector<std::pair<uint32_t, Index>> getSortOrder(const Index& begin, const Index& end);
	void resize(const Index& newSize);
public:
	template<typename Action>
	void forEachDataHasShapes(Action&& action);
	void setStatic(const Index& index);
	void maybeSetStatic(const Index& index);
	void unsetStatic(const Index& index);
	void maybeUnsetStatic(const Index& index);
	void setShape(const Index& index, const ShapeId& shape);
	void setCompoundShape(const Index& index, const ShapeId& shape) { m_compoundShape[index] = shape; }
	void addShapeToCompoundShape(const Index& index, const ShapeId& id, const Point3D& location, const Facing4& facing);
	void removeShapeFromCompoundShape(const Index& index, const ShapeId& id, const Point3D& location, const Facing4& facing);
	void log(const Index& index) const;
	[[nodiscard]] size_t size() const { return m_shape.size(); }
	[[nodiscard]] ShapeId getShape(const Index& index) const { return m_shape[index]; }
	[[nodiscard]] ShapeId getCompoundShape(const Index& index) const { return m_compoundShape[index]; }
	[[nodiscard]] Point3D getLocation(const Index& index) const { return m_location[index]; }
	[[nodiscard]] bool hasLocation(const Index& index) const { return getLocation(index).exists(); }
	[[nodiscard]] Facing4 getFacing(const Index& index) const { return m_facing[index]; }
	[[nodiscard]] const auto& getOccupied(const Index& index) const { return m_occupied[index]; }
	[[nodiscard]] const SmallSet<Point3D> getPointsAbove(const Index& index) const;
	[[nodiscard]] FactionId getFaction(const Index& index) const { return m_faction[index]; }
	[[nodiscard]] bool hasFaction(const Index& index) const { return m_faction[index].exists(); }
	[[nodiscard]] bool isStatic(const Index& index) const { return m_static[index]; }
	[[nodiscard]] bool isAdjacentToLocation(const Index& index, const Point3D& point) const;
	template<class Points>
	[[nodiscard]] bool isAdjacentToAny(const Index& index, const Points& point) const;
	[[nodiscard]] bool predicateForAnyOccupiedPoint(const Index& index, std::function<bool(const Point3D&)> predicate) const;
	[[nodiscard]] bool predicateForAnyAdjacentPoint(const Index& index, std::function<bool(const Point3D&)> predicate) const;
	[[nodiscard]] bool predicateForAnyOccupiedPointAtLocationAndFacing(const Index& index, std::function<bool(const Point3D&)> predicate, const Point3D& location, const Facing4& facing) const;
	[[nodiscard]] SmallSet<Point3D> getAdjacentPoints(const Index& index) const;
	[[nodiscard]] SmallSet<Point3D> getOccupiedAndAdjacentPoints(const Index& index) const;
	[[nodiscard]] SmallSet<ItemIndex> getAdjacentItems(const Index& index) const;
	[[nodiscard]] SmallSet<ActorIndex> getAdjacentActors(const Index& index) const;
	[[nodiscard]] SmallSet<Point3D> getAdjacentPointsAtLocationWithFacing(const Index& index, const Point3D& point, const Facing4& facing) const;
	[[nodiscard]] SmallSet<Point3D> getPointsWhichWouldBeOccupiedAtLocationAndFacing(const Index& index, const Point3D& location, const Facing4& facing) const;
	[[nodiscard]] bool allPointsAtLocationAndFacingAreReservable(const Index& index, const Point3D& location, const Facing4& facing, const FactionId& faction) const;
	[[nodiscard]] bool allOccupiedPointsAreReservable(const Index& index, const FactionId& faction) const;
	[[nodiscard]] bool isAdjacentToActor(const Index& index, const ActorIndex& actor) const;
	[[nodiscard]] bool isAdjacentToItem(const Index& index, const ItemIndex& actor) const;
	[[nodiscard]] bool isAdjacentToPlant(const Index& index, const PlantIndex& plant) const;
	[[nodiscard]] bool isAdjacentToActorAt(const Index& index, const Point3D& location, const Facing4& facing, const ActorIndex& actor) const;
	[[nodiscard]] bool isAdjacentToItemAt(const Index& index, const Point3D& location, const Facing4& facing, const ItemIndex& item) const;
	[[nodiscard]] bool isAdjacentToPlantAt(const Index& index, const Point3D& location, const Facing4& facing, const PlantIndex& plant) const;
	[[nodiscard]] bool isOnEdgeAt(const Index& index, const Point3D& location, const Facing4& facing) const;
	[[nodiscard]] bool isOnEdge(const Index& index) const;
	[[nodiscard]] bool isOnSurface(const Index& index) const;
	[[nodiscard]] const auto& getOnSurface() const { return m_onSurface; }
	[[nodiscard]] Distance distanceToActor(const Index& index, const ActorIndex& actor) const;
	[[nodiscard]] Distance distanceToItem(const Index& index, const ItemIndex& item) const;
	[[nodiscard]] DistanceFractional distanceToActorFractional(const Index& index, const ActorIndex& actor) const;
	[[nodiscard]] DistanceFractional distanceToItemFractional(const Index& index, const ItemIndex& item) const;
	[[nodiscard]] Point3D getPointWhichIsAdjacentAtLocationWithFacingAndPredicate(const Index& index, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)>& predicate) const;
	[[nodiscard]] Point3D getPointWhichIsOccupiedAtLocationWithFacingAndPredicate(const Index& index, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)>& predicate) const;
	[[nodiscard]] Point3D getPointWhichIsAdjacentWithPredicate(const Index& index, std::function<bool(const Point3D&)>& predicate) const;
	[[nodiscard]] Point3D getPointWhichIsOccupiedWithPredicate(const Index& index, std::function<bool(const Point3D&)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const Index& index, const Point3D& location, const Facing4& facing, std::function<bool(const ItemIndex&)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentWithPredicate(const Index& index, std::function<bool(const ItemIndex&)>& predicate) const;
	[[nodiscard]] Area& getArea() { return m_area; }
	[[nodiscard]] const Area& getArea() const { return m_area; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(HasShapes, m_shape, m_location, m_facing, m_faction, m_occupied, m_static, m_underground);
};
