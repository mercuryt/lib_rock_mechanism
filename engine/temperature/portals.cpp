 #include "portals.h"
 #include "../rtreeHelpers/rtreeHelpers.h"
 #include "../config/physics.h"
 #include "../area/area.h"
 #include "../space/space.h"
void AreaHasPortalsBetweenOutSideAndInside::add(Area& area, const CuboidSet& cuboids)
{
	for(const Cuboid cuboid : cuboids)
		add(area, cuboid);
}
void AreaHasPortalsBetweenOutSideAndInside::add(Area& area, const Cuboid cuboid)
{
	const CuboidSet affectedArea = getAffectedArea(area, cuboid);
	if(affectedArea.volume() == 1)
		// A portal which connects to nothing has no effect.
		return;
	m_data.insert(affectedArea, cuboid);
	area.m_hasTemperature.markToUpdate(affectedArea);
}
void AreaHasPortalsBetweenOutSideAndInside::remove(Area& area, const CuboidSet& cuboids)
{
	for(const Cuboid cuboid : cuboids)
		remove(area, cuboid);
}
void AreaHasPortalsBetweenOutSideAndInside::remove(Area& area, const Cuboid cuboid)
{
	auto condition = [&](const Cuboid other){ return other == cuboid; };
	CuboidSet removed = RTreeHelpers::deleteAdjacentWithConditionRecursive(m_data, cuboid, condition);
	area.m_hasTemperature.markToUpdate(removed);
}
CuboidSet AreaHasPortalsBetweenOutSideAndInside::getAffectedArea(const Area& area, const Cuboid cuboid)
{
	const Space& space = area.getSpace();
	TemperatureDelta absoluteDelta = TemperatureDelta::create(std::abs(area.m_hasTemperature.m_ambiant.get() - Config::undergroundAmbiantTemperature.get()));
	if(absoluteDelta < Config::Physics::minimumHeatDeltaToTrackAffectedArea)
		return CuboidSet::create(cuboid);
	// Subtracting one here because the effectDistanceAmbiant calculation includes origin but max range does not.
	int maxRange = std::min(
		absoluteDelta.effectDistanceAmbiant() - 1,
		Config::Physics::rangeOfInfluenceForPortalsBetweenOutsideAndInside
	).get();
	int maxVolume = std::min(
		(DistanceWidth)std::pow((float)(maxRange * 2), 3.f),
		(DistanceWidth)(Config::Physics::maxVolumeOfInfluenceForPortalsBetweenOutsideAndInside.get() * cuboid.volume())
	);
	auto query = [&](const Cuboid queryCuboid) -> CuboidSet {
		CuboidSet output = CuboidSet::create(queryCuboid);
		output.maybeRemoveAll(space.m_exposedToSky.get().queryGetLeaves(output));
		output = space.temperature_queryTransmitsCuboidsIntersection(output);
		output.inflate({1});
		output = output.intersection(queryCuboid);
		output.maybeRemoveAll(space.m_exposedToSky.get().queryGetLeaves(output));
		return output;
	};
	CuboidSet output = RTreeHelpers::findCuboidSetWithMaxVolumeAndMaxRangeStartingFromCuboid(maxVolume, maxRange, cuboid, query);
	output = output.intersection(space.boundry());
	return output;
}
DistanceFractional AreaHasPortalsBetweenOutSideAndInside::queryDistanceToNearest(const Point3D point)
{
	DistanceFractional nearest = DistanceFractional::max();
	m_data.queryForEach(point, [&](const Cuboid other){
		const DistanceFractional distanceToOther = point.distanceToFractional(other);
		if(distanceToOther < nearest)
			nearest = distanceToOther;
	});
	if(nearest == DistanceFractional::max())
		// nothing found
		return DistanceFractional::null();
	return nearest;
}
void AreaHasPortalsBetweenOutSideAndInside::onTemperatureCanNoLongerTransmit(Area& area, const CuboidSet& cuboids)
{
	m_data.queryForEach(cuboids, [&](const Cuboid cuboid){ m_toUpdate.maybeInsert(cuboid); });
	// Find any closed portals.
	CuboidSet adjacent = cuboids.inflated({1});
	maybeRemove(area, adjacent);
}
void AreaHasPortalsBetweenOutSideAndInside::onTemperatureCanNowTransmit(Area& area, const CuboidSet& cuboids)
{
	// This is the same as can no longer transmit. This is correct because heat affects the first layer of a wall despite not passing through it.
	m_data.queryForEach(cuboids, [&](const Cuboid cuboid){ m_toUpdate.maybeInsert(cuboid); });
	// Find any new portals generated.
	CuboidSet adjacent = cuboids.inflated({1});
	CuboidSet portals = getPortals(area, adjacent);
	if(portals.exists())
		maybeAdd(area, portals);
}
void AreaHasPortalsBetweenOutSideAndInside::doStep(Area& area)
{
	for(const Cuboid cuboid : m_toUpdate)
	{
		auto condition = [&](const Cuboid other) { return other == cuboid; };
		CuboidSet recordedArea = RTreeHelpers::getAdjacentWithConditionRecursive(m_data, cuboid, condition);
		m_data.removeWithCondition(recordedArea, condition);
		CuboidSet affectedArea = getAffectedArea(area, cuboid);
		m_data.insert(affectedArea, cuboid);
	}
	m_toUpdate.clear();
}
void AreaHasPortalsBetweenOutSideAndInside::maybeAdd(Area& area, const CuboidSet& cuboids)
{
	CuboidSet candidates = cuboids;
	m_data.queryForEach(cuboids, [&](const Cuboid source) { candidates.maybeRemove(source); });
	CuboidSet portals = getPortals(area, candidates);
	add(area, portals);
}
void AreaHasPortalsBetweenOutSideAndInside::maybeRemove(Area& area, const CuboidSet& cuboids)
{
	CuboidSet candidates = CuboidSet::create(m_data.queryGetAll(cuboids));
	CuboidSet portals = getPortals(area, cuboids);
	candidates.maybeRemoveAll(portals);
	remove(area, candidates);
}
bool AreaHasPortalsBetweenOutSideAndInside::isRecordedAsPortal(const Point3D& point) const
{
	return m_data.queryAnyWithCondition(point, [&](const Cuboid cuboid){ return cuboid.contains(point); });
}
CuboidSet AreaHasPortalsBetweenOutSideAndInside::getPortals(Area& area, const CuboidSet& cuboids)
{
	Space& space = area.getSpace();
	CuboidSet candidates = space.temperature_queryTransmitsCuboidsIntersection(cuboids);
	// Remove exposed from candidates.
	CuboidSet exposed = space.m_exposedToSky.get().queryGetLeaves(candidates.inflated({1}));
	candidates.maybeRemoveAll(exposed);
	if(candidates.empty())
		return {};
	// Remove candidates not adjacent to exposed.
	auto inflatedExposed = exposed;
	space.solid_removeAllFrom(inflatedExposed);
	inflatedExposed.inflate({1});
	candidates = candidates.intersection(inflatedExposed);
	if(candidates.empty())
		return {};
	// Remove candidates not adjacent to a temperture transmiting and non exposed point which is not adjacent to an exposed point.
	CuboidSet candidatesInflated = candidates;
	candidatesInflated.inflate({1});
	CuboidSet adjacentToCandidates = space.temperature_queryTransmitsCuboidsIntersection(candidatesInflated);
	adjacentToCandidates.maybeRemoveAll(candidates);
	adjacentToCandidates.maybeRemoveAll(exposed);
	if(adjacentToCandidates.empty())
		return {};
	adjacentToCandidates.inflate({1});
	candidates = candidates.intersection(adjacentToCandidates);
	// Extend candidates upwards to include solid above the top of the candidate area.
	// This will be used to create "above".
	for(Cuboid& cuboid : candidates)
		if(cuboid.m_high.z() != space.m_sizeZ - 1)
			cuboid.m_high.setZ((cuboid.m_high.z() + 1));
	CuboidSet above = space.solid_queryCuboids(candidates).intersection(candidates);
	// Ignore anything on the bottom most layer of above, below it would be outside of the cuboids paramater.
	Cuboid boundry = cuboids.boundry();
	above.maybeRemove(boundry.getFaceBelow());
	CuboidSet output;
	for(const Cuboid cuboid : above)
		if(cuboid.m_low.z() != 0)
		{
			Point3D high = cuboid.m_high;
			high.setZ(cuboid.m_low.z() - 1);
			Point3D low = cuboid.m_low;
			low.setZ(cuboid.m_low.z() - 1);
			Cuboid shadow = Cuboid::create(high, low);
			output.add(shadow);
		}
	return output;
}
bool AreaHasPortalsBetweenOutSideAndInside::isPortal(Area& area, const Point3D point)
{
	return getPortals(area, CuboidSet::create(point)).exists();
}
TemperatureDelta AreaHasPortalsBetweenOutSideAndInside::getDelta(Area& area, const Point3D point)
{
	DistanceFractional distanceToNearestPortalToOutdoors = queryDistanceToNearest(point);
	if(distanceToNearestPortalToOutdoors.empty())
		return {0};
	TemperatureDelta portalDelta = TemperatureDelta::create(area.m_hasTemperature.m_ambiant.get() - Config::undergroundAmbiantTemperature.get());
	return portalDelta.reduceForDistanceAmbiant(distanceToNearestPortalToOutdoors);
}
int AreaHasPortalsBetweenOutSideAndInside::getToUpdateSize() const { return m_toUpdate.size(); }