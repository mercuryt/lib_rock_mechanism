#include "fluidSource.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "area/stockpile.h"
#include "area/area.h"
#include "space/space.h"
#include "numericTypes/types.h"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(FluidSource, zone, fluidType, level);
FluidSource::FluidSource(const Json& data, DeserializationMemo&) :
	zone(data["zone"].get<CuboidSet>()), fluidType(data["fluidType"].get<FluidTypeId>()), level(data["level"].get<CollisionVolume>()) { }

void AreaHasFluidSources::doStep()
{
	Space& space = m_area.getSpace();
	for(FluidSource& source : m_data)
	{
		Cuboid boundry = source.zone.boundry();
		CollisionVolume delta = source.level - space.fluid_getTotalVolume(source.zone.intersectionPoint(boundry.getFaceAbove()));
		if(delta > 0)
			space.fluid_add(source.zone, delta.get(), source.fluidType);
		else if(delta < 0)
			space.fluid_remove(source.zone, -delta.get(), source.fluidType);
	}
	m_area.m_hasFluidGroups.clearMerged();
}
void AreaHasFluidSources::create(const CuboidSet& zone, FluidTypeId fluidType, CollisionVolume level)
{
	assert(!contains(zone));
	m_data.emplace_back(zone, fluidType, level);
}
void AreaHasFluidSources::destroy(const CuboidSet& zone)
{
	assert(contains(zone));
	auto found = std::ranges::find_if(m_data, [&zone](const FluidSource& source){ return source.zone.intersects(zone);});
	assert(found != m_data.end());
	(*found) = m_data.back();
	m_data.pop_back();
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
bool AreaHasFluidSources::contains(const CuboidSet& zone) const
{
	for(const FluidSource& source : m_data)
		if(source.zone.intersects(zone))
			return true;
	return false;
}
const FluidSource& AreaHasFluidSources::at(const CuboidSet& zone) const
{
	assert(contains(zone));
	for(const FluidSource& source : m_data)
		if(source.zone.intersects(zone))
			return source;
	std::unreachable();
}
