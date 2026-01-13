#pragma once
#include "../project.h"
#include "../numericTypes/index.h"
#include "../dataStructures/smallMap.hpp"

class GivePlantFluidProject final : public Project
{
	Point3D m_plantLocation;
	CollisionVolume m_volume;
	FluidTypeId m_fluidType;
public:
	GivePlantFluidProject(const Point3D& location, Area& area, const FactionId& faction);
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
	[[nodiscard]] SkillTypeId getSkill() const;
	[[nodiscard]] std::string description() const;
	[[nodiscard]] SmallMap<FluidTypeId, CollisionVolume> getFluids() const override { return {{m_fluidType, m_volume}}; }
};