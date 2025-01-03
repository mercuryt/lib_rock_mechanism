#include "hasShapes.h"
#include "actors/actors.h"
#include "area.h"
#include "blocks/blocks.h"
#include "index.h"
#include "items/items.h"
#include "plants.h"
#include "simulation.h"
#include "types.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <ranges>
template<class Derived, class Index>
HasShapes<Derived, Index>::HasShapes(Area& area) : m_area(area) { }
template<class Derived, class Index>
void HasShapes<Derived, Index>::create(const Index& index, const ShapeId& shape, const FactionId& faction, bool isStatic)
{
	assert(m_shape.size() > index);
	// m_location, m_facing, and m_underground are to be set by calling the derived class setLocation method.
	m_shape[index] = shape;
	m_faction[index] = faction;
	assert(m_blocks[index].empty());
	m_static.set(index, isStatic);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::resize(const Index& newSize)
{
	m_shape.resize(newSize);
	m_location.resize(newSize);
	m_facing.resize(newSize);
	m_faction.resize(newSize);
	m_blocks.resize(newSize);
	m_static.resize(newSize);
	m_underground.resize(newSize);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::moveIndex(const Index& oldIndex, const Index& newIndex)
{
	m_shape[newIndex] = m_shape[oldIndex];
	m_location[newIndex] = m_location[oldIndex];
	m_facing[newIndex] = m_facing[oldIndex];
	m_faction[newIndex] = m_faction[oldIndex];
	m_blocks[newIndex] = m_blocks[oldIndex];
	m_static.set(newIndex, m_static[oldIndex]);
	m_underground.set(newIndex, m_underground[oldIndex]);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::setStatic(const Index& index)
{
	assert(!m_static[index]);
	Blocks& blocks = m_area.getBlocks();
	for(auto& [x, y, z, v] : Shape::positionsWithFacing(m_shape[index], m_facing[index]))
	{
		BlockIndex block = blocks.offset(m_location[index], x, y, z);
		CollisionVolume volume = CollisionVolume::create(v);
		blocks.shape_addStaticVolume(block, volume);
		blocks.shape_removeDynamicVolume(block, volume);
	}
	m_static.set(index, true);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::maybeSetStatic(const Index& index)
{
	if(!m_static[index])
		setStatic(index);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::unsetStatic(const Index& index)
{
	assert(m_static[index]);
	Blocks& blocks = m_area.getBlocks();
	for(auto& [x, y, z, v] : Shape::positionsWithFacing(m_shape[index], m_facing[index]))
	{
		BlockIndex block = blocks.offset(m_location[index], x, y, z);
		CollisionVolume volume = CollisionVolume::create(v);
		blocks.shape_removeStaticVolume(block, volume);
		blocks.shape_addDynamicVolume(block, volume);
	}
	m_static.set(index, false);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::maybeUnsetStatic(const Index& index)
{
	if(m_static[index])
		unsetStatic(index);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::sortRange(const Index& begin, const Index& end)
{
	assert(false);
	assert(end > begin);
	//TODO: impliment sort with library rather then views::zip for clang / msvc support.
	/*
	auto zip = std::ranges::views::zip(m_shape, m_location, m_facing, m_faction, m_blocks, m_static, m_underground);
	std::ranges::sort(zip.begin() + begin, zip.end() + end, SpatialSort);
	*/
}
template<class Derived, class Index>
FactionId HasShapes<Derived, Index>::getFaction(const Index& index) const
{ 
	return m_faction[index];
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
bool HasShapes<Derived, Index>::isAdjacentToActorAt(const Index& index, const BlockIndex& location, const Facing& facing, const ActorIndex& actor) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return occupied.contains(block); };
	return m_area.getActors().predicateForAnyAdjacentBlock(actor, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToItemAt(const Index& index, const BlockIndex& location, const Facing& facing, const ItemIndex& item) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return occupied.contains(block); };
	return m_area.getItems().predicateForAnyAdjacentBlock(item, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToPlantAt(const Index& index, const BlockIndex& location, const Facing& facing, const PlantIndex& plant) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return occupied.contains(block); };
	return m_area.getPlants().predicateForAnyAdjacentBlock(plant, predicate);
}
template<class Derived, class Index>
DistanceInBlocks HasShapes<Derived, Index>::distanceToActor(const Index& index, const ActorIndex& actor) const
{
	// TODO: Make handle multi block creatures correctly somehow.
	// Use line of sight?
	return m_area.getBlocks().distance(m_location[index], m_area.getActors().getLocation(actor));
}
template<class Derived, class Index>
DistanceInBlocks HasShapes<Derived, Index>::distanceToItem(const Index& index, const ItemIndex& item) const
{
	// TODO: Make handle multi block creatures correctly somehow.
	// Use line of sight?
	return m_area.getBlocks().distance(m_location[index], m_area.getItems().getLocation(item));
}
template<class Derived, class Index>
DistanceInBlocksFractional HasShapes<Derived, Index>::distanceToActorFractional(const Index& index, const ActorIndex& actor) const
{
	return m_area.getBlocks().distanceFractional(m_location[index], m_area.getActors().getLocation(actor));
}
template<class Derived, class Index>
DistanceInBlocksFractional HasShapes<Derived, Index>::distanceToItemFractional(const Index& index, const ItemIndex& item) const
{
	return m_area.getBlocks().distanceFractional(m_location[index], m_area.getItems().getLocation(item));
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::allOccupiedBlocksAreReservable(const Index& index, const FactionId& faction) const
{
	return allBlocksAtLocationAndFacingAreReservable(index, m_location[index], m_facing[index], faction);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToLocation(const Index& index, const BlockIndex& location) const
{
	std::function<bool(const BlockIndex)> predicate = [&](const BlockIndex block) { return block == location; };
	return predicateForAnyAdjacentBlock(index, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isAdjacentToAny(const Index& index, const BlockIndices& locations) const
{
	std::function<bool(const BlockIndex)> predicate = [&](const BlockIndex block) { return locations.contains(block); };
	return predicateForAnyAdjacentBlock(index, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isOnEdgeAt(const Index& index, const BlockIndex& location, const Facing& facing) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex)> predicate = [&blocks](const BlockIndex block) { return blocks.isEdge(block); };
	return predicateForAnyOccupiedBlockAtLocationAndFacing(index, predicate, location, facing);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::isOnEdge(const Index& index) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex)> predicate = [&blocks](const BlockIndex block) { return blocks.isEdge(block); };
	return predicateForAnyOccupiedBlock(index, predicate);
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::predicateForAnyOccupiedBlock(const Index& index, std::function<bool(const BlockIndex&)> predicate) const
{
	assert(!m_blocks[index].empty());
	for(BlockIndex block : m_blocks[index])
		if(predicate(block))
			return true;
	return false;
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::predicateForAnyOccupiedBlockAtLocationAndFacing(const Index& index, std::function<bool(const BlockIndex&)> predicate, const BlockIndex& location, const Facing& facing) const
{
	for(BlockIndex occupied : const_cast<HasShapes&>(*this).getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		if(predicate(occupied))
			return true;
	return false;
}
template<class Derived, class Index>
bool HasShapes<Derived, Index>::predicateForAnyAdjacentBlock(const Index& index, std::function<bool(const BlockIndex&)> predicate) const
{
	assert(m_location[index].exists());
	//TODO: cache this.
	for(auto [x, y, z] : Shape::adjacentPositionsWithFacing(m_shape[index], m_facing[index]))
	{
		BlockIndex block = m_area.getBlocks().offset(m_location[index], x, y, z);
		if(block.exists() && predicate(block))
			return true;
	}
	return false;
}
template<class Derived, class Index>
BlockIndices HasShapes<Derived, Index>::getAdjacentBlocks(const Index& index) const
{
	BlockIndices output;
	for(BlockIndex block : m_blocks[index])
		for(BlockIndex adjacent : m_area.getBlocks().getAdjacentWithEdgeAndCornerAdjacent(block))
			if(!m_blocks[index].contains(adjacent))
				output.addNonunique(adjacent);
	output.unique();
	return output;
}
template<class Derived, class Index>
ActorIndices HasShapes<Derived, Index>::getAdjacentActors(const Index& index) const
{
	ActorIndices output;
	for(BlockIndex block : getOccupiedAndAdjacentBlocks(index))
		for(const ActorIndex& actor : m_area.getBlocks().actor_getAll(block))
			output.maybeAdd(actor);
	return output;
}
template<class Derived, class Index>
ItemIndices HasShapes<Derived, Index>::getAdjacentItems(const Index& index) const
{
	ItemIndices output;
	for(BlockIndex block : getOccupiedAndAdjacentBlocks(index))
		for(const ItemIndex& item : m_area.getBlocks().item_getAll(block))
			output.maybeAdd(item);
	return output;
}
template<class Derived, class Index>
BlockIndices HasShapes<Derived, Index>::getOccupiedAndAdjacentBlocks(const Index& index) const
{
	return Shape::getBlocksOccupiedAndAdjacentAt(m_shape[index], m_area.getBlocks(), m_location[index], m_facing[index]);
}
template<class Derived, class Index>
BlockIndices HasShapes<Derived, Index>::getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const Index& index, const BlockIndex& location, const Facing& facing) const
{
	return Shape::getBlocksOccupiedAt(m_shape[index], m_area.getBlocks(), location, facing);
}
template<class Derived, class Index>
BlockIndices HasShapes<Derived, Index>::getAdjacentBlocksAtLocationWithFacing(const Index& index, const BlockIndex& location, const Facing& facing) const
{
	return Shape::getBlocksWhichWouldBeAdjacentAt(m_shape[index], m_area.getBlocks(), location, facing);
}
template<class Derived, class Index>
BlockIndex HasShapes<Derived, Index>::getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(const Index& index, const BlockIndex& location, const Facing& facing, std::function<bool(const BlockIndex&)>& predicate) const
{
	return Shape::getBlockWhichWouldBeAdjacentAtWithPredicate(m_shape[index], m_area.getBlocks(), location, facing, predicate);
}
template<class Derived, class Index>
BlockIndex HasShapes<Derived, Index>::getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(const Index& index, const BlockIndex& location, const Facing& facing, std::function<bool(const BlockIndex&)>& predicate) const
{
	return Shape::getBlockWhichWouldBeOccupiedAtWithPredicate(m_shape[index], m_area.getBlocks(), location, facing, predicate);
}
template<class Derived, class Index>
BlockIndex HasShapes<Derived, Index>::getBlockWhichIsAdjacentWithPredicate(const Index& index, std::function<bool(const BlockIndex&)>& predicate) const
{
	return getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(index, m_location[index], m_facing[index], predicate);
}
template<class Derived, class Index>
BlockIndex HasShapes<Derived, Index>::getBlockWhichIsOccupiedWithPredicate(const Index& index, std::function<bool(const BlockIndex&)>& predicate) const
{
	return getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(index, m_location[index], m_facing[index], predicate);
}
template<class Derived, class Index>
ItemIndex HasShapes<Derived, Index>::getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const Index& index, const BlockIndex& location, const Facing& facing, std::function<bool(const ItemIndex&)>& predicate) const
{
	for(auto [x, y, z] : Shape::adjacentPositionsWithFacing(m_shape[index], facing))
	{
		BlockIndex block = m_area.getBlocks().offset(location, x, y, z);
		if(block.exists())
			for(ItemIndex item : m_area.getBlocks().item_getAll(block))
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
bool HasShapes<Derived, Index>::allBlocksAtLocationAndFacingAreReservable(const Index& index, const BlockIndex& location, const Facing& facing, const FactionId& faction) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex&)> predicate = [&blocks, faction](const BlockIndex& occupied) { return blocks.isReserved(occupied, faction); };
	return predicateForAnyOccupiedBlockAtLocationAndFacing(index, predicate, location, facing);
}
template<class Derived, class Index>
void HasShapes<Derived, Index>::log(const Index& index) const
{
	if(!m_location[index].exists())
	{
		std::cout << "*";
		return;
	}
	Point3D coordinates = m_area.getBlocks().getCoordinates(m_location[index]);
	std::cout << "[" << coordinates.x.get() << "," << coordinates.y.get() << "," << coordinates.z.get() << "]";
}
