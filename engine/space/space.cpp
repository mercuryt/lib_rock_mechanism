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
#include "../portables.h"
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
	// The constructors for the rtrees insert an empty root node. This is correct for 'normal' initalization but not for deserialization.
	// Delete these root nodes prior to deserializing.
	m_solid.beforeJsonLoad();
	data["solid"].get_to(m_solid);
	m_features.beforeJsonLoad();
	data["features"].get_to(m_features);
	m_exposedToSky.beforeJsonLoad();
	data["exposedToSky"].get_to(m_exposedToSky);
	for(const Json& pair : data["fluid"])
	{
		FluidData fluidData = pair[0].get<FluidData>();
		Cuboid cuboid = pair[1].get<Cuboid>();
		fluid_add(cuboid, fluidData.volume, fluidData.type);
	}
	m_mist.beforeJsonLoad();
	data["mist"].get_to(m_mist);
	m_mistInverseDistanceFromSourceSquared.beforeJsonLoad();
	data["mistInverseDistanceFromSource"].get_to(m_mistInverseDistanceFromSourceSquared);
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
		{"fluid", Json::array()},
		{"reservables", Json::array()},
	};
	output["exposedToSky"] = m_exposedToSky;
	output["solid"] = m_solid;
	output["features"] = m_features;
	output["mist"] = m_mist;
	output["mistInverseDistanceFromSource"] = m_mistInverseDistanceFromSourceSquared;
	for(const auto& [data, cuboid] : m_reservables.queryGetAllWithCuboids(boundry()))
		output["reservables"].push_back(std::pair(cuboid, reinterpret_cast<uintptr_t>(data.get())));
	for(const auto& [fluids, cuboid] : m_fluid.queryGetAllWithCuboids(boundry()))
		output["fluid"].push_back(std::pair(cuboid, fluids));
	return output;
}
Cuboid Space::boundry() const
{
	auto top = Point3D(m_dimensions - 1);
	return Cuboid(top, Point3D::create(0,0,0));
}
OffsetCuboid Space::offsetBoundry() const
{
	auto top = Point3D(m_dimensions - 1);
	return OffsetCuboid(top.toOffset(), Offset3D::create(0,0,0));
}
Point3D Space::getCenterAtGroundLevel() const
{
	Point3D center(m_sizeX / 2, m_sizeY / 2, m_sizeZ - 1);
	if(center.z() == 0)
		return center;
	Point3D below = center.below();
	while(!solid_isAny(below) && below.z() != 0)
	{
		below = below.below();
		center = below;
	}
	return center;
}
Cuboid Space::getAdjacentWithEdgeAndCornerAdjacent(const Point3D& point) const
{
	Cuboid output = {Point3D(point.data + 1), point.subtractWithMinimum(Distance::create(1))};
	return output.intersection(boundry());
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
Cuboid Space::getAdjacentWithEdgeOnSameZLevelOnly(const Point3D& point) const
{
	Point3D high(point.data + 1);
	high.z() = point.z();
	Point3D low = point.subtractWithMinimum({1});
	low.z() = point.z();
	Cuboid output = {high, low};
	return output.intersection(boundry());
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
	const Cuboid adjacent = point.getAllAdjacentIncludingOutOfBounds();
	const auto condition = [&](const ActorIndex& a){ return a == actor; };
	return m_actors.queryAnyWithCondition(adjacent, condition);
}
bool Space::isAdjacentToItem(const Point3D& point, const ItemIndex& item) const
{
	const Cuboid adjacent = point.getAllAdjacentIncludingOutOfBounds();
	const auto condition = [&](const ItemIndex& i){ return i == item; };
	return m_items.queryAnyWithCondition(adjacent, condition);
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
	if(solid_isAny(to) && !MaterialType::getTransparent(m_solid.queryGetOne(to)))
		return false;
	if(pointFeature_contains(to, PointFeatureTypeId::Door))
		return false;
	// looking up.
	const Cuboid toCuboid{to, to};
	if(to.z() > from.z())
	{
		const PointFeature& floor = pointFeature_at(toCuboid, PointFeatureTypeId::Floor);
		if(floor.exists() && !MaterialType::getTransparent(floor.materialType))
			return false;
		const PointFeature& hatch = pointFeature_at(toCuboid, PointFeatureTypeId::Hatch);
		if(hatch.exists() && !MaterialType::getTransparent(hatch.materialType))
			return false;
	}
	// looking down.
	const Cuboid fromCuboid{from, from};
	if(to.z() < from.z())
	{
		const PointFeature& floor = pointFeature_at(fromCuboid, PointFeatureTypeId::Floor);
		if(floor.exists() && !MaterialType::getTransparent(floor.materialType))
			return false;
		const PointFeature& hatch = pointFeature_at(fromCuboid, PointFeatureTypeId::Hatch);
		if(hatch.exists() && !MaterialType::getTransparent(hatch.materialType))
			return false;
	}
	return true;
}
void Space::moveContentsTo(const Point3D& from, const Point3D& to)
{
	if(solid_isAny(from))
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
void Space::prepareRtrees()
{
	#pragma omp parallel
	#pragma omp single nowait
	{
		#pragma omp task
			for(auto& pair : m_projects)
				pair.second.prepare();
		#pragma omp task
			m_reservables.prepare();
		#pragma omp task
			m_fires.prepare();
		#pragma omp task
			m_solid.prepare();
		#pragma omp task
			m_features.prepare();
		#pragma omp task
			m_mist.prepare();
		#pragma omp task
			m_mistInverseDistanceFromSourceSquared.prepare();
		#pragma omp task
			m_actors.prepare();
		#pragma omp task
			m_items.prepare();
		#pragma omp task
			m_totalFluidVolume.prepare();
		#pragma omp task
			m_fluid.prepare();
		#pragma omp task
			m_plants.prepare();
		#pragma omp task
			m_dynamicVolume.prepare();
		#pragma omp task
			m_staticVolume.prepare();
		#pragma omp task
			m_temperatureDelta.prepare();
		#pragma omp task
			m_visible.prepare();
		#pragma omp task
			m_constructed.prepare();
		#pragma omp task
			m_dynamic.prepare();
		#pragma omp task
			m_support.prepare();
		#pragma omp task
			m_exposedToSky.prepare();
		#pragma omp task
			m_area.m_spaceDesignations.prepare();
	}
}
bool Space::canSeeThrough(const Cuboid& cuboid) const
{
	const MaterialTypeId& material = solid_get(cuboid);
	if(material.exists() && !MaterialType::getTransparent(material))
		return false;
	const PointFeature& door = pointFeature_at(cuboid, PointFeatureTypeId::Door);
	if(door.exists() && door.isClosed() && !MaterialType::getTransparent(door.materialType) )
		return false;
	return true;
}
bool Space::canSeeThrough(const Point3D& point) const { return canSeeThrough(Cuboid{point, point}); }
bool Space::canSeeThroughFloor(const Cuboid& cuboid) const
{
	const PointFeature& floor = pointFeature_at(cuboid, PointFeatureTypeId::Floor);
	if(floor.exists() && !MaterialType::getTransparent(floor.materialType))
		return false;
	const PointFeature& hatch = pointFeature_at(cuboid, PointFeatureTypeId::Hatch);
	if(hatch.exists() && !MaterialType::getTransparent(hatch.materialType) && hatch.isClosed())
		return false;
	return true;
}
bool Space::canSeeThroughFrom(const Point3D& point, const Point3D& otherPoint) const
{
	if(!canSeeThrough({point, point}))
		return false;
	// On the same level.
	auto z = point.z();
	auto otherZ = otherPoint.z();
	if(z == otherZ)
		return true;
	// looking up.
	if(z > otherZ)
	{
		if(!canSeeThroughFloor({point, point}))
			return false;
	}
	// looking down.
	if(z < otherZ)
	{
		if(!canSeeThroughFloor({otherPoint, otherPoint}))
			return false;
	}
	return true;
}
bool Space::isSupport(const Point3D& point) const
{
	return solid_isAny(point) || pointFeature_isSupport(point);
}
bool Space::isExposedToSky(const Point3D& point) const
{
	return m_exposedToSky.check(point);
}
bool Space::isEdge(const Point3D& point) const
{
	return (point.data == 0).any() && (point.data == m_dimensions).any();
}
bool Space::isEdge(const Cuboid& cuboid) const
{
	return isEdge(cuboid.m_high) || isEdge(cuboid.m_low);
}
bool Space::hasLineOfSightTo(const Point3D& point, const Point3D& other) const
{
	return m_area.m_opacityFacade.hasLineOfSight(point, other);
}
Cuboid Space::getZLevel(const Distance& z)
{
	return Cuboid({m_sizeX - 1, m_sizeY - 1, z}, {Distance::create(0), Distance::create(0), z});
}
CuboidSet Space::collectAdjacentsInRange(const Point3D& point, const Distance& range)
{
	auto condition = [&](const Point3D& b){ return b.taxiDistanceTo(point) <= range; };
	return collectAdjacentsWithCondition(point, condition);
}
bool Space::designation_anyForFaction(const FactionId& faction, const SpaceDesignation& designation) const
{
	return m_area.m_spaceDesignations.getForFaction(faction).any(designation);
}
bool Space::designation_has(const Point3D& shape, const FactionId& faction, const SpaceDesignation& designation) const { return m_area.m_spaceDesignations.getForFaction(faction).check(shape, designation); }
bool Space::designation_has(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation) const { return m_area.m_spaceDesignations.getForFaction(faction).check(shape, designation); }
bool Space::designation_has(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation) const { return m_area.m_spaceDesignations.getForFaction(faction).check(shape, designation); }
Point3D Space::designation_hasPoint(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation) const { return m_area.m_spaceDesignations.getForFaction(faction).queryPoint(shape, designation); }
Point3D Space::designation_hasPoint(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation) const { return m_area.m_spaceDesignations.getForFaction(faction).queryPoint(shape, designation); }
void Space::designation_set(const Point3D& shape, const FactionId& faction, const SpaceDesignation& designation) { m_area.m_spaceDesignations.getForFaction(faction).set(shape, designation); }
void Space::designation_set(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation) { m_area.m_spaceDesignations.getForFaction(faction).set(shape, designation); }
void Space::designation_set(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation) { m_area.m_spaceDesignations.getForFaction(faction).set(shape, designation); }
void Space::designation_unset(const Point3D& shape, const FactionId& faction, const SpaceDesignation& designation) { m_area.m_spaceDesignations.getForFaction(faction).unset(shape, designation); }
void Space::designation_unset(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation) { m_area.m_spaceDesignations.getForFaction(faction).unset(shape, designation); }
void Space::designation_unset(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation) { m_area.m_spaceDesignations.getForFaction(faction).unset(shape, designation); }
void Space::designation_maybeUnset(const Point3D& shape, const FactionId& faction, const SpaceDesignation& designation) { m_area.m_spaceDesignations.getForFaction(faction).maybeUnset(shape, designation); }
void Space::designation_maybeUnset(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation) { m_area.m_spaceDesignations.getForFaction(faction).maybeUnset(shape, designation); }
void Space::designation_maybeUnset(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation) { m_area.m_spaceDesignations.getForFaction(faction).maybeUnset(shape, designation); }
bool FluidRTree::canOverlap(const FluidData& a, const FluidData& b) const { return a.type != b.type; }
bool PointFeatureRTree::canOverlap(const PointFeature& a, const PointFeature& b) const { return a.pointFeatureType != b.pointFeatureType; }