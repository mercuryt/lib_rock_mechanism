 #include "temperatureSource.h"
 #include "../rtreeHelpers/rtreeHelpers.h"
 #include "../config/config.h"
 #include "../config/physics.h"
 #include "../space/space.h"
 #include "../area/area.h"
 #include "../actors/actors.h"
 #include "../items/items.h"
 #include "../plants.h"
 #include "../pointFeature.h"
 #include<cmath>
std::string TemperatureSource2::toString() const
{
	return "location: " + m_location.toString() + " id: " + m_id.toString() + " delta: " + m_delta.toString();
}
CuboidSet AreaHasTemperatureSources::getAffectedArea(Area& area, const Point3D location, const TemperatureDelta delta)
{
	int maxRange = delta.effectDistanceRadiant().get();
	int maxVolume = std::pow(maxRange, 3);
	Space& space = area.getSpace();
	auto query = [&](const Cuboid cuboid) -> CuboidSet {
		CuboidSet output = space.temperature_queryTransmitsCuboidsIntersection(CuboidSet::create(cuboid));
		// Include walls, floors, etc.
		output.inflate({1});
		output = output.intersection(cuboid);
		return output;
	};
	CuboidSet output = RTreeHelpers::findCuboidSetWithMaxVolumeAndMaxRangeStartingFromPoint(maxVolume, maxRange, location, query);
	return output;
}
void AreaHasTemperatureSources::updateTemperatureSourceDelta(Area& area, const Point3D location, const TemperatureDelta oldDelta, const TemperatureSourceId id, const TemperatureDelta newDelta)
{
	assert(newDelta != oldDelta);
	auto condition = [&](const TemperatureSource2 other){ return other.m_id == id; };
	CuboidSet removed = RTreeHelpers::deleteAdjacentWithConditionRecursive(m_data, location, condition);
	CuboidSet toAdd = getAffectedArea(area, location, newDelta);
	TemperatureSource2 source = TemperatureSource2::create(location, id, newDelta);
	m_data.insert(toAdd, source);
	toAdd.maybeAddAll(removed);
	area.m_hasTemperature.markToUpdate(toAdd);
}
TemperatureSourceId AreaHasTemperatureSources::addTemperatureSource(Area& area, const Point3D location, const TemperatureDelta delta)
{
	TemperatureSourceId id = getNextId();
	TemperatureSource2 source = TemperatureSource2::create(location, id, delta);
	CuboidSet affectedArea = getAffectedArea(area, location, delta);
	m_data.insert(affectedArea, source);
	area.m_hasTemperature.markToUpdate(affectedArea);
	return id;
}
void AreaHasTemperatureSources::removeTemperatureSource(Area& area, const Point3D location, TemperatureSourceId id)
{
	auto condition = [&](const TemperatureSource2 other){ return other.m_id == id; };
	CuboidSet recordedArea = RTreeHelpers::deleteAdjacentWithConditionRecursive(m_data, location, condition);
	area.m_hasTemperature.markToUpdate(recordedArea);
}
CuboidSet AreaHasTemperatureSources::onChangeAmbiantSurfaceTemperatureReturnIntersection(Area& area)
{
	const CuboidSet intersections = getPointsIntersectingExposedToSky(area);
	area.m_hasTemperature.markToUpdate(intersections);
	return intersections;
}
void AreaHasTemperatureSources::doStep(Area& area)
{
	for(const auto [point, temperatueSourceId] : m_sourcesToUpdate)
	{
		auto condition = [&](const TemperatureSource2 other) { return other.m_id == temperatueSourceId; };
		TemperatureSource2 source = m_data.queryGetOneWithCondition(point, condition);
		CuboidSet recordedArea = RTreeHelpers::getAdjacentWithConditionRecursive(m_data, point, condition);
		m_data.removeWithCondition(recordedArea, condition);
		CuboidSet affectedArea = getAffectedArea(area, point, source.m_delta);
		m_data.insert(affectedArea, source);
		recordedArea.maybeAddAll(affectedArea);
		area.m_hasTemperature.markToUpdate(recordedArea);
	}
	m_sourcesToUpdate.clear();
}
TemperatureDelta AreaHasTemperatureSources::getDelta(const Point3D point)
{
	TemperatureDelta output{0};
	m_data.queryForEach(point, [&](const TemperatureSource2 temperatureSource){
		if(point == temperatureSource.m_location)
			output += temperatureSource.m_delta;
		else
		{
			DistanceFractional distance = point.distanceToFractional(temperatureSource.m_location);
			TemperatureDelta modifiedDelta = temperatureSource.m_delta.reduceForDistanceRadiant(distance);
			output += modifiedDelta;
		}
	});
	return output;
}
TemperatureSourceId AreaHasTemperatureSources::getNextId()
{
	if(m_unusedIds.empty())
		return m_nextId++;
	else
	{
		TemperatureSourceId output = m_unusedIds.back();
		m_unusedIds.pop_back();
		return output;
	}
}
void AreaHasTemperatureSources::releaseId(TemperatureSourceId id)
{
	m_unusedIds.push_back(id);
}
void AreaHasTemperatureSources::onTemperatureCanNoLongerTransmit(const CuboidSet& cuboids)
{
	// All sources which effect this area need to have the area of effect recalculated.
	const auto sources = m_data.queryGetAll(cuboids);
	for(const auto& source : sources)
		m_sourcesToUpdate.emplace_back(source.m_location, source.m_id);
}
void AreaHasTemperatureSources::onTemperatureCanNowTransmit(const CuboidSet& cuboids)
{
	// This is the same as can no longer transmit. This is correct because heat affects the first layer of a wall despite not passing through it.
	onTemperatureCanNoLongerTransmit(cuboids);
}
CuboidSet AreaHasTemperatureSources::getPointsIntersectingExposedToSky(Area& area) const
{
	Space &space = area.getSpace();
	CuboidSet output;
	space.m_exposedToSky.get().forEachCuboid([&](const Cuboid exposedCuboid){
		m_data.queryForEachCuboid(exposedCuboid, [&](const Cuboid cuboid){
			output.maybeAdd(cuboid);
		});
	});
	output = space.m_exposedToSky.get().queryGetIntersection(output);
	return output;
}
std::string AreaHasTemperatureSources::toString(Area& area, int x, int y, int z)
{
	Point3D location = Point3D::create(x, y, z);
	TemperatureSource2 source = m_data.queryGetOneWithCondition(location, [&](const TemperatureSource2 otherSource) { return otherSource.m_location == location; });
	CuboidSet affectedArea = getAffectedArea(area, location, source.m_delta);
	return source.toString() + " affecting: " + affectedArea.toString();
}