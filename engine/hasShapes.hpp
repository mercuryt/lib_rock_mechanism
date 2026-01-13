#pragma once
#include "hasShapes.h"

#include "actors/actors.h"
#include "area/area.h"
#include "items/items.h"
#include "numericTypes/index.h"
#include "numericTypes/types.h"
#include "plants.h"
#include "simulation/simulation.h"
#include "space/space.h"

template<class Derived, class Index>
HasShapes<Derived, Index>::HasShapes(Area& area) : m_area(area) { }
template<class Derived, class Index>
void HasShapes<Derived, Index>::create(const Index& index, const ShapeId& shape, const FactionId& faction, bool isStatic)
{
	assert(m_shape.size() > index);
	// m_location, m_facing, and m_underground are to be set by calling the derived class location_set method.
	m_shape[index] = shape;
	m_compoundShape[index] = shape;
	m_faction[index] = faction;
	assert(m_occupied[index].empty());
	assert(m_occupiedWithVolume[index].empty());
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
		const MapWithCuboidKeys<CollisionVolume> toOccupy = m_occupiedWithVolume[index];
		space.shape_removeDynamicVolume(toOccupy);
		space.shape_addStaticVolume(toOccupy);
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
	if(hasLocation(index))
	{
		Space& space = m_area.getSpace();
		const MapWithCuboidKeys<CollisionVolume> toOccupy = m_occupiedWithVolume[index];
		space.shape_removeStaticVolume(toOccupy);
		space.shape_addDynamicVolume(toOccupy);
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
	// Make a copy so we can move it relative to the mount's location.
	MapWithOffsetCuboidKeys<CollisionVolume> offsetsAndVolumes = Shape::positionsWithFacing(id, facing);
	Offset3D offsetToRiderFromMount = getLocation(index).offsetTo(location);
	for(auto& [offsetCuboid, volume]  : offsetsAndVolumes)
		offsetCuboid = offsetCuboid.relativeToOffset(offsetToRiderFromMount);
	m_compoundShape[index] = Shape::mutateAddMultiple(m_compoundShape[index], offsetsAndVolumes);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::removeShapeFromCompoundShape(const Index& index, const ShapeId& id, const Point3D& location, const Facing4& facing)
{
	auto offsetsAndVolumes = Shape::positionsWithFacing(id, facing);
	Offset3D offsetToRiderFromMount = getLocation(index).offsetTo(location);
	for(auto& [offsetCuboid, volume]  : offsetsAndVolumes)
		offsetCuboid = offsetCuboid.relativeToOffset(offsetToRiderFromMount);
	m_compoundShape[index] = Shape::mutateRemoveMultiple(m_compoundShape[index], offsetsAndVolumes);
}
template<class Derived, class Index>
std::vector<std::pair<int32_t, Index>> HasShapes<Derived, Index>::getSortOrder(const Index& begin, const Index& end)
{
	assert(end > begin + 1);
	std::vector<std::pair<int32_t, Index>> sortOrder;
	sortOrder.reserve((end - begin).get());
	for(Index index = begin; index < end; ++index)
		sortOrder.emplace_back(m_location[index].hilbertNumber(), index);
	std::ranges::sort(sortOrder, std::less{}, &std::pair<int32_t, Index>::first);
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
	const auto& occupied = Shape::getCuboidsOccupiedAt(m_shape[index], m_area.getSpace(), location, facing);
	return occupied.isTouching(m_area.getActors().getOccupied(actor));
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToItemAt(const Index& index, const Point3D& location, const Facing4& facing, const ItemIndex& item) const
{
	const auto& occupied = Shape::getCuboidsOccupiedAt(m_shape[index], m_area.getSpace(), location, facing);
	return occupied.isTouching(m_area.getItems().getOccupied(item));
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToPlantAt(const Index& index, const Point3D& location, const Facing4& facing, const PlantIndex& plant) const
{
	const auto& occupied = Shape::getCuboidsOccupiedAt(m_shape[index], m_area.getSpace(), location, facing);
	return occupied.isTouching(m_area.getPlants().getOccupied(plant));
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
	return getAdjacentCuboids(index).contains(location);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToOrOccupies(const Index& index, const Point3D& point) const
{
	return m_occupied[index].intersects(point.getAllAdjacentIncludingOutOfBounds());
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToAnyCuboid(const Index& index, const CuboidSet& cuboids) const
{
	const auto predicate = [&](const Cuboid& cuboid) { return cuboids.intersects(cuboid); };
	return predicateForAnyAdjacentCuboid(index, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isOnEdgeAt(const Index& index, const Point3D& location, const Facing4& facing) const
{
	Space& space = m_area.getSpace();
	const auto predicate = [&](const Cuboid& cuboid) { return space.isEdge(cuboid); };
	return predicateForAnyOccupiedCuboidAtLocationAndFacing(index, predicate, location, facing);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isOnEdge(const Index& index) const
{
	Space& space = m_area.getSpace();
	const auto predicate = [&](const Cuboid& cuboid) { return space.isEdge(cuboid); };
	return predicateForAnyOccupiedCuboid(index, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isOnSurface(const Index& index) const { return m_onSurface[index]; }
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isIntersectingOrAdjacentTo(const Index& index, const CuboidSet& cuboids) const
{
	return m_occupied[index].isIntersectingOrAdjacentTo(cuboids);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isIntersectingOrAdjacentTo(const Index& index, const ActorIndex& actor) const
{
	assert(m_area.getActors().hasLocation(actor));
	assert(hasLocation(index));
	return m_occupied[index].isIntersectingOrAdjacentTo(m_area.getActors().getOccupied(actor));
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isIntersectingOrAdjacentTo(const Index& index, const ItemIndex& item) const
{
	assert(m_area.getItems().hasLocation(item));
	assert(hasLocation(index));
	return m_occupied[index].isIntersectingOrAdjacentTo(m_area.getItems().getOccupied(item));
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::predicateForAnyOccupiedCuboid(const Index& index, std::function<bool(const Cuboid&)> predicate) const
{
	assert(!m_occupied[index].empty());
	assert(!m_occupiedWithVolume[index].empty());
	for(const Cuboid& cuboid : m_occupied[index])
		if(predicate(cuboid))
			return true;
	return false;
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::predicateForAnyOccupiedCuboidAtLocationAndFacing(const Index& index, std::function<bool(const Cuboid&)> predicate, const Point3D& location, const Facing4& facing) const
{
	const CuboidSet occupied = getCuboidsWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	for(const Cuboid& cuboid : occupied)
		if(predicate(cuboid))
			return true;
	return false;
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::predicateForAnyAdjacentCuboid(const Index& index, std::function<bool(const Cuboid&)> predicate) const
{
	assert(m_location[index].exists());
	const OffsetCuboid& boundry = m_area.getSpace().offsetBoundry();
	//TODO: cache this.
	for(const OffsetCuboid& offset : Shape::adjacentCuboidsWithFacing(m_shape[index], m_facing[index]))
	{
		Cuboid cuboid = Cuboid::create(offset.intersection(boundry).relativeToPoint(m_location[index]));
		if(predicate(cuboid))
			return true;
	}
	return false;
}
template<class Derived, class Index>
CuboidSet HasShapes<Derived, Index>::getAdjacentCuboids(const Index& index) const
{
	CuboidSet output;
	const auto& occupied = m_occupied[index];
	const Cuboid& boundry = m_area.getSpace().boundry();
	// Make a copy so we can inflate it.
	for(Cuboid cuboid : occupied)
	{
		cuboid.inflate({1});
		output.maybeAdd(cuboid.intersection(boundry));
	}
	output.maybeRemoveAll(occupied);
	return output;
}
template<class Derived, class Index>
SmallSet<ActorIndex> HasShapes<Derived, Index>::getAdjacentActors(const Index& index) const
{
	SmallSet<ActorIndex> output;
	for(const Cuboid& cuboid : getOccupiedAndAdjacentCuboids(index))
		for(const ActorIndex& actor : m_area.getSpace().actor_getAll(cuboid))
			output.maybeInsert(actor);
	return output;
}
template<class Derived, class Index>
SmallSet<ItemIndex> HasShapes<Derived, Index>::getAdjacentItems(const Index& index) const
{
	SmallSet<ItemIndex> output;
	for(Cuboid cuboid : getOccupiedAndAdjacentCuboids(index))
		for(const ItemIndex& item : m_area.getSpace().item_getAll(cuboid))
			output.maybeInsert(item);
	return output;
}
template<class Derived, class Index>
CuboidSet HasShapes<Derived, Index>::getOccupiedAndAdjacentCuboids(const Index& index) const
{
	return Shape::getCuboidsOccupiedAndAdjacentAt(m_shape[index], m_area.getSpace(), m_location[index], m_facing[index]);
}
template<class Derived, class Index>
CuboidSet HasShapes<Derived, Index>::getCuboidsWhichWouldBeOccupiedAtLocationAndFacing(const Index& index, const Point3D& location, const Facing4& facing) const
{
	return Shape::getCuboidsOccupiedAt(m_shape[index], m_area.getSpace(), location, facing);
}
template<class Derived, class Index>
CuboidSet HasShapes<Derived, Index>::getAdjacentCuboidsAtLocationWithFacing(const Index& index, const Point3D& location, const Facing4& facing) const
{
	return Shape::getCuboidsWhichWouldBeAdjacentAt(m_shape[index], m_area.getSpace(), location, facing);
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
	const OffsetCuboid boundry = m_area.getSpace().offsetBoundry();
	for(const OffsetCuboid& offset : Shape::adjacentCuboidsWithFacing(m_shape[index], facing))
	{
		const OffsetCuboid relativeOffset = offset.relativeToPoint(location);
		if(boundry.intersects(relativeOffset))
			for(ItemIndex item : m_area.getSpace().item_getAll(Cuboid::create(relativeOffset)))
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
	std::function<bool(const Cuboid&)> predicate = [&space, faction](const Cuboid& occupied) { return space.isReservedAny(occupied, faction); };
	return !predicateForAnyOccupiedCuboidAtLocationAndFacing(index, predicate, location, facing);
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
CuboidSet HasShapes<Derived, Index>::getCuboidsAbove(const Index& index) const
{
	CuboidSet output;
	const CuboidSet& contained = getOccupied(index);
	for(const Cuboid& cuboid : contained)
	{
		Cuboid above = cuboid.getFace(Facing6::Above);
		above.shift(Facing6::Above, Distance::create(1));
		output.maybeAdd(above);
	}
	return output;
}