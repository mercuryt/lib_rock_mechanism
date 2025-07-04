#pragma once
#include "area/area.h"
#include "definitions/shape.h"
#include "blocks/blocks.h"
#include "numericTypes/index.h"
#include "callbackTypes.h"

class MoveType;

class PathInnerLoops
{
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool symetric, bool adjacentSingleTile>
	static FindPathResult findPathInner(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const BlockIndex& start)
	{
		const Blocks& blocks = area.getBlocks();
		// Use BlockIndex::max to indicate the start.
		memo.setOpen(start, area);
		memo.setClosed(start, BlockIndex::max());
		while(!memo.openEmpty())
		{
			BlockIndex current = memo.next();
			for(AdjacentIndex adjacentCount = AdjacentIndex::create(0); adjacentCount < maxAdjacent; ++adjacentCount)
			{
				BlockIndex adjacentIndex = blocks.indexAdjacentToAtCount(current, adjacentCount);
				if(adjacentIndex.empty())
					// No block here, edge of area.
					continue;
				if constexpr (adjacentSingleTile)
				{
					Facing4 facing;
					if constexpr(!symetric)
						facing = blocks.facingToSetWhenEnteringFrom(adjacentIndex, current);
					else
						facing = Facing4::North;
					if(memo.isClosed(adjacentIndex))
						continue;
					//[[maybe_unused]] Point3D coordinates = blocks.getCoordinates(adjacentIndex);
					//[[maybe_unused]] bool stopHere = coordinates.y() == 7 && coordinates.x() == 2 && coordinates.z() == 1;
					auto [result, blockWhichPassedPredicate] = destinationCondition(adjacentIndex, facing);
					if(result)
					{
						const SmallSet<BlockIndex>& path = memo.getPath(current, BlockIndex::null());
						return {path, blockWhichPassedPredicate, path.empty()};
					}
					if(terrainFacade.canEnterFrom(current, adjacentCount) && accessCondition(adjacentIndex, facing))
					{
						memo.setOpen(adjacentIndex, area);
						memo.setClosed(adjacentIndex, current);
					}
				}
				else if(terrainFacade.canEnterFrom(current, adjacentCount))
				{
					assert(adjacentIndex.exists());
					if(memo.isClosed(adjacentIndex))
						continue;
					Facing4 facing;
					if constexpr(!symetric)
						facing = blocks.facingToSetWhenEnteringFrom(adjacentIndex, current);
					else
						facing = Facing4::North;
					if(!accessCondition(adjacentIndex, facing))
						continue;
					auto [result, blockWhichPassedPredicate] = destinationCondition(adjacentIndex, facing);
					if(result)
					{
						return {memo.getPath(current, adjacentIndex), blockWhichPassedPredicate, false};
					}
					memo.setOpen(adjacentIndex, area);
					memo.setClosed(adjacentIndex, current);
				}
			}
		}
		return {{}, BlockIndex::null(), false};
	}
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool adjacentSingleTile>
	static FindPathResult findPathSetSymetric(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const BlockIndex& start, bool isSymetric)
	{
		if(isSymetric)
			return findPathInner<AccessConditionT, DestinationConditionT, Memo, true, adjacentSingleTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start);
		else
			return findPathInner<AccessConditionT, DestinationConditionT, Memo, false, adjacentSingleTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start);
	}
	// Consumes isMultiTile, faction, and shape, all other arguments passed on to setSymetric.
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool adjacentSingleTile, bool isMultiTile>
	static FindPathResult findPathSetUnreserved(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const BlockIndex& start, const FactionId& faction, const ShapeId& shape)
	{
		if(faction.exists())
		{
			const Blocks& blocks = area.getBlocks();
			if(isMultiTile)
			{
				auto unreservedCondition = [&](const BlockIndex& block, const Facing4& facing) -> std::pair<bool, BlockIndex>
				{
					for(const BlockIndex& block : Shape::getBlocksOccupiedAt(shape, blocks, block, facing))
						if(blocks.isReserved(block, faction))
							return {false, block};
					return destinationCondition(block, facing);
				};
				return findPathSetSymetric<AccessConditionT, decltype(unreservedCondition), Memo, adjacentSingleTile>(accessCondition, unreservedCondition, area, terrainFacade, memo, start, Shape::getIsRadiallySymetrical(shape));
			}
			else
			{
				auto unreservedCondition = [&](const BlockIndex& block, const Facing4& facing) -> std::pair<bool, BlockIndex>
				{
					if(blocks.isReserved(block, faction))
						return {false, block};
					return destinationCondition(block, facing);
				};
				return findPathSetSymetric<AccessConditionT, decltype(unreservedCondition), Memo, adjacentSingleTile>(accessCondition, unreservedCondition, area, terrainFacade, memo, start, Shape::getIsRadiallySymetrical(shape));
			}
		}
		else
			return findPathSetSymetric<AccessConditionT, DestinationConditionT, Memo, adjacentSingleTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, Shape::getIsRadiallySymetrical(shape));
	}
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool adjacentSingleTile, bool isMultiTile>
	static FindPathResult findPathSetMaxRange(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const BlockIndex& start, const FactionId& faction, const ShapeId& shape, const DistanceInBlocks& maxRange)
	{
		if(maxRange != DistanceInBlocks::max())
		{
			const Blocks& blocks = area.getBlocks();
			auto maxRangeCondition = [&blocks, accessCondition, maxRange, start](const BlockIndex& block, const Facing4& facing) -> bool
			{
				return blocks.taxiDistance(block, start) <= maxRange && accessCondition(block, facing);
			};
			return findPathSetUnreserved<decltype(maxRangeCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(maxRangeCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape);
		}
		else
			return findPathSetUnreserved<AccessConditionT, DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape);
	}
	template<bool adjacentSingleTile, bool isMultiTile, DestinationCondition DestinationConditionT, typename Memo>
	static FindPathResult findPathSetDetour(DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const BlockIndex& start, const Facing4& startFacing, const FactionId& faction, const ShapeId& shape, const MoveTypeId& moveType, bool detour, const DistanceInBlocks& maxRange)
	{
		const Blocks& blocks = area.getBlocks();
		if constexpr(isMultiTile)
		{
			if(detour)
			{
				// TODO: This is converting a SmallSet<BlockIndex> into a SmallSet<BlockIndex>, remove it when SmallSet<BlockIndex> is removed.
				const OccupiedBlocksForHasShape initalBlocks = OccupiedBlocksForHasShape::create(Shape::getBlocksOccupiedAt(shape, blocks, start, startFacing));
				auto accessCondition = [&blocks, &initalBlocks, shape, moveType](const BlockIndex& block, const Facing4& facing) -> bool
				{
					return blocks.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(block, shape, moveType, facing, initalBlocks);
				};
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
			else
			{
				auto accessCondition = [&blocks, shape, moveType](const BlockIndex& block, const Facing4& facing) -> bool
				{
					return blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(block, shape, moveType, facing);
				};
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
		}
		else
		{
			if(detour)
			{
				CollisionVolume volume = Shape::getCollisionVolumeAtLocationBlock(shape);
				auto accessCondition = [start, &blocks, maxRange, volume](const BlockIndex& block, const Facing4&) -> bool
				{
					if (maxRange != DistanceInBlocks::max())
						if(blocks.taxiDistance(start, block) > maxRange)
							return false;
					return block == start || blocks.shape_getDynamicVolume(block) + volume <= Config::maxBlockVolume;
				};
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
			else
			{
				auto accessCondition = [](const BlockIndex&, const Facing4&) -> bool { return true; };
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
		}
	}
public:
	// Consumes adjacent and anyOccupiedBlock, all other paramaters are passed on to findPathSetDetour and onward.
	template<typename Memo, bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
	static FindPathResult findPath(DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const ShapeId& shape, const MoveTypeId& moveType, const BlockIndex& start, const Facing4& facing, bool detour, bool adjacent, const FactionId& faction, const DistanceInBlocks maxRange)
	{
		auto [result, blockWhichPassedPredicate] = destinationCondition(start, facing);
		if(result)
			return {{}, blockWhichPassedPredicate, true};
		const Blocks& blocks = area.getBlocks();
		if(Shape::getIsMultiTile(shape))
		{
			if(adjacent)
			{
				auto adjacentCondition = [shape, &blocks, destinationCondition](const BlockIndex& block, const Facing4& facing) ->std::pair<bool, BlockIndex>
				{
					for(const BlockIndex& adjacent : Shape::getBlocksWhichWouldBeAdjacentAt(shape, blocks, block, facing))
						if(destinationCondition(adjacent, Facing4::Null).first)
							return {true, adjacent};
					return {false, block};
				};
				return findPathSetDetour<false, true, decltype(adjacentCondition), decltype(memo)>(adjacentCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			}
			else
			{
				if constexpr (anyOccupiedBlock)
				{
					auto occupiedCondition = [shape, &blocks, destinationCondition](const BlockIndex& block, const Facing4& facing) ->std::pair<bool, BlockIndex>
					{
						for(const BlockIndex& occupied : Shape::getBlocksOccupiedAt(shape, blocks, block, facing))
							if(destinationCondition(occupied, Facing4::Null).first)
								return {true, occupied};
						return {false, block};
					};
					return findPathSetDetour<false, true, decltype(occupiedCondition), decltype(memo)>(occupiedCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
				}
				return findPathSetDetour<false, true, decltype(destinationCondition), decltype(memo)>(destinationCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			}
		}
		else
		{
			if(adjacent)
				// Don't wrap destination condition with an adjacentCondition here: use the singleTileAdjacent optimization instead.
				return findPathSetDetour<true, false, decltype(destinationCondition), decltype(memo)>(destinationCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			else
				return findPathSetDetour<false, false, decltype(destinationCondition), decltype(memo)>(destinationCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
		}
	}
};