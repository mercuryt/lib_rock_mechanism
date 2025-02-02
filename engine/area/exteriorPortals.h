#pragma once
#include "../types.h"
#include "../index.h"
#include "../dataVector.h"
#include "../config.h"

class Blocks;
class AreaHasExteriorPortals
{
	StrongVector<DistanceInBlocks, BlockIndex> m_distances;
	std::array<SmallSet<BlockIndex>, Config::maxDepthExteriorPortalPenetration.get() + 1> m_blocks;
	std::array<TemperatureDelta, Config::maxDepthExteriorPortalPenetration.get() + 1> m_deltas;
	void setDistance(Blocks& blocks, const BlockIndex& block, const DistanceInBlocks& distance);
	void unsetDistance(Blocks& blocks, const BlockIndex& block);
public:
	void initalize(Area& area);
	void add(Area& area, const BlockIndex& block, DistanceInBlocks distance = DistanceInBlocks::create(0));
	void remove(Area& area, const BlockIndex& block);
	void onChangeAmbiantSurfaceTemperature(Blocks& blocks, const Temperature& temperature);
	void onBlockCanTransmitTemperature(Area& area, const BlockIndex& block);
	void onBlockCanNotTransmitTemperature(Area& area, const BlockIndex& block);
	[[nodiscard]] bool isRecordedAsPortal(const BlockIndex& block) { return m_distances[block] == 0; }
	[[nodiscard]] DistanceInBlocks getDistanceFor(const BlockIndex& block) { return m_distances[block]; }
	[[nodiscard]] static TemperatureDelta getDeltaForAmbientTemperatureAndDistance(const Temperature& ambientTemperature, const DistanceInBlocks& distance);
	[[nodiscard]] static bool isPortal(const Blocks& blocks, const BlockIndex& block);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasExteriorPortals, m_distances, m_blocks, m_deltas);
};