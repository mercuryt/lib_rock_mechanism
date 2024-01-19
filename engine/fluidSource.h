#pragma once
#include "deserializationMemo.h"
#include "types.h"
#include "config.h"
#include <vector>
class FluidType;
class Block;
struct FluidSource;
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
