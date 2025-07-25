#include "space.h"
#include "adjacentOffsets.h"
#include "nthAdjacentOffsets.h"
#include "../area/area.h"
#include "../definitions/materialType.h"
#include "../numericTypes/types.h"
#include "../pointFeature.h"
#include "../fluidType.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"
#include "../portables.hpp"
#include "../dataStructures/rtreeData.hpp"
#include <string>

Space::Space(Area& area, const Distance& x, const Distance& y, const Distance& z) :
	m_area(area),
	m_pointToIndexConversionMultipliers(1, x.get(), x.get() * y.get()),
	m_dimensions(x.get(), y.get(), z.get()),
	m_sizeXInChunks(x.get() / 16),
	m_sizeXTimesYInChunks((x.get() / 16) * ( y.get() / 16)),
	m_sizeX(x),
	m_sizeY(y),
	m_sizeZ(z),
	m_zLevelSize(x.get() * y.get())
{
	m_exposedToSky.initalize(Cuboid{Point3D(x - 1, y - 1, z - 1), Point3D::create(0,0,0)});
}
void Space::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	data["solid"].get_to(m_solid);
	data["features"].get_to(m_features);
	data["exposedToSky"].get_to(m_exposedToSky);
	for(const Json& pair : data["fluid"])
	{
		Cuboid cuboid = pair[0].get<Cuboid>();
		std::vector<FluidData> fluidData = pair[1].get<std::vector<FluidData>>();
		m_fluid.insert(cuboid, std::move(fluidData));
	}
	data["mist"].get_to(m_mist);
	data["mistInverseDistanceFromSource"].get_to(m_mistInverseDistanceFromSource);
	for(const Json& pair : data["reservables"])
	{
		Cuboid cuboid = pair[0].get<Cuboid>();
		auto reservable = std::make_unique<Reservable>(Quantity::create(1u));
		deserializationMemo.m_reservables[pair[1].get<uintptr_t>()] = reservable.get();
		m_reservables.insert(cuboid, std::move(reservable));
	}
	Cuboid cuboid = boundry();
	for(const Point3D& point : cuboid)
		m_area.m_opacityFacade.update(m_area, point);
}
Json Space::toJson() const
{
	Json output{
		{"x", m_sizeX},
		{"y", m_sizeY},
		{"z", m_sizeZ},
		{"solid", m_solid},
		{"features", m_features},
		{"fluid", Json::array()},
		{"mist", m_mist},
		{"mistInverseDistanceFromSource", m_mistInverseDistanceFromSource},
		{"reservables", Json::array()},
		{"exposedToSky", m_exposedToSky}
	};
	for(const auto& [data, cuboid] : m_reservables.queryGetAllWithCuboids(boundry()))
		output["reservables"].push_back(std::pair(cuboid, reinterpret_cast<uintptr_t>(&data)));
	for(const auto& [fluids, cuboid] : m_fluid.queryGetAllWithCuboids(boundry()))
		output["fluid"].push_back(std::pair(cuboid, fluids));
	return output;
}
Cuboid Space::boundry() const
{
	auto top = Point3D(m_dimensions - 1);
	return Cuboid(top, Point3D::create(0,0,0));
}
Point3D Space::getCenterAtGroundLevel() const
{
	Point3D center(m_sizeX / 2, m_sizeY / 2, m_sizeZ - 1);
	if(center.z() == 0)
		return center;
	Point3D below = center.below();
	while(!solid_is(below) && below.z() != 0)
	{
		below = below.below();
		center = below;
	}
	return center;
}
SmallSet<Point3D> Space::getAdjacentWithEdgeAndCornerAdjacent(const Point3D& point) const
{
	SmallSet<Point3D> output;
	output.reserve(26);
	Cuboid cuboid = {Point3D(point.data + 1), point.subtractWithMinimum(Distance::create(1))};
	cuboid = cuboid.intersection(boundry());
	for(const Point3D& adjacent : cuboid)
		if(adjacent != point)
			output.insert(adjacent);
	return output;
}
SmallSet<Point3D> Space::getDirectlyAdjacent(const Point3D& point) const
{
	assert(boundry().contains(point));
	SmallSet<Point3D> output;
	if(point.x() != 0)
		output.insert(point.west());
	if(point.y() != 0)
		output.insert(point.north());
	if(point.z() != 0)
		output.insert(point.below());
	if(point.x() != m_sizeX - 1)
		output.insert(point.east());
	if(point.y() != m_sizeY - 1)
		output.insert(point.south());
	if(point.z() != m_sizeZ - 1)
		output.insert(point.above());
	return output;
}
SmallSet<Point3D> Space::getAdjacentWithEdgeAndCornerAdjacentExceptDirectlyAboveAndBelow(const Point3D& point) const
{
	SmallSet<Point3D> output;
	output.reserve(24);
	Cuboid cuboid = {Point3D(point.data + 1), point.subtractWithMinimum(Distance::create(1))};
	cuboid = cuboid.intersection(boundry());
	for(const Point3D& adjacent : cuboid)
		if(adjacent.x() != point.x() || adjacent.y() != point.x())
			output.insert(adjacent);
	return output;
}
SmallSet<Point3D> Space::getAdjacentOnSameZLevelOnly(const Point3D& point) const
{
	SmallSet<Point3D> output;
	output.reserve(4);
	if(point.x() != 0)
	{
		Point3D copy = point;
		copy.setX(point.x() - 1);
		output.insert(copy);
	}
	if(point.x() != m_sizeX - 1)
	{
		Point3D copy = point;
		copy.setX(point.x() + 1);
		output.insert(copy);
	}
	if(point.y() != 0)
	{
		Point3D copy = point;
		copy.setY(point.y() - 1);
		output.insert(copy);
	}
	if(point.y() != m_sizeY - 1)
	{
		Point3D copy = point;
		copy.setY(point.y() + 1);
		output.insert(copy);
	}
	return output;
}
SmallSet<Point3D> Space::getNthAdjacent(const Point3D& point, const Distance& n)
{
	const Cuboid spaceBoundry = boundry();
	const auto& offsets = getNthAdjacentOffsets(n.get());
	SmallSet<Point3D> output;
	const Offset3D pointOffset = point.toOffset();
	for(const Offset3D& offset : offsets)
	{
		Offset3D combined = pointOffset + offset;
		if(spaceBoundry.contains(combined))
			output.insert(Point3D::create(combined));
	}
	return output;
}
bool Space::isAdjacentToActor(const Point3D& point, const ActorIndex& actor) const
{
	const auto& points = m_area.getActors().getOccupied(actor);
	return point.isAdjacentToAny(points);
}
bool Space::isAdjacentToItem(const Point3D& point, const ItemIndex& item) const
{
	const auto& points = m_area.getItems().getOccupied(item);
	return point.isAdjacentToAny(points);
}
void Space::setExposedToSky(const Point3D& point, bool exposed)
{
	if(exposed)
		m_exposedToSky.set(m_area, point);
	else
		m_exposedToSky.unset(m_area, point);
}
void Space::setBelowVisible(const Point3D& point)
{
	if(point.z() == 0)
		return;
	Point3D below = point.below();
	while(canSeeThroughFrom(below, below.above()) && !m_visible.query(below))
	{
		m_visible.maybeInsert(below);
		if(below.z() == 0)
			break;
		below = below.below();
	}
}
bool Space::canSeeIntoFromAlways(const Point3D& to, const Point3D& from) const
{
	if(solid_is(to) && !MaterialType::getTransparent(m_solid.queryGetOne(to)))
		return false;
	if(pointFeature_contains(to, PointFeatureTypeId::Door))
		return false;
	// looking up.
	if(to.z() > from.z())
	{
		const PointFeature* floor = pointFeature_at(to, PointFeatureTypeId::Floor);
		if(floor != nullptr && !MaterialType::getTransparent(floor->materialType))
			return false;
		const PointFeature* hatch = pointFeature_at(to, PointFeatureTypeId::Hatch);
		if(hatch != nullptr && !MaterialType::getTransparent(hatch->materialType))
			return false;
	}
	// looking down.
	if(to.z() < from.z())
	{
		const PointFeature* floor = pointFeature_at(from, PointFeatureTypeId::Floor);
		if(floor != nullptr && !MaterialType::getTransparent(floor->materialType))
			return false;
		const PointFeature* hatch = pointFeature_at(from, PointFeatureTypeId::Hatch);
		if(hatch != nullptr && !MaterialType::getTransparent(hatch->materialType))
			return false;
	}
	return true;
}
void Space::moveContentsTo(const Point3D& from, const Point3D& to)
{
	if(solid_is(from))
	{
		solid_set(to, solid_get(from), m_constructed.query(from));
		solid_setNot(from);
	}
	//TODO: other stuff falls?
}
void Space::maybeContentsFalls(const Point3D& point)
{
	Items& items = m_area.getItems();
	auto itemsCopy = item_getAll(point);
	for(const ItemIndex& item : itemsCopy)
		items.fall(item);
	Actors& actors = m_area.getActors();
	auto actorsCopy = actor_getAll(point);
	for(const ActorIndex& actor : actorsCopy)
		actors.fall(actor);
}
void Space::setDynamic(const Point3D& point) { m_dynamic.maybeInsert(point); }
void Space::unsetDynamic(const Point3D& point) { m_dynamic.maybeInsert(point); }
bool Space::canSeeThrough(const Point3D& point) const
{
	const MaterialTypeId& material = solid_get(point);
	if(material.exists() && !MaterialType::getTransparent(material))
		return false;
	const PointFeature* door = pointFeature_at(point, PointFeatureTypeId::Door);
	if(door != nullptr && door->closed && !MaterialType::getTransparent(door->materialType) )
		return false;
	return true;
}
bool Space::canSeeThroughFloor(const Point3D& point) const
{
	const PointFeature* floor = pointFeature_at(point, PointFeatureTypeId::Floor);
	if(floor != nullptr && !MaterialType::getTransparent(floor->materialType))
		return false;
	const PointFeature* hatch = pointFeature_at(point, PointFeatureTypeId::Hatch);
	if(hatch != nullptr && !MaterialType::getTransparent(hatch->materialType) && hatch->closed)
		return false;
	return true;
}
bool Space::canSeeThroughFrom(const Point3D& point, const Point3D& otherPoint) const
{
	if(!canSeeThrough(point))
		return false;
	// On the same level.
	auto z = point.z();
	auto otherZ = otherPoint.z();
	if(z == otherZ)
		return true;
	// looking up.
	if(z > otherZ)
	{
		if(!canSeeThroughFloor(point))
			return false;
	}
	// looking down.
	if(z < otherZ)
	{
		if(!canSeeThroughFloor(otherPoint))
			return false;
	}
	return true;
}
bool Space::isSupport(const Point3D& point) const
{
	return solid_is(point) || pointFeature_isSupport(point);
}
bool Space::isExposedToSky(const Point3D& point) const
{
	return m_exposedToSky.check(point);
}
bool Space::isEdge(const Point3D& point) const
{
	return (point.data == 0).any() && (point.data == m_dimensions).any();
}
bool Space::hasLineOfSightTo(const Point3D& point, const Point3D& other) const
{
	return m_area.m_opacityFacade.hasLineOfSight(point, other);
}
Cuboid Space::getZLevel(const Distance& z)
{
	return Cuboid({m_sizeX - 1, m_sizeY - 1, z}, {Distance::create(0), Distance::create(0), z});
}
SmallSet<Point3D> Space::collectAdjacentsInRange(const Point3D& point, const Distance& range)
{
	auto condition = [&](const Point3D& b){ return b.taxiDistanceTo(point) <= range; };
	return collectAdjacentsWithCondition(point, condition);
}