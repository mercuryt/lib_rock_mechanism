#pragma once
#include "numericTypes/types.h"
#include "dataStructures/smallMap.h"
#include "dataStructures/rtreeData.h"
#include "geometry/cuboidSet.h"

class Area;

enum class FireStage : uint8_t {Smouldering, Burning, Flaming};
NLOHMANN_JSON_SERIALIZE_ENUM(FireStage, {
		{FireStage::Smouldering, "Smouldering"},
		{FireStage::Burning, "Burning"},
		{FireStage::Flaming, "Flaming"}
});
struct FireDelta
{
	CuboidSet location;
	MaterialTypeId materialType;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(FireDelta, location, materialType);
};
struct FireData final
{
	TemperatureSourceId m_temperatureSource;
	MaterialTypeId m_materialType;
	FireStage m_stage = FireStage::Smouldering;
	bool m_hasPeaked = false;
	struct Primitive
	{
		TemperatureSourceIdWidth temperatureSource;
		MaterialTypeIdWidth materialType;
		FireStage stage;
		bool hasPeaked;
		[[nodiscard]] constexpr std::strong_ordering operator<=>(const Primitive& other) const = default;
		[[nodiscard]] constexpr bool operator==(const Primitive& other) const = default;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Primitive, temperatureSource, materialType, stage, hasPeaked);
	};
	void nextPhase(Area& area, Cuboid cuboid);
	void clear();
	[[nodiscard]] bool empty() const;
	[[nodiscard]] Primitive get() const { return {m_temperatureSource.get(), m_materialType.get(), m_stage, m_hasPeaked}; }
	[[nodiscard]] bool operator==(const FireData&) const = default;
	[[nodiscard]] std::strong_ordering operator<=>(const FireData&) const = default;
	[[nodiscard]] FireDelta createDelta(Cuboid cuboid) const;
	[[nodiscard]] TemperatureDelta getTemperatureDelta() const;
	[[nodiscard]] std::string toS() const;
	static FireData create(Primitive primitive) { return {TemperatureSourceId{primitive.temperatureSource}, MaterialTypeId{primitive.materialType}, primitive.stage, primitive.hasPeaked}; }
	static FireData create(Area& area, Cuboid cuboid, MaterialTypeId materialType, bool hasPeaked = false, FireStage stage = FireStage::Smouldering);
	static FireData null() { return {}; }
	constexpr static Primitive nullPrimitive() { return {TemperatureSourceId::null().get(), MaterialTypeId::null().get(), FireStage::Smouldering, false}; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(FireData, m_temperatureSource, m_materialType, m_stage, m_hasPeaked);
};
struct FireTree final : public RTreeData<FireData, RTreeDataConfigs::canOverlapNoMerge>
{
	[[nodiscard]] bool canOverlap(FireData a, FireData b) const { return a.m_materialType != b.m_materialType; }
};
struct AreaHasFires final
{
	FireTree m_fires;
	SmallMap<Step, std::vector<FireDelta>> m_deltas;
	void doStep(Step step, Area& area);
	void scheduleNextPhase(Step step, FireData fire, Cuboid cuboid);
	void ignite(Area& area, const CuboidSet& cuboidSet, MaterialTypeId materialType);
	void extinguish(Area& area, FireData fire, Cuboid cuboid);
	[[nodiscard]] bool containsDeltas() const { return !m_deltas.empty(); }

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasFires, m_fires, m_deltas);
};
