/*
	Represents shapes constructed by the player, such as large ships.
	Stores an offset map of solid blocks and another of block features.
	There maps may be rotated in place to match the current facing.
	TODO: Treat dynamic constructed shapes as temporary blockages in pathing.
*/
#pragma once
#include "../geometry/offsetCuboidSet.h"
#include "../types.h"
#include "../index.h"
#include "../json.h"
#include "../blockFeature.h"

class ConstructedShape
{
	SmallMap<Offset3D, MaterialTypeId> m_solidBlocks;
	SmallMap<Offset3D, BlockFeatureSet> m_features;
	OffsetCuboidSet m_decks;
	ShapeId m_shape;
	ShapeId m_shapeIncludingDecks;
	uint m_value = 0;
	Force m_motiveForce = Force::create(0);
	FullDisplacement m_fullDisplacement = FullDisplacement::create(0);
	Mass m_mass = Mass::create(0);
	void constructShape();
	void constructDecks();
	void addBlock(Area& area, const BlockIndex& origin, const Facing4& facing, const BlockIndex& newBlock);
public:
	ConstructedShape() = default;
	ConstructedShape(const ConstructedShape&) = default;
	explicit ConstructedShape(const Json& data);
	ConstructedShape& operator=(ConstructedShape&&) = default;
	ConstructedShape& operator=(const ConstructedShape&) = default;
	// Remove this shape from blocks and update the status of the features.
	void recordAndClearDynamic(Area& area, const BlockIndex& origin);
	void recordAndClearStatic(Area& area, const BlockIndex& origin);
	void setLocationAndFacingDynamic(Area& area, const Facing4& currentFacing, const BlockIndex& newLocation, const Facing4& newFacing, OccupiedBlocksForHasShape& occupied);
	void setLocationAndFacingStatic(Area& area, const Facing4& currentFacing, const BlockIndex& newLocation, const Facing4& newFacing, OccupiedBlocksForHasShape& occupied);
	[[nodiscard]] SetLocationAndFacingResult tryToSetLocationAndFacingDynamic(Area& area, const Facing4& currentFacing, const BlockIndex& newLocation, const Facing4& newFacing, OccupiedBlocksForHasShape& occupied);
	[[nodiscard]] ShapeId getShape() const { return m_shape; }
	[[nodiscard]] ShapeId getShapeIncludingDecks() const { return m_shapeIncludingDecks; }
	[[nodiscard]] const OffsetCuboidSet& getDecks() const { return m_decks; }
	[[nodiscard]] FullDisplacement getFullDisplacement() const { return m_fullDisplacement; }
	[[nodiscard]] Force getMotiveForce() const { return m_motiveForce; }
	[[nodiscard]] uint getValue() const { return m_value; }
	[[nodiscard]] Mass getMass() const { return m_mass; }
	// TODO: this would be better as a view.
	[[nodiscard]] SmallSet<MaterialTypeId> getMaterialTypesAt(const Area& area, const BlockIndex& location, const Facing4& facing, const BlockIndex& block) const;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] static std::pair<ConstructedShape, BlockIndex> makeForKeelBlock(Area& area, const BlockIndex& block, const Facing4& facing);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ConstructedShape, m_solidBlocks, m_features, m_decks, m_motiveForce, m_fullDisplacement, m_value, m_mass);
};
inline void to_json(Json& data, const std::unique_ptr<ConstructedShape>& shape) { data = *shape; }
inline void from_json(const Json& data, std::unique_ptr<ConstructedShape>& shape) { shape = std::make_unique<ConstructedShape>(data.get<ConstructedShape>()); }