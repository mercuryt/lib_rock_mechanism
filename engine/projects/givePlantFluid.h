#pragma once
#include "../project.h"
#include "../index.h"

class GivePlantFluidProject final : public Project
{
	BlockIndex m_plantLocation;
	CollisionVolume m_volume;
	FluidTypeId m_fluidType;
public:
	GivePlantFluidProject(const BlockIndex& location, Area& area, const FactionId& faction);
	GivePlantFluidProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void onComplete() override;
	void onCancel() override;
	void onDelay() override { cancel(); }
	void offDelay() override { std::unreachable(); }
	void setFluidTypeAndVolume();
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Step getDuration() const override { return Config::stepsPerUnitFluidVolumeGivenToPlant * m_volume.get(); }
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const override { return {}; }
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const override { return {}; }
	[[nodiscard]] std::vector<ActorReference> getActors() const override { return {}; }
	[[nodiscard]] std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const override { return {}; }
	[[nodiscard]] SmallMap<FluidTypeId, CollisionVolume> getFluids() const override { return {{m_fluidType, m_volume}}; }
};