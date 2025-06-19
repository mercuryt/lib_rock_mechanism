#pragma once
#include "../dataStructures/strongVector.h"
#include "../index.h"

class BlocksExposedToSky
{
	StrongBitSet<BlockIndex> m_data;
public:
	void initalizeEmpty();
	void set(Area& area, const BlockIndex& block);
	void unset(Area& area, const BlockIndex& block);
	void resize(const auto& size) { m_data.resize(size); }
	[[nodiscard]] bool check(const BlockIndex& block) const { return m_data[block]; };
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(BlocksExposedToSky, m_data);
};