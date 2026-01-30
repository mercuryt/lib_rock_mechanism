
#include "exteriorPortals.h"
#include "area/area.h"

#include "../space/space.h"
#include "../definitions/moveType.h"
void AreaHasExteriorPortals::initialize()
{
	m_deltas.fill(TemperatureDelta::create(0));
	//updateAmbientSurfaceTemperature(space, area.m_hasTemperature.getAmbientSurfaceTemperature());
}
void AreaHasExteriorPortals::setDistance(Space& space, const Point3D& point, const Distance& distance)
{
	assert(!space.isExposedToSky(point));
	assert(distance.exists());
	assert(distance < m_points.size());
	m_points[distance.get()].insert(point);
	if(distance == 0)
		assert(isPortal(space, point));
	if(m_distances.queryGetOne(point) == distance)
		return;
	TemperatureDelta previousDelta;
	const TemperatureDelta& newDelta = m_deltas[distance.get()];
	if(m_distances.queryGetOne(point).exists())
		previousDelta = m_deltas[m_distances.queryGetOne(point).get()];
	else
		previousDelta = TemperatureDelta::create(0);
	// If the newDelta is greater then previousDelta: deltaDelta will be positive.
	auto deltaDelta = newDelta - previousDelta;
	space.temperature_updateDelta(point, deltaDelta);
	m_distances.maybeInsertOrOverwrite(point, distance);
}
void AreaHasExteriorPortals::unsetDistance(Space& space, const Point3D& point)
{
	const Distance& distance = m_distances.queryGetOne(point);
	assert(distance.exists());
	m_points[distance.get()].erase(point);
	TemperatureDelta deltaDelta = m_deltas[distance.get()] * -1;
	space.temperature_updateDelta(point, deltaDelta);
	m_distances.maybeRemove(point);
}
void AreaHasExteriorPortals::add(Area& area, const Point3D& point, Distance distance )
{
	Space& space = area.getSpace();
	assert(!space.isExposedToSky(point));
	// Distance might already be set if we are regenerating after removing.
	if(m_distances.queryGetOne(point) != distance)
		setDistance(space, point, distance);
	++distance;
	SmallSet<Point3D> currentSet;
	SmallSet<Point3D> nextSet;
	currentSet.insert(point);
	while(distance <= Config::maxDepthExteriorPortalPenetration && !currentSet.empty())
	{
		for(const Point3D& current : currentSet)
		{
			if(m_distances.queryGetOne(current).exists() && m_distances.queryGetOne(current) <= distance)
				continue;
			setDistance(space, current, distance);
			// Don't propigate through solid, but do warm / cool the exterior space.
			if(space.temperature_transmits(current))
				for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(current))
					nextSet.maybeInsert(adjacent);
		}
		currentSet.swap(nextSet);
		nextSet.clear();
		++distance;
	}
}
void AreaHasExteriorPortals::remove(Area& area, const Point3D& point)
{
	Space& space = area.getSpace();
	Distance distance = m_distances.queryGetOne(point);
	assert(distance.exists());
	SmallSet<Point3D> previousSet;
	SmallSet<Point3D> currentSet;
	SmallSet<Point3D> nextSet;
	previousSet.insert(point);
	currentSet.insert(point);
	// Record distances created by other portals, they will be used to regenerate some of the cleared space.
	SmallSet<Point3D> regenerateFrom;
	while(distance <= Config::maxDepthExteriorPortalPenetration)
	{
		for(const Point3D& current : currentSet)
		{
			// If a lower distance value then expected is seen it must be associated with a different source.
			// Record in regenerateFrom instead of unsetting.
			if(m_distances.queryGetOne(current) < distance)
			{
				const Distance& currentDistance = m_distances.queryGetOne(current);
				if(space.temperature_transmits(current) && currentDistance != Config::maxDepthExteriorPortalPenetration && !previousSet.contains(current))
					regenerateFrom.insert(current);
				continue;
			}
			unsetDistance(space, current);
			// Don't propigate through solid, but do warm / cool the exterior space.
			if(!space.solid_isAny(current))
				for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(current))
					if(!m_distances.queryGetOne(adjacent).empty())
						nextSet.maybeInsert(adjacent);
		}
		previousSet.swap(currentSet);
		currentSet.swap(nextSet);
		nextSet.clear();
		++distance;
	}
	for(const Point3D& regenerateFromPoint : regenerateFrom)
		add(area, regenerateFromPoint, m_distances.queryGetOne(regenerateFromPoint));
}
void AreaHasExteriorPortals::onChangeAmbiantSurfaceTemperature(Space& space, const Temperature& temperature)
{
	for(auto distance = Distance::create(0); distance <= Config::maxDepthExteriorPortalPenetration; ++distance)
	{
		TemperatureDelta oldDelta = m_deltas[distance.get()];
		if(oldDelta.empty())
			oldDelta = {0};
		TemperatureDelta newDelta = m_deltas[distance.get()] = getDeltaForAmbientTemperatureAndDistance(temperature, distance);
		if(newDelta == oldDelta)
			return;
		TemperatureDelta deltaDelta = newDelta - oldDelta;
		for(const Point3D& point : m_points[distance.get()])
			space.temperature_updateDelta(point, deltaDelta);
	}
}
void AreaHasExteriorPortals::onPointCanTransmitTemperature(Area& area, const Point3D& point)
{
	Distance distance = m_distances.queryGetOne(point);
	assert(distance != 0);
	Space& space = area.getSpace();
	if(distance.empty())
	{
		if(space.isExposedToSky(point))
			return;
		// Check if we should create a portal here.
		bool create = false;
		Distance lowestAdjacentDistanceToAPortal = Config::maxDepthExteriorPortalPenetration;
		for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(point))
		{
			if(space.isExposedToSky(adjacent) && space.temperature_transmits(adjacent))
			{
				create = true;
				break;
			}
			if(m_distances.queryGetOne(adjacent).exists() && m_distances.queryGetOne(adjacent) < lowestAdjacentDistanceToAPortal)
				lowestAdjacentDistanceToAPortal = m_distances.queryGetOne(adjacent);
		}
		if(!create)
		{
			// Check if we should extend a portal's infulence here.
			if(lowestAdjacentDistanceToAPortal == Config::maxDepthExteriorPortalPenetration)
				return;
			distance = lowestAdjacentDistanceToAPortal + 1;
		}
		else
			distance = Distance::create(0);
	}
	add(area, point, distance);
}
void AreaHasExteriorPortals::onCuboidCanNotTransmitTemperature(Area& area, const Cuboid& cuboid)
{
	CuboidSet hasDistance;
	m_distances.queryForEachCuboid(cuboid, [&](const Cuboid& foundCuboid) { hasDistance.add(foundCuboid); });
	for(const Cuboid& hasDistanceCuboid : hasDistance)
		for(const Point3D& point : hasDistanceCuboid)
			remove(area, point);
}
TemperatureDelta AreaHasExteriorPortals::getDeltaForAmbientTemperatureAndDistance(const Temperature& ambientTemperature, const Distance& distance)
{
	int deltaBetweenSurfaceAndUnderground = ((int)ambientTemperature.get() - (int)Config::undergroundAmbiantTemperature.get());
	int delta = util::scaleByInverseFraction(deltaBetweenSurfaceAndUnderground, distance.get(), Config::maxDepthExteriorPortalPenetration.get() + 1);
	return TemperatureDelta::create(delta);
}
bool AreaHasExteriorPortals::isPortal(const Space& space, const Point3D& point)
{
	assert(space.temperature_transmits(point));
	if(space.isExposedToSky(point))
		return false;
	for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(point))
	{
		if(space.isExposedToSky(adjacent) && space.temperature_transmits(adjacent))
			return true;
	}
	return false;
}