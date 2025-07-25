#include "fluidSource.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "area/stockpile.h"
#include "area/area.h"
#include "space/space.h"
#include "numericTypes/types.h"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(FluidSource, point, fluidType, level);
FluidSource::FluidSource(const Json& data, DeserializationMemo&) :
	point(data["point"].get<Point3D>()), fluidType(data["fluidType"].get<FluidTypeId>()), level(data["level"].get<CollisionVolume>()) { }

void AreaHasFluidSources::doStep()
{
	Space& space = m_area.getSpace();
	for(FluidSource& source : m_data)
	{
		CollisionVolume delta = source.level - space.fluid_getTotalVolume(source.point);
		if(delta > 0)
			space.fluid_add(source.point, delta, source.fluidType);
		else if(delta < 0)
			//TODO: can this be changed to use the async version?
			space.fluid_removeSyncronus(source.point, -delta, source.fluidType);
	}
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
void AreaHasFluidSources::create(Point3D point, FluidTypeId fluidType, CollisionVolume level)
{
	assert(!contains(point));
	m_data.emplace_back(point, fluidType, level);
}
void AreaHasFluidSources::destroy(Point3D point)
{
	assert(std::ranges::find(m_data, point, &FluidSource::point) != m_data.end());
	std::ranges::remove(m_data, point, &FluidSource::point);
}
void AreaHasFluidSources::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& sourceData : data)
		m_data.emplace_back(sourceData, deserializationMemo);
}
Json AreaHasFluidSources::toJson() const
{
	Json data = Json::array();
	for(const FluidSource& source : m_data)
		data.push_back(source);
	return data;
}
bool AreaHasFluidSources::contains(Point3D point) const
{
	return std::ranges::find(m_data, point, &FluidSource::point) != m_data.end();
}
const FluidSource& AreaHasFluidSources::at(Point3D point) const
{
	assert(contains(point));
	return *std::ranges::find(m_data, point, &FluidSource::point);
}
