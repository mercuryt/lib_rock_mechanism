#include "constructed.h"
#include "items.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../pointFeature.h"
#include "../numericTypes/types.h"
ConstructedShape::ConstructedShape(const Json& data) { nlohmann::from_json(data, *this); }
void ConstructedShape::addCuboidSet(Area& area, const Point3D& origin, const Facing4& facing, const CuboidSet& cuboids)
{
	Space& space = area.getSpace();
	for(const auto& [solidCuboid, materialType] : space.solid_getAllWithCuboidsAndRemove(cuboids))
	{
		OffsetCuboid offsetCuboid = solidCuboid.difference(origin);
		offsetCuboid.rotate2D(facing);
		m_solid.insertOrMerge(offsetCuboid, materialType);
		uint size = solidCuboid.volume();
		m_value += MaterialType::getValuePerUnitFullDisplacement(materialType) * size;
		m_mass += MaterialType::getMassForSolidVolumeAsANumberOfPoints(materialType, size);
		m_fullDisplacement += FullDisplacement::createFromCollisionVolume(Config::maxPointVolume) * size;
	}
	for(const auto& [featureCuboid, feature] : space.pointFeature_getAllWithCuboidsAndRemove(cuboids))
	{
		OffsetCuboid offsetCuboid = featureCuboid.difference(origin);
		offsetCuboid.rotate2D(facing);
		m_features.insertOrMerge(offsetCuboid, feature);
		const auto& featureType = PointFeatureType::byId(feature.pointFeatureType);
		uint size = featureCuboid.volume();
		if(featureType.blocksEntrance)
			m_fullDisplacement += FullDisplacement::createFromCollisionVolume(Config::maxPointVolume) * size;
		m_value += featureType.value * MaterialType::getValuePerUnitFullDisplacement(feature.materialType) * size;
		m_motiveForce += featureType.motiveForce * size;
	}
}
void ConstructedShape::constructShape()
{
	// Collect all space making up the shape, including enterable features.
	MapWithOffsetCuboidKeys<CollisionVolume> occupied;
	for(const auto& pair : m_solid)
		occupied.insertOrMerge({pair.first, Config::maxPointVolume});
	for(const auto& pair : m_features)
		occupied.insertOrMerge({pair.first, Config::maxPointVolume});
	auto includingDecks = occupied;
	m_shape = Shape::createCustom(std::move(occupied));
	for(const OffsetCuboid& offset : m_decks)
		includingDecks.insert(offset, Config::maxPointVolume);
	m_shapeIncludingDecks = Shape::createCustom(std::move(includingDecks));
}
void ConstructedShape::constructDecks()
{
	for(const auto& pair : m_solid)
		m_decks.add(pair.first.above());
	for(const auto& [cuboid, feature] : m_features)
	{
		const OffsetCuboid above = cuboid.above();
		if(PointFeatureType::byId(feature.pointFeatureType).canStandAbove)
			for(const Offset3D& point : above)
				if(!m_solid.contains(point))
					m_decks.add(point);
	}
	for(const auto& pair : m_solid)
		m_decks.removeContainedAndFragmentIntercepted(pair.first);
	for(const auto& pair : m_features)
	{
		const PointFeatureType& featureType = PointFeatureType::byId(pair.second.pointFeatureType);
		if(featureType.blocksEntrance)
			m_decks.remove(pair.first);
	}
}
void ConstructedShape::recordAndClearDynamic(Area& area, const CuboidSet& occupied, const Point3D& location)
{
	Space& space = area.getSpace();
	for(const auto& [solidCuboid, materialType] : space.solid_getAllWithCuboidsAndRemove(occupied))
		for(const Point3D& point : solidCuboid)
			space.solid_setNotDynamic(point);
	m_features.clear();
	for(const auto& [featureCuboid, feature] : space.pointFeature_getAllWithCuboidsAndRemove(occupied))
		m_features.insertOrMerge(featureCuboid.offsetTo(location), feature);
	//TODO: combine with other loop at 72? Use cuboids rather then points.
	for(const Cuboid& cuboid : occupied)
		for(const Point3D& point : cuboid)
			space.pointFeature_removeAll(point);
}
void ConstructedShape::recordAndClearStatic(Area& area, const CuboidSet& occupied, const Point3D& location)
{
	Space& space = area.getSpace();
	for(const auto& [solidCuboid, materialType] : space.solid_getAllWithCuboidsAndRemove(occupied))
		space.solid_setNotCuboid(solidCuboid);
	m_features.clear();
	for(const auto& [featureCuboid, feature] : space.pointFeature_getAllWithCuboidsAndRemove(occupied))
		m_features.insertOrMerge(featureCuboid.offsetTo(location), feature);
	for(const Cuboid& cuboid : occupied)
		for(const Point3D& point : cuboid)
			space.pointFeature_removeAll(point);
}
void ConstructedShape::setLocationAndFacingDynamic(Area& area, const Facing4& currentFacing, const Point3D& newLocation, const Facing4& newFacing, CuboidSet& occupied)
{
	Space& space = area.getSpace();
	if(newFacing != currentFacing)
	{
		for(auto& [offsetCuboid, materialType] : m_solid)
			offsetCuboid.rotate2D(currentFacing, newFacing);
		for(auto& [offsetCuboid, features] : m_features)
			offsetCuboid.rotate2D(currentFacing, newFacing);
	}
	[[maybe_unused]] OffsetCuboid boundry = space.offsetBoundry();
	for(const auto& [offsetCuboid, materialType] : m_solid)
	{
		const OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToPoint(newLocation);
		assert(boundry.contains(relativeOffsetCuboid));
		const Cuboid cuboid = Cuboid::create(relativeOffsetCuboid);
		assert(space.shape_anythingCanEnterEver(cuboid));
		assert(space.item_empty(cuboid) && space.actor_empty(cuboid));
		space.solid_setCuboidDynamic(cuboid, materialType, true);
		occupied.add(cuboid);
	}
	for(auto& [offsetCuboid, feature] : m_features)
	{
		const OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToPoint(newLocation);
		assert(boundry.contains(relativeOffsetCuboid));
		const Cuboid cuboid = Cuboid::create(relativeOffsetCuboid);
		assert(space.shape_anythingCanEnterEver(cuboid));
		space.pointFeature_add(cuboid, feature);
		space.setDynamic(cuboid);
		occupied.add(cuboid);
	}
}
void ConstructedShape::setLocationAndFacingStatic(Area& area, const Facing4& currentFacing, const Point3D& newLocation, const Facing4& newFacing, CuboidSet& occupied)
{
	Space& space = area.getSpace();
	if(newFacing != currentFacing)
	{
		for(auto& [offsetCuboid, materialType] : m_solid)
			offsetCuboid.rotate2D(currentFacing, newFacing);
		for(auto& [offsetCuboid, features] : m_features)
			offsetCuboid.rotate2D(currentFacing, newFacing);
	}
	[[maybe_unused]] OffsetCuboid boundry = space.offsetBoundry();
	for(const auto& [offsetCuboid, materialType] : m_solid)
	{
		const OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToOffset(newLocation);
		assert(boundry.contains(relativeOffsetCuboid));
		const Cuboid cuboid = Cuboid::create(relativeOffsetCuboid);
		assert(space.shape_anythingCanEnterEver(cuboid));
		assert(space.item_empty(cuboid) && space.actor_empty(cuboid));
		space.solid_setCuboid(cuboid, materialType, true);
		occupied.add(cuboid);
	}
	for(auto& [offsetCuboid, feature] : m_features)
	{
		const OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToOffset(newLocation);
		assert(boundry.contains(relativeOffsetCuboid));
		const Cuboid cuboid = Cuboid::create(relativeOffsetCuboid);
		// Not bothering with move here because this method shouldn't be very hot.
		space.pointFeature_add(cuboid, feature);
		occupied.add(cuboid);
	}
}
SmallSet<MaterialTypeId> ConstructedShape::getMaterialTypesAt(const Point3D& location, const Facing4& facing, const Point3D& point) const
{
	SmallSet<MaterialTypeId> output;
	Offset3D offset = location.offsetTo(point);
	// All m_solid and m_features are both stored with north facing, subtract the current facing from it's self to convert the offset to north.
	offset.rotate2DInvert(facing);
	for(const auto& [cuboid, mateiralType] : m_solid)
		if(cuboid.contains(offset))
			output.insert(mateiralType);
	for(const auto& [cuboid, feature] : m_features)
		if(cuboid.contains(offset))
			output.insert(feature.materialType);
	return output;
}
std::pair<ConstructedShape, Point3D> ConstructedShape::makeForKeelPoint(Area& area, const Point3D& point, const Facing4& facing)
{
	ConstructedShape output;
	Space& space = area.getSpace();
	auto keelCondition = [&](const Point3D& p) { return space.pointFeature_contains(p, PointFeatureTypeId::Keel); };
	assert(keelCondition(point));
	CuboidSet keelCuboids = space.collectAdjacentsWithCondition(point, keelCondition);
	assert(!keelCuboids.empty());
	Offset3D sum = Offset3D::create(0,0,0);
	for(const Cuboid& cuboid : keelCuboids)
		sum += cuboid.getCenter().toOffset();
	Point3D center = keelCuboids.center();
	Distance lowestZ = keelCuboids.lowestZ();
	center.setZ(lowestZ);
	output.addCuboidSet(area, center, facing, keelCuboids);
	auto occupiedCondition = [&](const Cuboid& cuboid) -> CuboidSet
	{
		CuboidSet conditionOutput;
		for(const Cuboid& solidCuboid : space.solid_getCuboidsIntersecting(cuboid))
			// Only keel space are allowed on same level as center.
			if(solidCuboid.m_low.z() > 0)
				conditionOutput.add(solidCuboid);
		for(const auto& [featureCuboid, feature] : space.pointFeature_getAllWithCuboids(cuboid))
			//TODO: handle floors, floor grates, and hatches not connecting to the level below: slice off lower part of cuboid.
			if(featureCuboid.m_low.z() > 0 || feature.pointFeatureType == PointFeatureTypeId::Keel)
				conditionOutput.add(featureCuboid);
		return conditionOutput;
	};
	constexpr bool includeStart = false;
	CuboidSet connectedCuboids = space.collectAdjacentsWithCondition<includeStart>(keelCuboids, occupiedCondition);
	assert(!connectedCuboids.empty());
	output.addCuboidSet(area, center, facing, connectedCuboids);
	output.constructShape();
	output.constructDecks();
	return {output, center};
}
std::pair<ConstructedShape, Point3D> ConstructedShape::makeForPlatform(Area& area, const CuboidSet& cuboids, const Facing4& facing)
{
	ConstructedShape output;
	Point3D center = cuboids.getLowest();
	output.addCuboidSet(area, center, facing, cuboids);
	output.constructShape();
	output.constructDecks();
	return {output, center};
}
Json ConstructedShape::toJson() const
{
	return *this;
}