#pragma once
#include "../geometry/offsetCuboidSet.h"
#include "../types.h"
#include "../index.h"

class BlockFeature;

class ConstructedShape
{
	SmallMap<Offset3D, MaterialTypeId> m_solidBlocks;
	SmallMap<Offset3D, std::vector<BlockFeature>> m_features;
	ShapeId m_occupationShape;
	ShapeId m_pathShape;
	OffsetCuboidSet m_decks;
	Force m_motiveForce = Force::create(0);
	FullDisplacement m_fullDisplacement = FullDisplacement::create(0);
	uint m_value = 0;
	BlockIndex m_location;
public:
	ConstructedShape() = default;
	ConstructedShape(const ConstructedShape&) = default;
	ConstructedShape& operator=(ConstructedShape&&) = default;
	ConstructedShape& operator=(const ConstructedShape&) = default;
	void addBlock(Area& area, const BlockIndex& origin, const Facing4& facing, const BlockIndex& newBlock);
	void removeBlock(Area& area, const BlockIndex& origin, const Facing4& facing, const BlockIndex& block);
	void recordAndErase(Area& area, const BlockIndex& orgin, const Facing4& facing);
	[[nodiscard]] SetLocationAndFacingResult tryToSetLocationAndFacing(Area& area, const BlockIndex& previousLocation, const Facing4& previousFacing, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] std::tuple<ShapeId, ShapeId, OffsetCuboidSet> getShapeAndPathingShapeAndDecks() const;
	[[nodiscard]] ShapeId extendPathingShapeWithDecks(const DistanceInBlocks& height);
	[[nodiscard]] ShapeId getOccupiedShape() const { return m_occupationShape; }
	[[nodiscard]] const OffsetCuboidSet& getDecks() const { return m_decks; }
	[[nodiscard]] FullDisplacement getFullDisplacement() const { return m_fullDisplacement; }
	[[nodiscard]] Force getMotiveForce() const { return m_motiveForce; }
	[[nodiscard]] uint getValue() const { return m_value; }
	[[nodiscard]] BlockIndex getLocation() const { return m_location; }
	[[nodiscard]] static ConstructedShape makeForKeelBlock(Area& area, const BlockIndex& block, const Facing4& facing);
};