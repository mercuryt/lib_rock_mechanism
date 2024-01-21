#pragma once
#include "types.h"
#include "config.h"
#include <vector>
class FluidType;
class Block;
struct DeserializationMemo;
struct FluidSource final
{
	Block* block;
	const FluidType* fluidType;
	Volume level;
	FluidSource(Block* b, const FluidType* ft, Volume l) : block(b), fluidType(ft), level(l) { }
	FluidSource(const Json& data, DeserializationMemo& deserializationMemo);
};
class AreaHasFluidSources final
{
	std::vector<FluidSource> m_data;
public:
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void step();
	void create(Block& block, const FluidType& fluidType, Volume level);
	void destroy(Block&);
};
