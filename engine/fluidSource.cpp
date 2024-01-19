#include "fluidSource.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "block.h"
#include "stockpile.h"
struct FluidSource final
{
	Block* block;
	const FluidType* fluidType;
	Volume level;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(FluidSource, block, fluidType, level);
	FluidSource(Block* b, const FluidType* ft, Volume l) : block(b), fluidType(ft), level(l) { }
	FluidSource(const Json& data, DeserializationMemo& deserializationMemo) : block(&deserializationMemo.blockReference(data["block"])), fluidType(data["fluidType"].get<const FluidType*>()), level(data["level"].get<Volume>()) { }
};
void AreaHasFluidSources::step()
{
	for(FluidSource& source : m_data)
	{
		int delta = source.block->m_totalFluidVolume - source.level;
		if(delta > 0)
			source.block->addFluid(delta, *source.fluidType);
	}

}
void AreaHasFluidSources::create(Block& block, const FluidType& fluidType, Volume level)
{
	assert(std::ranges::find(m_data, &block, &FluidSource::block) == m_data.end());
	m_data.emplace_back(&block, &fluidType, level);
}
void AreaHasFluidSources::destroy(Block& block)
{
	assert(std::ranges::find(m_data, &block, &FluidSource::block) != m_data.end());
	std::ranges::remove(m_data, &block, &FluidSource::block);
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
