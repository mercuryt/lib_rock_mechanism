#include "fluidSource.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "stockpile.h"
#include "area.h"
#include "blocks/blocks.h"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(FluidSource, block, fluidType, level);
FluidSource::FluidSource(const Json& data, DeserializationMemo&) : 
	block(data["block"].get<BlockIndex>()), fluidType(data["fluidType"].get<const FluidType*>()), level(data["level"].get<Volume>()) { }

void AreaHasFluidSources::doStep()
{
	Blocks& blocks = m_area.getBlocks();
	for(FluidSource& source : m_data)
	{
		int delta =  source.level - blocks.fluid_getTotalVolume(source.block);
		if(delta > 0)
			blocks.fluid_add(source.block, delta, *source.fluidType);
		else if(delta < 0)
			//TODO: can this be changed to use the async version?
			blocks.fluid_removeSyncronus(source.block, -delta, *source.fluidType);
	}
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
void AreaHasFluidSources::create(BlockIndex block, const FluidType& fluidType, Volume level)
{
	assert(!contains(block));
	m_data.emplace_back(block, &fluidType, level);
}
void AreaHasFluidSources::destroy(BlockIndex block)
{
	assert(std::ranges::find(m_data, block, &FluidSource::block) != m_data.end());
	std::ranges::remove(m_data, block, &FluidSource::block);
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
bool AreaHasFluidSources::contains(BlockIndex block) const
{
	return std::ranges::find(m_data, block, &FluidSource::block) != m_data.end();
}
const FluidSource& AreaHasFluidSources::at(BlockIndex block) const
{
	assert(contains(block));
	return *std::ranges::find(m_data, block, &FluidSource::block);
}
