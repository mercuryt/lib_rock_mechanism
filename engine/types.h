// One BlockIndex has xyz dimensions of 1 meter by 1 meter by 2 meters.
// This File defines physical types.
// idTypes.h defines id types, which don't ever change.
// index.h defines index types, which do change and must be updated.

#pragma once
#include "strongFloat.h"
#include "strongInteger.h"
#include "json.h"
#include "idTypes.h"
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
	Step() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Step& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Step& index) { data = index.get(); }
inline void from_json(const Json& data, Step& index) { index = Step::create(data.get<StepWidth>()); }

using TemperatureWidth = uint32_t;
class Temperature : public StrongInteger<Temperature, TemperatureWidth>
{
public:
	Temperature() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Temperature& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Temperature& index) { data = index.get(); }
inline void from_json(const Json& data, Temperature& index) { index = Temperature::create(data.get<TemperatureWidth>()); }

using TemperatureDeltaWidth = int32_t;
class TemperatureDelta : public StrongInteger<TemperatureDelta, TemperatureDeltaWidth>
{
public:
	TemperatureDelta() = default;
	struct Hash { [[nodiscard]] size_t operator()(const TemperatureDelta& index) const { return index.get(); } };
};
inline void to_json(Json& data, const TemperatureDelta& index) { data = index.get(); }
inline void from_json(const Json& data, TemperatureDelta& index) { index = TemperatureDelta::create(data.get<TemperatureDeltaWidth>()); }

using QuantityWidth = uint16_t;
class Quantity : public StrongInteger<Quantity, QuantityWidth>
{
public:
	Quantity() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Quantity& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Quantity& index) { data = index.get(); }
inline void from_json(const Json& data, Quantity& index) { index = Quantity::create(data.get<QuantityWidth>()); }

class FullDisplacement;
using CollisionVolumeWidth = uint32_t;
class CollisionVolume : public StrongInteger<CollisionVolume, CollisionVolumeWidth>
{
public:
	CollisionVolume() = default;
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
	FullDisplacement() = default;
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
	Mass() = default;
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
	Density() = default;
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
	Force() = default;
	Speed operator/(const Mass& mass) const;
	struct Hash { [[nodiscard]] size_t operator()(const Force& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Force& index) { data = index.get(); }
inline void from_json(const Json& data, Force& index) { index = Force::create(data.get<ForceWidth>()); }

using KilometersWidth = uint32_t;
class Kilometers : public StrongInteger<Kilometers, KilometersWidth>
{
public:
	Kilometers() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Kilometers& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Kilometers& index) { data = index.get(); }
inline void from_json(const Json& data, Kilometers& index) { index = Kilometers::create(data.get<KilometersWidth>()); }

using MetersWidth = uint32_t;
class Meters : public StrongInteger<Meters, MetersWidth>
{
public:
	Meters() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Meters& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Meters& index) { data = index.get(); }
inline void from_json(const Json& data, Meters& index) { index = Meters::create(data.get<MetersWidth>()); }

//TODO: DistanceInBlocksSquared, narrow DistanceInBlocks to 16.
class DistanceInBlocksFractional;
using DistanceInBlocksWidth = uint32_t;
class DistanceInBlocks : public StrongInteger<DistanceInBlocks, DistanceInBlocksWidth>
{
public:
	DistanceInBlocks() = default;
	DistanceInBlocks operator+=(int32_t x) { data += x; return *this; }
	[[nodiscard]] constexpr DistanceInBlocksFractional toFloat() const;
	[[nodiscard]] constexpr DistanceInBlocks squared() const { return DistanceInBlocks::create(data * data); }
	struct Hash { [[nodiscard]] size_t operator()(const DistanceInBlocks& index) const { return index.get(); } };
};
inline void to_json(Json& data, const DistanceInBlocks& index) { data = index.get(); }
inline void from_json(const Json& data, DistanceInBlocks& index) { index = DistanceInBlocks::create(data.get<DistanceInBlocksWidth>()); }

class DistanceInBlocksFractional : public StrongFloat<DistanceInBlocksFractional>
{
public:
	DistanceInBlocksFractional() = default;
	DistanceInBlocksFractional operator+=(float x) { data += x; return *this; }
	[[nodiscard]] DistanceInBlocks toInt() const { return DistanceInBlocks::create(std::round(data)); }
	struct Hash { [[nodiscard]] size_t operator()(const DistanceInBlocksFractional& index) const { return index.get(); } };
};
inline void to_json(Json& data, const DistanceInBlocksFractional& index) { data = index.get(); }
inline void from_json(const Json& data, DistanceInBlocksFractional& index) { index = DistanceInBlocksFractional::create(data.get<float>()); }
constexpr DistanceInBlocksFractional DistanceInBlocks::toFloat() const { return DistanceInBlocksFractional::create(data); }
// For use by location buckets.
using DistanceInBucketsWidth = uint32_t;
class DistanceInBuckets : public StrongInteger<DistanceInBuckets, DistanceInBucketsWidth>
{
public:
	DistanceInBuckets() = default;
	DistanceInBuckets operator+=(int32_t x) { data += x; return *this; }
	struct Hash { [[nodiscard]] size_t operator()(const DistanceInBuckets& index) const { return index.get(); } };
};
inline void to_json(Json& data, const DistanceInBuckets& index) { data = index.get(); }
inline void from_json(const Json& data, DistanceInBuckets& index) { index = DistanceInBuckets::create(data.get<DistanceInBucketsWidth>()); }

using OffsetWidth = int32_t;
class Offset : public StrongInteger<Offset, OffsetWidth>
{
public:
	Offset() = default;
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
	Speed() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Speed& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Speed& index) { data = index.get(); }
inline void from_json(const Json& data, Speed& index) { index = Speed::create(data.get<SpeedWidth>()); }

using MoveCostWidth = uint32_t;
class MoveCost : public StrongInteger<MoveCost, MoveCostWidth>
{
public:
	MoveCost() = default;
	struct Hash { [[nodiscard]] size_t operator()(const MoveCost& index) const { return index.get(); } };
};
inline void to_json(Json& data, const MoveCost& index) { data = index.get(); }
inline void from_json(const Json& data, MoveCost& index) { index = MoveCost::create(data.get<MoveCostWidth>()); }

using QualityWidth = uint32_t;
class Quality : public StrongInteger<Quality, QualityWidth>
{
public:
	Quality() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Quality& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Quality& index) { data = index.get(); }
inline void from_json(const Json& data, Quality& index) { index = Quality::create(data.get<QualityWidth>()); }

using PercentWidth = uint32_t;
class Percent : public StrongInteger<Percent, PercentWidth>
{
public:
	Percent() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Percent& index) const { return index.get(); } };
	[[nodiscard]] float ratio() const { return (float)data /100.f; }
};
inline void to_json(Json& data, const Percent& index) { data = index.get(); }
inline void from_json(const Json& data, Percent& index) { index = Percent::create(data.get<PercentWidth>()); }

using CombatScoreWidth = uint16_t;
class CombatScore : public StrongInteger<CombatScore, CombatScoreWidth>
{
public:
	CombatScore() = default;
	struct Hash { [[nodiscard]] size_t operator()(const CombatScore& index) const { return index.get(); } };
};
inline void to_json(Json& data, const CombatScore& index) { data = index.get(); }
inline void from_json(const Json& data, CombatScore& index) { index = CombatScore::create(data.get<CombatScoreWidth>()); }

using SkillLevelWidth = uint16_t;
class SkillLevel : public StrongInteger<SkillLevel, SkillLevelWidth>
{
public:
	SkillLevel() = default;
	struct Hash { [[nodiscard]] size_t operator()(const SkillLevel& index) const { return index.get(); } };
};
inline void to_json(Json& data, const SkillLevel& index) { data = index.get(); }
inline void from_json(const Json& data, SkillLevel& index) { index = SkillLevel::create(data.get<SkillLevelWidth>()); }

using SkillExperiencePointsWidth = uint16_t;
class SkillExperiencePoints : public StrongInteger<SkillExperiencePoints, SkillExperiencePointsWidth>
{
public:
	SkillExperiencePoints() = default;
	struct Hash { [[nodiscard]] size_t operator()(const SkillExperiencePoints& index) const { return index.get(); } };
};
inline void to_json(Json& data, const SkillExperiencePoints& index) { data = index.get(); }
inline void from_json(const Json& data, SkillExperiencePoints& index) { index = SkillExperiencePoints::create(data.get<SkillExperiencePointsWidth>()); }

using AttributeLevelWidth = uint16_t;
class AttributeLevel : public StrongInteger<AttributeLevel, AttributeLevelWidth>
{
public:
	AttributeLevel() = default;
	struct Hash { [[nodiscard]] size_t operator()(const AttributeLevel& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AttributeLevel& index) { data = index.get(); }
inline void from_json(const Json& data, AttributeLevel& index) { index = AttributeLevel::create(data.get<AttributeLevelWidth>()); }

using AttributeLevelBonusOrPenaltyWidth = uint16_t;
class AttributeLevelBonusOrPenalty : public StrongInteger<AttributeLevelBonusOrPenalty, AttributeLevelBonusOrPenaltyWidth>
{
public:
	AttributeLevelBonusOrPenalty() = default;
	struct Hash { [[nodiscard]] size_t operator()(const AttributeLevelBonusOrPenalty& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AttributeLevelBonusOrPenalty& index) { data = index.get(); }
inline void from_json(const Json& data, AttributeLevelBonusOrPenalty& index) { index = AttributeLevelBonusOrPenalty::create(data.get<AttributeLevelBonusOrPenaltyWidth>()); }

using PriorityWidth = uint16_t;
class Priority : public StrongInteger<Priority, PriorityWidth>
{
public:
	Priority() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Priority& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Priority& index) { data = index.get(); }
inline void from_json(const Json& data, Priority& index) { index = Priority::create(data.get<PriorityWidth>()); }

using StaminaWidth = uint16_t;
class Stamina : public StrongInteger<Stamina, StaminaWidth>
{
public:
	Stamina() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Stamina& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Stamina& index) { data = index.get(); }
inline void from_json(const Json& data, Stamina& index) { index = Stamina::create(data.get<StaminaWidth>()); }

using LongRangePathNodeIndexWidth = uint32_t;
class LongRangePathNodeIndex : public StrongInteger<LongRangePathNodeIndex, LongRangePathNodeIndexWidth>
{
public:
	LongRangePathNodeIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const LongRangePathNodeIndex& index) const { return index.get(); } };
};
inline void to_json(Json& data, const LongRangePathNodeIndex& index) { data = index.get(); }
inline void from_json(const Json& data, LongRangePathNodeIndex& index) { index = LongRangePathNodeIndex::create(data.get<uint32_t>()); }
using LongRangePathNodeIndexSet = StrongIntegerSet<LongRangePathNodeIndex>;

class DeckId final : public StrongInteger<DeckId, uint16_t>
{
public:
	DeckId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Step& index) const { return index.get(); } };
};
inline void to_json(Json& data, const DeckId& index) { data = index.get(); }
inline void from_json(const Json& data, DeckId& index) { index = DeckId::create(data.get<uint16_t>()); }