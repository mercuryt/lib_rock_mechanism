#include "hasShapes.h"
#include "actors/actors.h"
#include "area/area.h"
#include "space/space.h"
#include "numericTypes/index.h"
#include "items/items.h"
#include "plants.h"
#include "simulation/simulation.h"
#include "numericTypes/types.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <ranges>
template<class Derived, class Index>
HasShapes<Derived, Index>::HasShapes(Area& area) : m_area(area) { }
template<class Derived, class Index>
template<typename Action>
void HasShapes<Derived, Index>::forEachDataHasShapes(Action&& action)
{
	action(m_shape);
	action(m_compoundShape);
	action(m_location);
	action(m_facing);
	action(m_faction);
	action(m_occupied);
	action(m_static);
	action(m_underground);
	action(m_onSurface);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::create(const Index& index, const ShapeId& shape, const FactionId& faction, bool isStatic)
{
	assert(m_shape.size() > index);
	// m_location, m_facing, and m_underground are to be set by calling the derived class location_set method.
	m_shape[index] = shape;
	m_compoundShape[index] = shape;
	m_faction[index] = faction;
	assert(m_occupied[index].empty());
	m_static.set(index, isStatic);
	m_onSurface.maybeUnset(index);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::resize(const Index& newSize)
{
	static_cast<Derived&>(*this).forEachData([&newSize](auto& data) { data.resize(newSize); });
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::setStatic(const Index& index)
{
	assert(!m_static[index]);
	if(hasLocation(index))
	{
		Space& space = m_area.getSpace();
		for(const auto& pair : Shape::positionsWithFacing(m_shape[index], m_facing[index]))
		{
			Point3D point = m_location[index].applyOffset(pair.offset);
			space.shape_addStaticVolume(point, pair.volume);
			space.shape_removeDynamicVolume(point, pair.volume);
		}
	}
	m_static.set(index, true);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::maybeSetStatic(const Index& index)
{
	if(!m_static[index])
		static_cast<Derived*>(this)->setStatic(index);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::unsetStatic(const Index& index)
{
	assert(m_static[index]);
	Space& space = m_area.getSpace();
	for(const auto& pair : Shape::positionsWithFacing(m_shape[index], m_facing[index]))
	{
		Point3D point = m_location[index].applyOffset(pair.offset);
		space.shape_removeStaticVolume(point, pair.volume);
		space.shape_addDynamicVolume(point, pair.volume);
	}
	m_static.set(index, false);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::maybeUnsetStatic(const Index& index)
{
	if(m_static[index])
		static_cast<Derived*>(this)->unsetStatic(index);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::setShape(const Index& index, const ShapeId& shape)
{
	Point3D location = m_location[index];
	Facing4 facing = m_facing[index];
	if(location.exists())
		static_cast<Derived*>(this)->location_clear(index);
	m_shape[index] = shape;
	if(location.exists())
		static_cast<Derived*>(this)->location_set(index, location, facing);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::addShapeToCompoundShape(const Index& index, const ShapeId& id, const Point3D& location, const Facing4& facing)
{
	auto offsetsAndVolumes = Shape::positionsWithFacing(id, facing);
	Offset3D offsetToRiderFromMount = getLocation(index).offsetTo(location);
	for(OffsetAndVolume& offsetAndVolume : offsetsAndVolumes)
		offsetAndVolume.offset += offsetToRiderFromMount;
	m_compoundShape[index] = Shape::mutateAddMultiple(m_compoundShape[index], offsetsAndVolumes);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::removeShapeFromCompoundShape(const Index& index, const ShapeId& id, const Point3D& location, const Facing4& facing)
{
	auto offsetsAndVolumes = Shape::positionsWithFacing(id, facing);
	Offset3D offsetToRiderFromMount = getLocation(index).offsetTo(location);
	for(OffsetAndVolume offsetAndVolume : offsetsAndVolumes)
		offsetAndVolume.offset += offsetToRiderFromMount;
	m_compoundShape[index] = Shape::mutateRemoveMultiple(m_compoundShape[index], offsetsAndVolumes);
}
template<class Derived, class Index>
std::vector<std::pair<uint32_t, Index>> HasShapes<Derived, Index>::getSortOrder(const Index& begin, const Index& end)
{
	assert(end > begin + 1);
	std::vector<std::pair<uint32_t, Index>> sortOrder;
	sortOrder.reserve((end - begin).get());
	for(Index index = begin; index < end; ++index)
		sortOrder.emplace_back(m_location[index].hilbertNumber(), index);
	std::ranges::sort(sortOrder, std::less{}, &std::pair<uint32_t, Index>::first);
	return sortOrder;
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToActor(const Index& index, const ActorIndex& actor) const
{
	return isAdjacentToActorAt(index, m_location[index], m_facing[index], actor);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToItem(const Index& index, const ItemIndex& item) const
{
	return isAdjacentToItemAt(index, m_location[index], m_facing[index], item);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToPlant(const Index& index, const PlantIndex& plant) const
{
	return isAdjacentToPlantAt(index, m_location[index], m_facing[index], plant);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToActorAt(const Index& index, const Point3D& location, const Facing4& facing, const ActorIndex& actor) const
{
	auto occupied = getPointsWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(Point3D)> predicate = [&](Point3D point) { return occupied.contains(point); };
	return m_area.getActors().predicateForAnyAdjacentPoint(actor, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToItemAt(const Index& index, const Point3D& location, const Facing4& facing, const ItemIndex& item) const
{
	auto occupied = getPointsWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(Point3D)> predicate = [&](Point3D point) { return occupied.contains(point); };
	return m_area.getItems().predicateForAnyAdjacentPoint(item, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToPlantAt(const Index& index, const Point3D& location, const Facing4& facing, const PlantIndex& plant) const
{
	auto occupied = getPointsWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(Point3D)> predicate = [&](Point3D point) { return occupied.contains(point); };
	return m_area.getPlants().predicateForAnyAdjacentPoint(plant, predicate);
}
template<class Derived, class Index>
Distance HasShapes<Derived, Index>::distanceToActor(const Index& index, const ActorIndex& actor) const
{
	// TODO: Make handle multi point creatures correctly somehow.
	// Use line of sight?
	return m_location[index].distanceTo(m_area.getActors().getLocation(actor));
}
template<class Derived, class Index>
Distance HasShapes<Derived, Index>::distanceToItem(const Index& index, const ItemIndex& item) const
{
	// TODO: Make handle multi point creatures correctly somehow.
	// Use line of sight?
	return m_location[index].distanceTo(m_area.getItems().getLocation(item));
}
template<class Derived, class Index>
DistanceFractional HasShapes<Derived, Index>::distanceToActorFractional(const Index& index, const ActorIndex& actor) const
{
	return m_location[index].distanceToFractional(m_area.getActors().getLocation(actor));
}
template<class Derived, class Index>
DistanceFractional HasShapes<Derived, Index>::distanceToItemFractional(const Index& index, const ItemIndex& item) const
{
	return m_location[index].distanceToFractional(m_area.getItems().getLocation(item));
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::allOccupiedPointsAreReservable(const Index& index, const FactionId& faction) const
{
	return allPointsAtLocationAndFacingAreReservable(index, m_location[index], m_facing[index], faction);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToLocation(const Index& index, const Point3D& location) const
{
	std::function<bool(const Point3D)> predicate = [&](const Point3D point) { return point == location; };
	return predicateForAnyAdjacentPoint(index, predicate);
}
template<class Derived, class Index>
template<class PointCollection>
bool HasShapes<Derived, Index>::isAdjacentToAny(const Index& index, const PointCollection& locations) const
{
	std::function<bool(const Point3D)> predicate = [&](const Point3D point) { return locations.contains(point); };
	return predicateForAnyAdjacentPoint(index, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isOnEdgeAt(const Index& index, const Point3D& location, const Facing4& facing) const
{
	Space& space = m_area.getSpace();
	std::function<bool(const Point3D)> predicate = [&space](const Point3D point) { return space.isEdge(point); };
	return predicateForAnyOccupiedPointAtLocationAndFacing(index, predicate, location, facing);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isOnEdge(const Index& index) const
{
	Space& space = m_area.getSpace();
	std::function<bool(const Point3D)> predicate = [&space](const Point3D point) { return space.isEdge(point); };
	return predicateForAnyOccupiedPoint(index, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isOnSurface(const Index& index) const { return m_onSurface[index]; }
template<class Derived, class Index>
bool HasShapes<Derived, Index>::predicateForAnyOccupiedPoint(const Index& index, std::function<bool(const Point3D&)> predicate) const
{
	assert(!m_occupied[index].empty());
	for(Point3D point : m_occupied[index])
		if(predicate(point))
			return true;
	return false;
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::predicateForAnyOccupiedPointAtLocationAndFacing(const Index& index, std::function<bool(const Point3D&)> predicate, const Point3D& location, const Facing4& facing) const
{
	for(Point3D occupied : const_cast<HasShapes&>(*this).getPointsWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		if(predicate(occupied))
			return true;
	return false;
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::predicateForAnyAdjacentPoint(const Index& index, std::function<bool(const Point3D&)> predicate) const
{
	assert(m_location[index].exists());
	//TODO: cache this.
	for(const auto& offset : Shape::adjacentPositionsWithFacing(m_shape[index], m_facing[index]))
	{
		Point3D point = m_location[index].applyOffset(offset);
		if(point.exists() && predicate(point))
			return true;
	}
	return false;
}
template<class Derived, class Index>
SmallSet<Point3D> HasShapes<Derived, Index>::getAdjacentPoints(const Index& index) const
{
	SmallSet<Point3D> output;
	for(Point3D point : m_occupied[index])
		for(Point3D adjacent : m_area.getSpace().getAdjacentWithEdgeAndCornerAdjacent(point))
			if(!m_occupied[index].contains(adjacent))
				output.insertNonunique(adjacent);
	output.makeUnique();
	return output;
}
template<class Derived, class Index>
SmallSet<ActorIndex> HasShapes<Derived, Index>::getAdjacentActors(const Index& index) const
{
	SmallSet<ActorIndex> output;
	for(Point3D point : getOccupiedAndAdjacentPoints(index))
		for(const ActorIndex& actor : m_area.getSpace().actor_getAll(point))
			output.maybeInsert(actor);
	return output;
}
template<class Derived, class Index>
SmallSet<ItemIndex> HasShapes<Derived, Index>::getAdjacentItems(const Index& index) const
{
	SmallSet<ItemIndex> output;
	for(Point3D point : getOccupiedAndAdjacentPoints(index))
		for(const ItemIndex& item : m_area.getSpace().item_getAll(point))
			output.maybeInsert(item);
	return output;
}
template<class Derived, class Index>
SmallSet<Point3D> HasShapes<Derived, Index>::getOccupiedAndAdjacentPoints(const Index& index) const
{
	return Shape::getPointsOccupiedAndAdjacentAt(m_shape[index], m_area.getSpace(), m_location[index], m_facing[index]);
}
template<class Derived, class Index>
SmallSet<Point3D> HasShapes<Derived, Index>::getPointsWhichWouldBeOccupiedAtLocationAndFacing(const Index& index, const Point3D& location, const Facing4& facing) const
{
	return Shape::getPointsOccupiedAt(m_shape[index], m_area.getSpace(), location, facing);
}
template<class Derived, class Index>
SmallSet<Point3D> HasShapes<Derived, Index>::getAdjacentPointsAtLocationWithFacing(const Index& index, const Point3D& location, const Facing4& facing) const
{
	return Shape::getPointsWhichWouldBeAdjacentAt(m_shape[index], m_area.getSpace(), location, facing);
}
template<class Derived, class Index>
Point3D HasShapes<Derived, Index>::getPointWhichIsAdjacentAtLocationWithFacingAndPredicate(const Index& index, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)>& predicate) const
{
	return Shape::getPointWhichWouldBeAdjacentAtWithPredicate(m_shape[index], m_area.getSpace(), location, facing, predicate);
}
template<class Derived, class Index>
Point3D HasShapes<Derived, Index>::getPointWhichIsOccupiedAtLocationWithFacingAndPredicate(const Index& index, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)>& predicate) const
{
	return Shape::getPointWhichWouldBeOccupiedAtWithPredicate(m_shape[index], m_area.getSpace(), location, facing, predicate);
}
template<class Derived, class Index>
Point3D HasShapes<Derived, Index>::getPointWhichIsAdjacentWithPredicate(const Index& index, std::function<bool(const Point3D&)>& predicate) const
{
	return getPointWhichIsAdjacentAtLocationWithFacingAndPredicate(index, m_location[index], m_facing[index], predicate);
}
template<class Derived, class Index>
Point3D HasShapes<Derived, Index>::getPointWhichIsOccupiedWithPredicate(const Index& index, std::function<bool(const Point3D&)>& predicate) const
{
	return getPointWhichIsOccupiedAtLocationWithFacingAndPredicate(index, m_location[index], m_facing[index], predicate);
}
template<class Derived, class Index>
ItemIndex HasShapes<Derived, Index>::getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const Index& index, const Point3D& location, const Facing4& facing, std::function<bool(const ItemIndex&)>& predicate) const
{
	for(const Offset3D& offset : Shape::adjacentPositionsWithFacing(m_shape[index], facing))
	{
		Point3D point = location.applyOffset(offset);
		if(point.exists())
			for(ItemIndex item : m_area.getSpace().item_getAll(point))
				if(predicate(item))
					return item;
	}
	return ItemIndex::null();
}
template<class Derived, class Index>
ItemIndex HasShapes<Derived, Index>::getItemWhichIsAdjacentWithPredicate(const Index& index, std::function<bool(const ItemIndex&)>& predicate) const
{
	return getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(index, m_location[index], m_facing[index], predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::allPointsAtLocationAndFacingAreReservable(const Index& index, const Point3D& location, const Facing4& facing, const FactionId& faction) const
{
	Space& space = m_area.getSpace();
	std::function<bool(const Point3D&)> predicate = [&space, faction](const Point3D& occupied) { return space.isReserved(occupied, faction); };
	return predicateForAnyOccupiedPointAtLocationAndFacing(index, predicate, location, facing);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::log(const Index& index) const
{
	if(!m_location[index].exists())
	{
		std::cout << "*";
		return;
	}
	Point3D coordinates = m_location[index];
	std::cout << "[" << coordinates.x().get() << "," << coordinates.y().get() << "," << coordinates.z().get() << "]";
}
template<class Derived, class Index>
const SmallSet<Point3D> HasShapes<Derived, Index>::getPointsAbove(const Index& index) const
{
	SmallSet<Point3D> output;
	const auto& contained = getOccupied(index);
	for(const Point3D& point : contained)
	{
		const Point3D& above = point.above();
		if(!contained.contains(above))
			output.insert(above);
	}
	return output;
}