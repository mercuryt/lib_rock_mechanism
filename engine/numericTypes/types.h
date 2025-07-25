// One Point3D has xyz dimensions of 1 meter by 1 meter by 2 meters.
// This File defines physical types.
// numericTypes/idTypes.h defines id types, which don't ever change.
// numericTypes/index.h defines index types, which do change and must be updated.

#pragma once
#include "strongFloat.h"
#include "strongInteger.h"
#include "json.h"
#include "numericTypes/idTypes.h"
#include <compare>
#include <cstdint>
#include <iostream>
#include <string>

#ifndef NDEBUG
    constexpr bool DEBUG = true;
#else
    constexpr bool DEBUG = false;
#endif

enum class Facing4{North,East,South,West,Null};
enum class Facing6{Below,North,East,South,West,Above,Null};
enum class Facing8{North,NorthEast,East,SouthEast,South,SouthWest,West,NorthWest,Null};
enum class SetLocationAndFacingResult{PermanantlyBlocked,TemporarilyBlocked,Success,Null};
using StepWidth = uint64_t;
class Step : public StrongInteger<Step, StepWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Step& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Step& index) { data = index.get(); }
inline void from_json(const Json& data, Step& index) { index = Step::create(data.get<StepWidth>()); }

using TemperatureWidth = uint32_t;
class Temperature : public StrongInteger<Temperature, TemperatureWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Temperature& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Temperature& index) { data = index.get(); }
inline void from_json(const Json& data, Temperature& index) { index = Temperature::create(data.get<TemperatureWidth>()); }

using TemperatureDeltaWidth = int32_t;
class TemperatureDelta : public StrongInteger<TemperatureDelta, TemperatureDeltaWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const TemperatureDelta& index) const { return index.get(); } };
};
inline void to_json(Json& data, const TemperatureDelta& index) { data = index.get(); }
inline void from_json(const Json& data, TemperatureDelta& index) { index = TemperatureDelta::create(data.get<TemperatureDeltaWidth>()); }

using QuantityWidth = uint16_t;
class Quantity : public StrongInteger<Quantity, QuantityWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Quantity& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Quantity& index) { data = index.get(); }
inline void from_json(const Json& data, Quantity& index) { index = Quantity::create(data.get<QuantityWidth>()); }

class FullDisplacement;
using CollisionVolumeWidth = uint32_t;
class CollisionVolume : public StrongInteger<CollisionVolume, CollisionVolumeWidth>
{
public:
	// TODO: do we need this?
	[[nodiscard]] FullDisplacement toVolume() const;
	[[nodiscard]] CollisionVolume operator*(const Quantity& quantity) const { return CollisionVolume::create(data * quantity.get()); }
	[[nodiscard]] CollisionVolume operator*(CollisionVolumeWidth other) const { return CollisionVolume::create(other * data); }
	[[nodiscard]] CollisionVolume operator*(float other) const { return CollisionVolume::create(other * data); }
	struct Hash { [[nodiscard]] size_t operator()(const CollisionVolume& index) const { return index.get(); } };
};
inline void to_json(Json& data, const CollisionVolume& index) { data = index.get(); }
inline void from_json(const Json& data, CollisionVolume& index) { index = CollisionVolume::create(data.get<CollisionVolumeWidth>()); }

class Mass;
class Density;
using VolumeWidth = uint32_t;
class FullDisplacement : public StrongInteger<FullDisplacement, VolumeWidth>
{
public:
	[[nodiscard]] FullDisplacement operator*(Quantity other) const;
	[[nodiscard]] Mass operator*(Density density) const;
	[[nodiscard]] FullDisplacement operator*(VolumeWidth other) const;
	[[nodiscard]] FullDisplacement operator*(float other) const;
	[[nodiscard]] CollisionVolume toCollisionVolume() const;
	[[nodiscard]] static FullDisplacement createFromCollisionVolume(const CollisionVolume& collisionvolume);
	struct Hash { [[nodiscard]] size_t operator()(const FullDisplacement& index) const { return index.get(); } };
};
inline void to_json(Json& data, const FullDisplacement& index) { data = index.get(); }
inline void from_json(const Json& data, FullDisplacement& index) { index = FullDisplacement::create(data.get<VolumeWidth>()); }

using MassWidth = uint32_t;
class Mass : public StrongInteger<Mass, MassWidth>
{
public:
	[[nodiscard]] Mass operator*(Quantity quantity) const;
	[[nodiscard]] Mass operator*(MassWidth other) const;
	[[nodiscard]] Mass operator*(float other) const;
	[[nodiscard]] Density operator/(const FullDisplacement& displacement) const;
	[[nodiscard]] Mass operator/(const Mass& other) const { return Mass::create(data / other.data); }
	struct Hash { [[nodiscard]] size_t operator()(const Mass& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Mass& index) { data = index.get(); }
inline void from_json(const Json& data, Mass& index) { index = Mass::create(data.get<MassWidth>()); }

class Density : public StrongFloat<Density>
{
public:
	[[nodiscard]] Mass operator*(FullDisplacement volume) const;
	[[nodiscard]] Density operator*(float other) const;
	struct Hash { [[nodiscard]] size_t operator()(const Density& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Density& index) { data = index.get(); }
inline void from_json(const Json& data, Density& index) { index = Density::create(data.get<float>()); }

class Speed;
using ForceWidth = uint32_t;
class Force : public StrongInteger<Force, ForceWidth>
{
public:
	Speed operator/(const Mass& mass) const;
	struct Hash { [[nodiscard]] size_t operator()(const Force& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Force& index) { data = index.get(); }
inline void from_json(const Json& data, Force& index) { index = Force::create(data.get<ForceWidth>()); }

using KilometersWidth = uint32_t;
class Kilometers : public StrongInteger<Kilometers, KilometersWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Kilometers& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Kilometers& index) { data = index.get(); }
inline void from_json(const Json& data, Kilometers& index) { index = Kilometers::create(data.get<KilometersWidth>()); }

using MetersWidth = uint32_t;
class Meters : public StrongInteger<Meters, MetersWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Meters& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Meters& index) { data = index.get(); }
inline void from_json(const Json& data, Meters& index) { index = Meters::create(data.get<MetersWidth>()); }

//TODO: DistanceSquared, narrow Distance to 16.
class DistanceFractional;
using DistanceWidth = uint32_t;
class Distance : public StrongInteger<Distance, DistanceWidth>
{
public:
	Distance operator+=(int32_t x) { data += x; return *this; }
	[[nodiscard]] constexpr DistanceFractional toFloat() const;
	[[nodiscard]] constexpr Distance squared() const { return Distance::create(data * data); }
	struct Hash { [[nodiscard]] size_t operator()(const Distance& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Distance& index) { data = index.get(); }
inline void from_json(const Json& data, Distance& index) { index = Distance::create(data.get<DistanceWidth>()); }

class DistanceFractional : public StrongFloat<DistanceFractional>
{
public:
	DistanceFractional operator+=(float x) { data += x; return *this; }
	[[nodiscard]] Distance toInt() const { return Distance::create(std::round(data)); }
	struct Hash { [[nodiscard]] size_t operator()(const DistanceFractional& index) const { return index.get(); } };
};
inline void to_json(Json& data, const DistanceFractional& index) { data = index.get(); }
inline void from_json(const Json& data, DistanceFractional& index) { index = DistanceFractional::create(data.get<float>()); }
constexpr DistanceFractional Distance::toFloat() const { return DistanceFractional::create(data); }
// For use by location buckets.
using DistanceInBucketsWidth = uint32_t;
class DistanceInBuckets : public StrongInteger<DistanceInBuckets, DistanceInBucketsWidth>
{
public:
	DistanceInBuckets operator+=(int32_t x) { data += x; return *this; }
	struct Hash { [[nodiscard]] size_t operator()(const DistanceInBuckets& index) const { return index.get(); } };
};
inline void to_json(Json& data, const DistanceInBuckets& index) { data = index.get(); }
inline void from_json(const Json& data, DistanceInBuckets& index) { index = DistanceInBuckets::create(data.get<DistanceInBucketsWidth>()); }

using OffsetWidth = int32_t;
class Offset : public StrongInteger<Offset, OffsetWidth>
{
public:
	Offset operator+=(int32_t x) { data += x; return *this; }
	struct Hash { [[nodiscard]] size_t operator()(const Offset& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Offset& index) { data = index.get(); }
inline void from_json(const Json& data, Offset& index) { index = Offset::create(data.get<OffsetWidth>()); }

//using Latitude = double;
//using Longitude = double;
//using Bearing = double;

using SpeedWidth = uint32_t;
class Speed : public StrongInteger<Speed, SpeedWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Speed& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Speed& speed) { data = speed.get(); }
inline void from_json(const Json& data, Speed& speed) { speed = Speed::create(data.get<SpeedWidth>()); }

using EnergyWidth = uint32_t;
class Energy : public StrongInteger<Energy, EnergyWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Energy& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Energy& index) { data = index.get(); }
inline void from_json(const Json& data, Energy& index) { index = Energy::create(data.get<EnergyWidth>()); }

using MoveCostWidth = uint32_t;
class MoveCost : public StrongInteger<MoveCost, MoveCostWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const MoveCost& index) const { return index.get(); } };
};
inline void to_json(Json& data, const MoveCost& index) { data = index.get(); }
inline void from_json(const Json& data, MoveCost& index) { index = MoveCost::create(data.get<MoveCostWidth>()); }

using QualityWidth = uint32_t;
class Quality : public StrongInteger<Quality, QualityWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Quality& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Quality& index) { data = index.get(); }
inline void from_json(const Json& data, Quality& index) { index = Quality::create(data.get<QualityWidth>()); }

using PercentWidth = uint32_t;
class Percent : public StrongInteger<Percent, PercentWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Percent& index) const { return index.get(); } };
	[[nodiscard]] float ratio() const { return (float)data /100.f; }
};
inline void to_json(Json& data, const Percent& index) { data = index.get(); }
inline void from_json(const Json& data, Percent& index) { index = Percent::create(data.get<PercentWidth>()); }

using CombatScoreWidth = uint16_t;
class CombatScore : public StrongInteger<CombatScore, CombatScoreWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const CombatScore& index) const { return index.get(); } };
};
inline void to_json(Json& data, const CombatScore& index) { data = index.get(); }
inline void from_json(const Json& data, CombatScore& index) { index = CombatScore::create(data.get<CombatScoreWidth>()); }

using SkillLevelWidth = uint16_t;
class SkillLevel : public StrongInteger<SkillLevel, SkillLevelWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const SkillLevel& index) const { return index.get(); } };
};
inline void to_json(Json& data, const SkillLevel& index) { data = index.get(); }
inline void from_json(const Json& data, SkillLevel& index) { index = SkillLevel::create(data.get<SkillLevelWidth>()); }

using SkillExperiencePointsWidth = uint16_t;
class SkillExperiencePoints : public StrongInteger<SkillExperiencePoints, SkillExperiencePointsWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const SkillExperiencePoints& index) const { return index.get(); } };
};
inline void to_json(Json& data, const SkillExperiencePoints& index) { data = index.get(); }
inline void from_json(const Json& data, SkillExperiencePoints& index) { index = SkillExperiencePoints::create(data.get<SkillExperiencePointsWidth>()); }

using AttributeLevelWidth = uint16_t;
class AttributeLevel : public StrongInteger<AttributeLevel, AttributeLevelWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const AttributeLevel& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AttributeLevel& index) { data = index.get(); }
inline void from_json(const Json& data, AttributeLevel& index) { index = AttributeLevel::create(data.get<AttributeLevelWidth>()); }

using AttributeLevelBonusOrPenaltyWidth = uint16_t;
class AttributeLevelBonusOrPenalty : public StrongInteger<AttributeLevelBonusOrPenalty, AttributeLevelBonusOrPenaltyWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const AttributeLevelBonusOrPenalty& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AttributeLevelBonusOrPenalty& index) { data = index.get(); }
inline void from_json(const Json& data, AttributeLevelBonusOrPenalty& index) { index = AttributeLevelBonusOrPenalty::create(data.get<AttributeLevelBonusOrPenaltyWidth>()); }

using PriorityWidth = uint16_t;
class Priority : public StrongInteger<Priority, PriorityWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Priority& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Priority& index) { data = index.get(); }
inline void from_json(const Json& data, Priority& index) { index = Priority::create(data.get<PriorityWidth>()); }

using StaminaWidth = uint16_t;
class Stamina : public StrongInteger<Stamina, StaminaWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Stamina& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Stamina& index) { data = index.get(); }
inline void from_json(const Json& data, Stamina& index) { index = Stamina::create(data.get<StaminaWidth>()); }

using LongRangePathNodeIndexWidth = uint32_t;
class LongRangePathNodeIndex : public StrongInteger<LongRangePathNodeIndex, LongRangePathNodeIndexWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const LongRangePathNodeIndex& index) const { return index.get(); } };
};
inline void to_json(Json& data, const LongRangePathNodeIndex& index) { data = index.get(); }
inline void from_json(const Json& data, LongRangePathNodeIndex& index) { index = LongRangePathNodeIndex::create(data.get<uint32_t>()); }

class DeckId final : public StrongInteger<DeckId, uint16_t>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Step& index) const { return index.get(); } };
};
inline void to_json(Json& data, const DeckId& index) { data = index.get(); }
inline void from_json(const Json& data, DeckId& index) { index = DeckId::create(data.get<uint16_t>()); }

enum class PointFeatureTypeId
{
	Door,
	Flap,
	FloodGate,
	Floor,
	FloorGrate,
	Fortification,
	Hatch,
	Keel,
	Ramp,
	Stairs,
	Null
};
NLOHMANN_JSON_SERIALIZE_ENUM(PointFeatureTypeId,
{
	{PointFeatureTypeId::Door, "Door"},
	{PointFeatureTypeId::Flap, "Flap"},
	{PointFeatureTypeId::FloodGate, "FloodGate"},
	{PointFeatureTypeId::Floor, "Floor"},
	{PointFeatureTypeId::FloorGrate, "FloorGrate"},
	{PointFeatureTypeId::Fortification, "Fortification"},
	{PointFeatureTypeId::Hatch, "Hatch"},
	{PointFeatureTypeId::Keel, "Keel"},
	{PointFeatureTypeId::Ramp, "Ramp"},
	{PointFeatureTypeId::Stairs, "Stairs"},
	{PointFeatureTypeId::Null, "Null"}
});

enum class Dimensions {X,Y,Z};