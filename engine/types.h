// One BlockIndex has xyz dimensions of 1 meter by 1 meter by 2 meters.
//
#pragma once
#include "strongFloat.h"
#include "strongInteger.h"
#include "json.h"
#include <cmath>
#include <compare>
#include <cstdint>
#include <functional>
#include <iostream>
#include <variant>

class Step : public StrongInteger<Step, uint64_t>
{
public:
	Step() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Step& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Step& index) { data = index.get(); }
inline void from_json(const Json& data, Step& index) { index = Step::create(data.get<uint64_t>()); }

class Temperature : public StrongInteger<Temperature, uint32_t>
{
public:
	Temperature() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Temperature& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Temperature& index) { data = index.get(); }
inline void from_json(const Json& data, Temperature& index) { index = Temperature::create(data.get<uint32_t>()); }

class TemperatureDelta : public StrongInteger<TemperatureDelta, int32_t>
{
public:
	TemperatureDelta() = default;
	struct Hash { [[nodiscard]] size_t operator()(const TemperatureDelta& index) const { return index.get(); } };
};
inline void to_json(Json& data, const TemperatureDelta& index) { data = index.get(); }
inline void from_json(const Json& data, TemperatureDelta& index) { index = TemperatureDelta::create(data.get<int32_t>()); }

class Quantity : public StrongInteger<Quantity, uint32_t>
{
public:
	Quantity() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Quantity& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Quantity& index) { data = index.get(); }
inline void from_json(const Json& data, Quantity& index) { index = Quantity::create(data.get<uint32_t>()); }

class Volume;
class CollisionVolume : public StrongInteger<CollisionVolume, uint32_t>
{
public:
	CollisionVolume() = default;
	[[nodiscard]] Volume toVolume() const;
	[[nodiscard]] CollisionVolume operator*(const Quantity& quantity) const { return CollisionVolume::create(data * quantity.get()); }
	[[nodiscard]] CollisionVolume operator*(uint32_t other) const { return CollisionVolume::create(other * data); }
	[[nodiscard]] CollisionVolume operator*(float other) const { return CollisionVolume::create(other * data); }
	struct Hash { [[nodiscard]] size_t operator()(const CollisionVolume& index) const { return index.get(); } };
};
inline void to_json(Json& data, const CollisionVolume& index) { data = index.get(); }
inline void from_json(const Json& data, CollisionVolume& index) { index = CollisionVolume::create(data.get<uint32_t>()); }

class Mass;
class Density;
class Volume : public StrongInteger<Volume, uint32_t>
{
public:
	Volume() = default;
	[[nodiscard]] Volume operator*(Quantity other) const;
	[[nodiscard]] Mass operator*(Density density) const;
	[[nodiscard]] Volume operator*(uint32_t other) const;
	[[nodiscard]] Volume operator*(float other) const;
	[[nodiscard]] CollisionVolume toCollisionVolume() const;
	struct Hash { [[nodiscard]] size_t operator()(const Volume& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Volume& index) { data = index.get(); }
inline void from_json(const Json& data, Volume& index) { index = Volume::create(data.get<uint32_t>()); }

class Mass : public StrongInteger<Mass, uint32_t>
{
public:
	Mass() = default;
	[[nodiscard]] Mass operator*(Quantity quantity) const;
	[[nodiscard]] Mass operator*(uint32_t other) const;
	[[nodiscard]] Mass operator*(float other) const;
	struct Hash { [[nodiscard]] size_t operator()(const Mass& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Mass& index) { data = index.get(); }
inline void from_json(const Json& data, Mass& index) { index = Mass::create(data.get<uint32_t>()); }

class Density : public StrongInteger<Density, uint32_t>
{
public:
	Density() = default;
	[[nodiscard]] Mass operator*(Volume volume) const;
	[[nodiscard]] Density operator*(uint32_t other) const;
	[[nodiscard]] Density operator*(float other) const;
	struct Hash { [[nodiscard]] size_t operator()(const Density& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Density& index) { data = index.get(); }
inline void from_json(const Json& data, Density& index) { index = Density::create(data.get<uint32_t>()); }

class Force : public StrongInteger<Force, uint32_t>
{
public:
	Force() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Force& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Force& index) { data = index.get(); }
inline void from_json(const Json& data, Force& index) { index = Force::create(data.get<uint32_t>()); }

class Kilometers : public StrongInteger<Kilometers, uint32_t>
{
public:
	Kilometers() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Kilometers& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Kilometers& index) { data = index.get(); }
inline void from_json(const Json& data, Kilometers& index) { index = Kilometers::create(data.get<uint32_t>()); }

class Meters : public StrongInteger<Meters, uint32_t>
{
public:
	Meters() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Meters& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Meters& index) { data = index.get(); }
inline void from_json(const Json& data, Meters& index) { index = Meters::create(data.get<uint32_t>()); }

class DistanceInBlocksFractional;
class DistanceInBlocks : public StrongInteger<DistanceInBlocks, uint32_t>
{
public:
	DistanceInBlocks() = default;
	DistanceInBlocks operator+=(int32_t x) { data += x; return *this; }
	[[nodiscard]] DistanceInBlocksFractional toFloat() const;
	struct Hash { [[nodiscard]] size_t operator()(const DistanceInBlocks& index) const { return index.get(); } };
};
inline void to_json(Json& data, const DistanceInBlocks& index) { data = index.get(); }
inline void from_json(const Json& data, DistanceInBlocks& index) { index = DistanceInBlocks::create(data.get<uint32_t>()); }

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

// For use by location buckets.
class DistanceInBuckets : public StrongInteger<DistanceInBuckets, uint32_t>
{
public:
	DistanceInBuckets() = default;
	DistanceInBuckets operator+=(int32_t x) { data += x; return *this; }
	struct Hash { [[nodiscard]] size_t operator()(const DistanceInBuckets& index) const { return index.get(); } };
};
inline void to_json(Json& data, const DistanceInBuckets& index) { data = index.get(); }
inline void from_json(const Json& data, DistanceInBuckets& index) { index = DistanceInBuckets::create(data.get<uint32_t>()); }
//using Latitude = double;
//using Longitude = double;
//using Bearing = double;
class Speed : public StrongInteger<Speed, uint32_t>
{
public:
	Speed() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Speed& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Speed& index) { data = index.get(); }
inline void from_json(const Json& data, Speed& index) { index = Speed::create(data.get<uint32_t>()); }
class MoveCost : public StrongInteger<MoveCost, uint32_t>
{
public:
	MoveCost() = default;
	struct Hash { [[nodiscard]] size_t operator()(const MoveCost& index) const { return index.get(); } };
};
inline void to_json(Json& data, const MoveCost& index) { data = index.get(); }
inline void from_json(const Json& data, MoveCost& index) { index = MoveCost::create(data.get<uint32_t>()); }

class Quality : public StrongInteger<Quality, uint32_t>
{
public:
	Quality() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Quality& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Quality& index) { data = index.get(); }
inline void from_json(const Json& data, Quality& index) { index = Quality::create(data.get<uint32_t>()); }

class Percent : public StrongInteger<Percent, uint32_t>
{
public:
	Percent() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Percent& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Percent& index) { data = index.get(); }
inline void from_json(const Json& data, Percent& index) { index = Percent::create(data.get<uint32_t>()); }

class Facing : public StrongInteger<Facing, uint8_t>
{
public:
	Facing() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Facing& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Facing& index) { data = index.get(); }
inline void from_json(const Json& data, Facing& index) { index = Facing::create(data.get<uint8_t>()); }

class AreaId : public StrongInteger<AreaId, uint32_t>
{
public:
	AreaId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const AreaId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AreaId& index) { data = index.get(); }
inline void from_json(const Json& data, AreaId& index) { index = AreaId::create(data.get<uint32_t>()); }

template<typename T>
using AreaIdMap = std::unordered_map<AreaId, T, AreaId::Hash>;
class ItemId : public StrongInteger<ItemId, uint32_t>
{
public:
	ItemId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const ItemId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const ItemId& index) { data = index.get(); }
inline void from_json(const Json& data, ItemId& index) { index = ItemId::create(data.get<uint32_t>()); }

template<typename T>
using ItemIdMap = std::unordered_map<ItemId, T, ItemId::Hash>;
class ActorId : public StrongInteger<ActorId, uint32_t>
{
public:
	ActorId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const ActorId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const ActorId& index) { data = index.get(); }
inline void from_json(const Json& data, ActorId& index) { index = ActorId::create(data.get<uint32_t>()); }

template<typename T>
using ActorIdMap = std::unordered_map<ActorId, T, ActorId::Hash>;
class VisionCuboidId : public StrongInteger<VisionCuboidId, uint32_t>
{
public:
	VisionCuboidId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const VisionCuboidId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const VisionCuboidId& index) { data = index.get(); }
inline void from_json(const Json& data, VisionCuboidId& index) { index = VisionCuboidId::create(data.get<uint32_t>()); }

class FactionId : public StrongInteger<FactionId, uint32_t>
{
public:
	FactionId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const FactionId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const FactionId& index) { data = index.get(); }
inline void from_json(const Json& data, FactionId& index) { index = FactionId::create(data.get<uint16_t>()); }

class CombatScore : public StrongInteger<CombatScore, uint16_t>
{
public:
	CombatScore() = default;
	struct Hash { [[nodiscard]] size_t operator()(const CombatScore& index) const { return index.get(); } };
};
inline void to_json(Json& data, const CombatScore& index) { data = index.get(); }
inline void from_json(const Json& data, CombatScore& index) { index = CombatScore::create(data.get<uint16_t>()); }

class SkillLevel : public StrongInteger<SkillLevel, uint16_t>
{
public:
	SkillLevel() = default;
	struct Hash { [[nodiscard]] size_t operator()(const SkillLevel& index) const { return index.get(); } };
};
inline void to_json(Json& data, const SkillLevel& index) { data = index.get(); }
inline void from_json(const Json& data, SkillLevel& index) { index = SkillLevel::create(data.get<uint16_t>()); }

class SkillExperiencePoints : public StrongInteger<SkillExperiencePoints, uint16_t>
{
public:
	SkillExperiencePoints() = default;
	struct Hash { [[nodiscard]] size_t operator()(const SkillExperiencePoints& index) const { return index.get(); } };
};
inline void to_json(Json& data, const SkillExperiencePoints& index) { data = index.get(); }
inline void from_json(const Json& data, SkillExperiencePoints& index) { index = SkillExperiencePoints::create(data.get<uint16_t>()); }


class AttributeLevel : public StrongInteger<AttributeLevel, uint16_t>
{
public:
	AttributeLevel() = default;
	struct Hash { [[nodiscard]] size_t operator()(const AttributeLevel& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AttributeLevel& index) { data = index.get(); }
inline void from_json(const Json& data, AttributeLevel& index) { index = AttributeLevel::create(data.get<uint16_t>()); }

class AttributeLevelBonusOrPenalty : public StrongInteger<AttributeLevelBonusOrPenalty, int16_t>
{
public:
	AttributeLevelBonusOrPenalty() = default;
	struct Hash { [[nodiscard]] size_t operator()(const AttributeLevelBonusOrPenalty& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AttributeLevelBonusOrPenalty& index) { data = index.get(); }
inline void from_json(const Json& data, AttributeLevelBonusOrPenalty& index) { index = AttributeLevelBonusOrPenalty::create(data.get<uint16_t>()); }

class Priority : public StrongInteger<Priority, int16_t>
{
public:
	Priority() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Priority& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Priority& index) { data = index.get(); }
inline void from_json(const Json& data, Priority& index) { index = Priority::create(data.get<uint16_t>()); }

class Stamina : public StrongInteger<Stamina, int16_t>
{
public:
	Stamina() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Stamina& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Stamina& index) { data = index.get(); }
inline void from_json(const Json& data, Stamina& index) { index = Stamina::create(data.get<uint16_t>()); }

// The offset required to lookup in one block's getAdjacentUnfiltered to find another block.
class AdjacentIndex : public StrongInteger<AdjacentIndex, int8_t>
{
public:
	AdjacentIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const AdjacentIndex& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AdjacentIndex& index) { data = index.get(); }
inline void from_json(const Json& data, AdjacentIndex& index) { index = AdjacentIndex::create(data.get<uint8_t>()); }

class HarvestDataTypeId : public StrongInteger<HarvestDataTypeId, int8_t>
{
public:
	HarvestDataTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const HarvestDataTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const HarvestDataTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, HarvestDataTypeId& index) { index = HarvestDataTypeId::create(data.get<uint8_t>()); }

class PlantSpeciesId : public StrongInteger<PlantSpeciesId, int8_t>
{
public:
	PlantSpeciesId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const PlantSpeciesId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const PlantSpeciesId& index) { data = index.get(); }
inline void from_json(const Json& data, PlantSpeciesId& index) { index = PlantSpeciesId::create(data.get<uint8_t>()); }

class AnimalSpeciesId : public StrongInteger<AnimalSpeciesId, int8_t>
{
public:
	AnimalSpeciesId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const AnimalSpeciesId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AnimalSpeciesId& index) { data = index.get(); }
inline void from_json(const Json& data, AnimalSpeciesId& index) { index = AnimalSpeciesId::create(data.get<uint8_t>()); }

class MaterialTypeId : public StrongInteger<MaterialTypeId, int8_t>
{
public:
	MaterialTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const MaterialTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const MaterialTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, MaterialTypeId& index) { index = MaterialTypeId::create(data.get<uint8_t>()); }

class MaterialCategoryTypeId : public StrongInteger<MaterialCategoryTypeId, int8_t>
{
public:
	MaterialCategoryTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const MaterialCategoryTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const MaterialCategoryTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, MaterialCategoryTypeId& index) { index = MaterialCategoryTypeId::create(data.get<uint8_t>()); }

class SpoilsDataTypeId : public StrongInteger<SpoilsDataTypeId, int8_t>
{
public:
	SpoilsDataTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const SpoilsDataTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const SpoilsDataTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, SpoilsDataTypeId& index) { index = SpoilsDataTypeId::create(data.get<uint8_t>()); }

class BurnDataTypeId : public StrongInteger<BurnDataTypeId, int8_t>
{
public:
	BurnDataTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const BurnDataTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const BurnDataTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, BurnDataTypeId& index) { index = BurnDataTypeId::create(data.get<uint8_t>()); }

class MaterialConstructionDataTypeId : public StrongInteger<MaterialConstructionDataTypeId, int8_t>
{
public:
	MaterialConstructionDataTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const MaterialConstructionDataTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const MaterialConstructionDataTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, MaterialConstructionDataTypeId& index) { index = MaterialConstructionDataTypeId::create(data.get<uint8_t>()); }

class ItemTypeId : public StrongInteger<ItemTypeId, int8_t>
{
public:
	ItemTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const ItemTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const ItemTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, ItemTypeId& index) { index = ItemTypeId::create(data.get<uint8_t>()); }

class FluidTypeId : public StrongInteger<FluidTypeId, int8_t>
{
public:
	FluidTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const FluidTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const FluidTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, FluidTypeId& index) { index = FluidTypeId::create(data.get<uint8_t>()); }

class SkillTypeId : public StrongInteger<SkillTypeId, int8_t>
{
public:
	SkillTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const SkillTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const SkillTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, SkillTypeId& index) { index = SkillTypeId::create(data.get<uint8_t>()); }

class MoveTypeId : public StrongInteger<MoveTypeId, int8_t>
{
public:
	MoveTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const MoveTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const MoveTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, MoveTypeId& index) { index = MoveTypeId::create(data.get<uint8_t>()); }

class BodyTypeId : public StrongInteger<BodyTypeId, int8_t>
{
public:
	BodyTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const BodyTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const BodyTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, BodyTypeId& index) { index = BodyTypeId::create(data.get<uint8_t>()); }

class BodyPartTypeId : public StrongInteger<BodyPartTypeId, int8_t>
{
public:
	BodyPartTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const BodyPartTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const BodyPartTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, BodyPartTypeId& index) { index = BodyPartTypeId::create(data.get<uint8_t>()); }

class CraftStepTypeCategoryId : public StrongInteger<CraftStepTypeCategoryId, int8_t>
{
public:
	CraftStepTypeCategoryId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const CraftStepTypeCategoryId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const CraftStepTypeCategoryId& index) { data = index.get(); }
inline void from_json(const Json& data, CraftStepTypeCategoryId& index) { index = CraftStepTypeCategoryId::create(data.get<uint8_t>()); }

class AttackTypeId : public StrongInteger<AttackTypeId, int8_t>
{
public:
	AttackTypeId() = default;
	struct Hash { [[nodiscard]] size_t operator()(const AttackTypeId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AttackTypeId& index) { data = index.get(); }
inline void from_json(const Json& data, AttackTypeId& index) { index = AttackTypeId::create(data.get<uint8_t>()); }

template<typename T>
using FactionIdMap = std::unordered_map<FactionId, T, FactionId::Hash>;
using FactionIdSet = StrongIntegerSet<FactionId>;
//using HaulSubprojectId = uint32_t;
//using ProjectId = uint32_t;
struct Vector3D;
struct Point3D
{
	DistanceInBlocks x;
	DistanceInBlocks y;
	DistanceInBlocks z;
	[[nodiscard]] bool operator==(const Point3D& other) const 
	{
		return x == other.x && y == other.y && z == other.z;
	}
	[[nodiscard]] bool operator!=(const Point3D& other) const 
	{
		return !(*this == other);
	}
	[[nodiscard]] auto operator<=>(const Point3D& other) const 
	{
		if (x != other.x)
			return x <=> other.x;
		else if (y != other.y)
			return y <=> other.y;
		else
			return z <=> other.z;
	}
	void operator+=(const Vector3D& other);
	void log()
	{
		std::cout << "(" << x.get() << "," << y.get() << "," << z.get() << ")";
	}
};
struct Point3D_fractional
{
	DistanceInBlocksFractional x;
	DistanceInBlocksFractional y;
	DistanceInBlocksFractional z;
};
struct Vector3D
{
	int32_t x = INT32_MAX;
	int32_t y = INT32_MAX;
	int32_t z = INT32_MAX;
};
inline void Point3D::operator+=(const Vector3D& other)
{
	x += other.x;
	y += other.y;
	z += other.z;
}
