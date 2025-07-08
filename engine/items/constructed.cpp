#include "constructed.h"
#include "items.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../pointFeature.h"
#include "../numericTypes/types.h"
ConstructedShape::ConstructedShape(const Json& data) { nlohmann::from_json(data, *this); }
void ConstructedShape::addPoint(Area& area, const Point3D& origin, const Facing4& facing, const Point3D& newPoint)
{
	Space& space =  area.getSpace();
	const Offset3D offset = origin.offsetTo(newPoint);
	const Point3D& rotatedPoint = origin.offsetRotated(offset, facing, Facing4::North);
	Offset3D rotatedOffset = origin.offsetTo(rotatedPoint);
	MaterialTypeId solid = space.solid_get(newPoint);
	bool canEnter = true;
	if(solid.exists())
	{
		m_solidPoints.insert(rotatedOffset, solid);
		m_value += MaterialType::getValuePerUnitFullDisplacement(solid);
		m_mass += space.solid_getMass(newPoint);
		canEnter = false;
		space.solid_setNot(newPoint);
	}
	else
	{
		m_features.insert(rotatedOffset, space.pointFeature_getAll(newPoint));
		space.pointFeature_removeAll(newPoint);
		const auto& features = m_features.back().second;
		// If the point is not solid then it must contain at least one feature.
		assert(!features.empty());
		for(const PointFeature& pointFeature : features)
		{
			const auto& featureType = PointFeatureType::byId(pointFeature.pointFeatureTypeId);
			if(featureType.blocksEntrance)
				canEnter = false;
			m_value += featureType.value * MaterialType::getValuePerUnitFullDisplacement(pointFeature.materialType);
			m_motiveForce += featureType.motiveForce;
			// TODO: add mass for point features.
		}
	}
	if(!canEnter)
		m_fullDisplacement += FullDisplacement::createFromCollisionVolume(Config::maxPointVolume);
}
void ConstructedShape::constructShape()
{
	// Collect all space making up the shape, including enterable features.
	SmallSet<OffsetAndVolume> space;
	for(const auto& pair : m_solidPoints)
		space.maybeInsert({pair.first, Config::maxPointVolume});
	for(const auto& pair : m_features)
		space.maybeInsert({pair.first, Config::maxPointVolume});
	m_shape = Shape::createCustom(space);
	for(const Offset3D& offset : m_decks)
		space.maybeEmplace(offset, Config::maxPointVolume);
	m_shapeIncludingDecks = Shape::createCustom(space);
}
void ConstructedShape::constructDecks()
{
	for(const auto& pair : m_solidPoints)
	{
		Offset3D above = pair.first;
		++above.z();
		m_decks.addOrMerge(above);
		m_decks.maybeRemove(pair.first);
	}
	for(const auto& pair : m_features)
	{
		if(pair.second.blocksEntrance())
			m_decks.maybeRemove(pair.first);
		if(pair.second.canStandAbove())
		{
			Offset3D above = pair.first;
			++above.z();
			if(!m_solidPoints.contains(above))
				m_decks.addOrMerge(above);
		}
	}
}
void ConstructedShape::recordAndClearDynamic(Area& area, const Point3D& origin)
{
	Space& space =  area.getSpace();
	for(const auto& [offset, materialType] : m_solidPoints)
	{
		const Point3D& point = origin.applyOffset(offset);
		assert(space.solid_get(point) == materialType);
		space.solid_setNotDynamic(point);
	}
	for(auto& [offset, features] : m_features)
	{
		const Point3D& point = origin.applyOffset(offset);
		// Update the stored feature data with moved feature data from point.
		features = space.pointFeature_getAll(point);
		space.pointFeature_removeAll(point);
		space.unsetDynamic(point);
	}
}
void ConstructedShape::recordAndClearStatic(Area& area, const Point3D& origin)
{
	Space& space =  area.getSpace();
	for(const auto& [offset, materialType] : m_solidPoints)
	{
		const Point3D& point = origin.applyOffset(offset);
		assert(space.solid_get(point) == materialType);
		space.solid_setNot(point);
	}
	for(auto& [offset, features] : m_features)
	{
		const Point3D& point = origin.applyOffset(offset);
		// Update the stored feature data with moved feature data from point.
		features = space.pointFeature_getAll(point);
		space.pointFeature_removeAll(point);
	}
}
SetLocationAndFacingResult ConstructedShape::tryToSetLocationAndFacingDynamic(Area& area, const Facing4& currentFacing, const Point3D& newLocation, const Facing4& newFacing, OccupiedSpaceForHasShape& occupied)
{
	Space& space =  area.getSpace();
	SetLocationAndFacingResult output = SetLocationAndFacingResult::Success;
	if(newFacing != currentFacing)
	{
		for(auto& [offset, materialType] : m_solidPoints)
			offset.rotate2D(currentFacing, newFacing);
		for(auto& [offset, features] : m_features)
			offset.rotate2D(currentFacing, newFacing);
	}
	Offset3D rollback;
	for(const auto& [offset, materialType] : m_solidPoints)
	{
		const Point3D point = newLocation.applyOffset(offset);
		if(!space.shape_anythingCanEnterEver(point))
		{
			rollback = offset;
			output = SetLocationAndFacingResult::PermanantlyBlocked;
		}
		else if(!space.item_empty(point) || !space.actor_empty(point))
		{
			rollback = offset;
			output = SetLocationAndFacingResult::TemporarilyBlocked;
		}
		if(output != SetLocationAndFacingResult::Success)
		{
			// Contains is more expensive then permanant or temporary point test so check it last.
			if(occupied.contains(point))
			{
				output = SetLocationAndFacingResult::Success;
				rollback.clear();
			}
			else
				break;
		}
		space.solid_setDynamic(point, materialType, true);
		occupied.insert(point);
	}
	if(!rollback.empty())
	{
		// Remove all solid space placed so far.
		assert(output != SetLocationAndFacingResult::Success);
		for(const auto& [offset, materialType] : m_solidPoints)
		{
			if(offset == rollback)
				break;
			const Point3D point = newLocation.applyOffset(offset);
			space.solid_setNotDynamic(point);
		}
		occupied.clear();
		return output;
	}
	// Solid space placed successfully, try to place point features.
	for(auto& [offset, features] : m_features)
	{
		const Point3D point = newLocation.applyOffset(offset);
		if(!space.shape_anythingCanEnterEver(point))
		{
			rollback = offset;
			output = SetLocationAndFacingResult::PermanantlyBlocked;
		}
		else if(!space.item_empty(point) || !space.actor_empty(point))
		{
			rollback = offset;
			output = SetLocationAndFacingResult::TemporarilyBlocked;
		}
		if(output != SetLocationAndFacingResult::Success)
		{
			// Contains is more expensive then permanant or temporary point test so check it last.
			if(occupied.contains(point))
			{
				output = SetLocationAndFacingResult::Success;
				rollback.clear();
			}
			else
				break;
		}
		space.pointFeature_setAll(point, features);
		occupied.insert(point);
	}
	if(!rollback.empty())
	{
		// Remove all point features placed so far. Move them back into this object.
		assert(output != SetLocationAndFacingResult::Success);
		for(auto& [offset, features] : m_features)
		{
			if(offset == rollback)
				break;
			const Point3D point = newLocation.applyOffset(offset);
			features = space.pointFeature_getAll(point);
			space.pointFeature_removeAll(point);
		}
		// Remove all solid space.
		assert(output != SetLocationAndFacingResult::Success);
		for(const auto& [offset, materialType] : m_solidPoints)
		{
			const Point3D point = newLocation.applyOffset(offset);
			space.solid_setNotDynamic(point);
		}
		occupied.clear();
		return output;
	}
	assert(output == SetLocationAndFacingResult::Success);
	return output;
}
void ConstructedShape::setLocationAndFacingDynamic(Area& area, const Facing4& currentFacing, const Point3D& newLocation, const Facing4& newFacing, OccupiedSpaceForHasShape& occupied)
{
	Space& space =  area.getSpace();
	if(newFacing != currentFacing)
	{
		for(auto& [offset, materialType] : m_solidPoints)
			offset.rotate2D(currentFacing, newFacing);
		for(auto& [offset, features] : m_features)
			offset.rotate2D(currentFacing, newFacing);
	}
	for(const auto& [offset, materialType] : m_solidPoints)
	{
		const Point3D point = newLocation.applyOffset(offset);
		assert(space.shape_anythingCanEnterEver(point));
		assert(space.item_empty(point) && space.actor_empty(point));
		space.solid_setDynamic(point, materialType, true);
		occupied.insert(point);
	}
	for(auto& [offset, features] : m_features)
	{
		const Point3D point = newLocation.applyOffset(offset);
		space.pointFeature_setAll(point, features);
		occupied.insert(point);
	}
}
void ConstructedShape::setLocationAndFacingStatic(Area& area, const Facing4& currentFacing, const Point3D& newLocation, const Facing4& newFacing, OccupiedSpaceForHasShape& occupied)
{
	Space& space =  area.getSpace();
	if(newFacing != currentFacing)
	{
		for(auto& [offset, materialType] : m_solidPoints)
			offset.rotate2D(currentFacing, newFacing);
		for(auto& [offset, features] : m_features)
			offset.rotate2D(currentFacing, newFacing);
	}
	for(const auto& [offset, materialType] : m_solidPoints)
	{
		const Point3D point = newLocation.applyOffset(offset);
		assert(space.shape_anythingCanEnterEver(point));
		assert(space.item_empty(point) && space.actor_empty(point));
		space.solid_set(point, materialType, true);
		occupied.insert(point);
	}
	for(auto& [offset, features] : m_features)
	{
		const Point3D point = newLocation.applyOffset(offset);
		// Not bothering with move here because this method shouldn't be very hot.
		space.pointFeature_setAll(point, features);
		occupied.insert(point);
	}
}
SmallSet<MaterialTypeId> ConstructedShape::getMaterialTypesAt(const Point3D& location, const Facing4& facing, const Point3D& point) const
{
	SmallSet<MaterialTypeId> output;
	Offset3D offset = location.offsetTo(point);
	// All m_solid and m_features are both stored with north facing, subtract the current facing from it's self to convert the offset to north.
	offset.rotate2DInvert(facing);
	auto found = m_solidPoints.find(offset);
	if(found != m_solidPoints.end())
		output.insert(found->second);
	else
	{
		auto found2 = m_features.find(offset);
		assert(found2 != m_features.end());
		for(const PointFeature& feature : found2->second)
			output.insert(feature.materialType);
	}
	return output;
}
std::pair<ConstructedShape, Point3D> ConstructedShape::makeForKeelPoint(Area& area, const Point3D& point, const Facing4& facing)
{
	ConstructedShape output;
	Space& space =  area.getSpace();
	auto keelCondition = [&](const Point3D& point) { return space.pointFeature_contains(point, PointFeatureTypeId::Keel); };
	auto keelPoints = space.collectAdjacentsWithCondition(point, keelCondition);
	Offset3D sum = Offset3D::create(0,0,0);
	for(const Point3D& point : keelPoints)
		sum += point.toOffset();
	Point3D center = Point3D::create(sum / keelPoints.size());
	Distance lowestZ = center.z();
	for(const Point3D& point : keelPoints)
		lowestZ = std::min(lowestZ, point.z());
	center.setZ(lowestZ);
	output.addPoint(area, center, facing, center);
	// Only keel space are allowed on same level as center.
	auto occupiedCondition = [&](const Point3D& point) {
		return ( space.solid_is(point) || !space.pointFeature_empty(point)) && point.z() > lowestZ;
	};
	auto connectedPoints = space.collectAdjacentsWithCondition(center, occupiedCondition);
	for(const Point3D& point : connectedPoints)
		if(point != center)
			output.addPoint(area, center, facing, point);
	for(const Point3D& point : keelPoints)
		if(point != center)
			output.addPoint(area, center, facing, point);
	output.constructShape();
	output.constructDecks();
	return {output, center};
}
Json ConstructedShape::toJson() const
{
	return *this;
}