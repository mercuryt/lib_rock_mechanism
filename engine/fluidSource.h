#pragma once
#include "types.h"
#include "config.h"
#include <vector>
class FluidType;
class Block;
class Area;
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
	Area& m_area;
	std::vector<FluidSource> m_data;
public:
	AreaHasFluidSources(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void step();
	void create(Block& block, const FluidType& fluidType, Volume level);
	void destroy(Block&);
	[[nodiscard]] bool contains(Block& block) const;
	[[nodiscard]] const FluidSource& at(Block& block) const;
};
