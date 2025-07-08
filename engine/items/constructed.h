/*
	Represents shapes constructed by the player, such as large ships.
	Stores an offset map of solid space and another of point features.
	There maps may be rotated in place to match the current facing.
	TODO: Treat dynamic constructed shapes as temporary blockages in pathing.
*/
#pragma once
#include "../geometry/offsetCuboidSet.h"
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "../json.h"
#include "../pointFeature.h"
#include "../dataStructures/smallMap.h"

class ConstructedShape
{
	SmallMap<Offset3D, MaterialTypeId> m_solidPoints;
	SmallMap<Offset3D, PointFeatureSet> m_features;
	OffsetCuboidSet m_decks;
	ShapeId m_shape;
	ShapeId m_shapeIncludingDecks;
	uint m_value = 0;
	Force m_motiveForce = Force::create(0);
	FullDisplacement m_fullDisplacement = FullDisplacement::create(0);
	Mass m_mass = Mass::create(0);
	void constructShape();
	void constructDecks();
	void addPoint(Area& area, const Point3D& origin, const Facing4& facing, const Point3D& newPoint);
public:
	ConstructedShape() = default;
	ConstructedShape(const ConstructedShape&) = default;
	explicit ConstructedShape(const Json& data);
	ConstructedShape& operator=(ConstructedShape&&) = default;
	ConstructedShape& operator=(const ConstructedShape&) = default;
	// Remove this shape from space and update the status of the features.
	void recordAndClearDynamic(Area& area, const Point3D& origin);
	void recordAndClearStatic(Area& area, const Point3D& origin);
	void setLocationAndFacingDynamic(Area& area, const Facing4& currentFacing, const Point3D& newLocation, const Facing4& newFacing, OccupiedSpaceForHasShape& occupied);
	void setLocationAndFacingStatic(Area& area, const Facing4& currentFacing, const Point3D& newLocation, const Facing4& newFacing, OccupiedSpaceForHasShape& occupied);
	[[nodiscard]] SetLocationAndFacingResult tryToSetLocationAndFacingDynamic(Area& area, const Facing4& currentFacing, const Point3D& newLocation, const Facing4& newFacing, OccupiedSpaceForHasShape& occupied);
	[[nodiscard]] ShapeId getShape() const { return m_shape; }
	[[nodiscard]] ShapeId getShapeIncludingDecks() const { return m_shapeIncludingDecks; }
	[[nodiscard]] const OffsetCuboidSet& getDecks() const { return m_decks; }
	[[nodiscard]] FullDisplacement getFullDisplacement() const { return m_fullDisplacement; }
	[[nodiscard]] Force getMotiveForce() const { return m_motiveForce; }
	[[nodiscard]] uint getValue() const { return m_value; }
	[[nodiscard]] Mass getMass() const { return m_mass; }
	// TODO: this would be better as a view.
	[[nodiscard]] SmallSet<MaterialTypeId> getMaterialTypesAt(const Point3D& location, const Facing4& facing, const Point3D& point) const;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] static std::pair<ConstructedShape, Point3D> makeForKeelPoint(Area& area, const Point3D& point, const Facing4& facing);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ConstructedShape, m_solidPoints, m_features, m_decks, m_motiveForce, m_fullDisplacement, m_value, m_mass);
};
inline void to_json(Json& data, const std::unique_ptr<ConstructedShape>& shape) { data = *shape; }
inline void from_json(const Json& data, std::unique_ptr<ConstructedShape>& shape) { shape = std::make_unique<ConstructedShape>(data.get<ConstructedShape>()); }