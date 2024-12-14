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

class Volume;
using CollisionVolumeWidth = uint32_t;
class CollisionVolume : public StrongInteger<CollisionVolume, CollisionVolumeWidth>
{
public:
	CollisionVolume() = default;
	[[nodiscard]] Volume toVolume() const;
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
class Volume : public StrongInteger<Volume, VolumeWidth>
{
public:
	Volume() = default;
	[[nodiscard]] Volume operator*(Quantity other) const;
	[[nodiscard]] Mass operator*(Density density) const;
	[[nodiscard]] Volume operator*(VolumeWidth other) const;
	[[nodiscard]] Volume operator*(float other) const;
	[[nodiscard]] CollisionVolume toCollisionVolume() const;
	struct Hash { [[nodiscard]] size_t operator()(const Volume& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Volume& index) { data = index.get(); }
inline void from_json(const Json& data, Volume& index) { index = Volume::create(data.get<VolumeWidth>()); }

using MassWidth = uint32_t;
class Mass : public StrongInteger<Mass, MassWidth>
{
public:
	Mass() = default;
	[[nodiscard]] Mass operator*(Quantity quantity) const;
	[[nodiscard]] Mass operator*(MassWidth other) const;
	[[nodiscard]] Mass operator*(float other) const;
	struct Hash { [[nodiscard]] size_t operator()(const Mass& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Mass& index) { data = index.get(); }
inline void from_json(const Json& data, Mass& index) { index = Mass::create(data.get<MassWidth>()); }

class Density : public StrongFloat<Density>
{
public:
	Density() = default;
	[[nodiscard]] Mass operator*(Volume volume) const;
	[[nodiscard]] Density operator*(float other) const;
	struct Hash { [[nodiscard]] size_t operator()(const Density& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Density& index) { data = index.get(); }
inline void from_json(const Json& data, Density& index) { index = Density::create(data.get<float>()); }

using ForceWidth = uint32_t;
class Force : public StrongInteger<Force, ForceWidth>
{
public:
	Force() = default;
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

class DistanceInBlocksFractional;
using DistanceInBlocksWidth = uint32_t;
class DistanceInBlocks : public StrongInteger<DistanceInBlocks, DistanceInBlocksWidth>
{
public:
	DistanceInBlocks() = default;
	DistanceInBlocks operator+=(int32_t x) { data += x; return *this; }
	[[nodiscard]] DistanceInBlocksFractional toFloat() const;
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

using FacingWidth = uint8_t;
class Facing : public StrongInteger<Facing, FacingWidth>
{
public:
	Facing() = default;
	struct Hash { [[nodiscard]] size_t operator()(const Facing& index) const { return index.get(); } };
};
inline void to_json(Json& data, const Facing& index) { data = index.get(); }
inline void from_json(const Json& data, Facing& index) { index = Facing::create(data.get<FacingWidth>()); }

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
	[[nodiscard]] DistanceInBlocks taxiDistanceTo(const Point3D& other) const
	{
		return DistanceInBlocks::create(
			std::abs((int)x.get() - (int)other.x.get()) + 
			std::abs((int)y.get() - (int)other.y.get()) +
			std::abs((int)z.get() - (int)other.z.get())
		);
	}
	[[nodiscard]] DistanceInBlocks distanceTo(const Point3D& other) const
	{
		DistanceInBlocks squared = distanceSquared(other);
		return DistanceInBlocks::create(pow((double)squared.get(), 0.5));
	}
	[[nodiscard]] DistanceInBlocks distanceSquared(const Point3D& other) const
	{
		return DistanceInBlocks::create(
			pow(std::abs((int)x.get() - (int)other.x.get()), 2) + 
			pow(std::abs((int)y.get() - (int)other.y.get()), 2) +
			pow(std::abs((int)z.get() - (int)other.z.get()), 2)
		);
	}
	[[nodiscard]] std::string toString() const 
	{
		return "(" + std::to_string(x.get()) + "," + std::to_string(y.get()) + "," + std::to_string(z.get()) + ")";
	}
	static const int hilbertOrder = 1;
	[[nodiscard]] uint32_t hilbertNumber()
	{
		int n = hilbertOrder;
		int _x = x.get();
		int _y = y.get();
		int _z = z.get();
		int index = 0;
		int rx, ry, rz;

		for (int i = n - 1; i >= 0; --i)
		{
			int mask = 1 << i;
			rx = (_x & mask) > 0;
			ry = (_y & mask) > 0;
			rz = (_z & mask) > 0;

			index <<= 3;

			if (ry == 0)
			{
				if (rx == 1)
				{
					std::swap(_y, _z);
					std::swap(ry, rz);
				}
				if (rz == 1)
				{
					index |= 1;
					_x ^= mask;
					_z ^= mask;
				}
				if (rx == 1)
				{
					index |= 2;
					_y ^= mask;
					_x ^= mask;
				}
			}
			else
			{
				if (rz == 1)
				{
					index |= 4;
					_x ^= mask;
					_z ^= mask;
				}
				if (rx == 1)
				{
					index |= 5;
					_y ^= mask;
					_x ^= mask;
				}
				if (rz == 0)
				{
					index |= 6;
					std::swap(_y, _z);
					std::swap(ry, rz);
				}
			}
		}
		return index;
	}
	void log() const 
	{
		std::cout << toString() << std::endl;
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
using Point3DSet = SmallSet<Point3D>;
class Area;
class Cuboid;
struct Cube
{
	Point3D center;
	DistanceInBlocks halfWidth;
	[[nodiscard]] uint size() const { return pow(halfWidth.get() * 2, 3); }
	[[nodiscard]] bool intersects(const Cube& other) const
	{
		return
			center.x + halfWidth >= other.center.x.subtractWithMinimum(other.halfWidth) &&
			center.y + halfWidth >= other.center.y.subtractWithMinimum(other.halfWidth) &&
			center.z + halfWidth >= other.center.z.subtractWithMinimum(other.halfWidth) &&
			center.x - halfWidth <= other.center.x + other.halfWidth &&
			center.y - halfWidth <= other.center.y + other.halfWidth &&
			center.z - halfWidth <= other.center.z + other.halfWidth;
	}
	[[nodiscard]] bool intersects(Area& area, const Cuboid& other) const;
	[[nodiscard]] bool intersects(const Point3D high, const Point3D low) const
	{
		return
			center.x + halfWidth >= low.x &&
			center.y + halfWidth >= low.y &&
			center.z + halfWidth >= low.z &&
			center.x.subtractWithMinimum(halfWidth) <= high.x &&
			center.y.subtractWithMinimum(halfWidth) <= high.y &&
			center.z.subtractWithMinimum(halfWidth) <= high.z;
	}
	[[nodiscard]] bool contains(const Cube& other) const
	{
		return
			center.x + halfWidth >= other.center.x + other.halfWidth &&
			center.y + halfWidth >= other.center.y + other.halfWidth &&
			center.z + halfWidth >= other.center.z + other.halfWidth &&
			center.x.subtractWithMinimum(halfWidth) <= other.center.x.subtractWithMinimum(other.halfWidth) &&
			center.y.subtractWithMinimum(halfWidth) <= other.center.y.subtractWithMinimum(other.halfWidth) &&
			center.z.subtractWithMinimum(halfWidth) <= other.center.z.subtractWithMinimum(other.halfWidth);
	}
	[[nodiscard]] bool contains(const Point3D& coordinates) const
	{
		return
			center.x + halfWidth >= coordinates.x &&
			center.y + halfWidth >= coordinates.y &&
			center.z + halfWidth >= coordinates.z &&
			center.x.subtractWithMinimum(halfWidth) <= coordinates.x &&
			center.y.subtractWithMinimum(halfWidth) <= coordinates.y &&
			center.z.subtractWithMinimum(halfWidth) <= coordinates.z;
	}
	[[nodiscard]] bool isContainedBy(Area& area, const Cuboid& cuboid) const;
	[[nodiscard]] bool isContainedBy(const Cube& other) const { return other.contains(*this); }
	[[nodiscard]] bool isContainedBy(const Point3D& highest, const Point3D& lowest) const
	{
		return
			highest.x >= center.x + halfWidth &&
			highest.y >= center.y + halfWidth &&
			highest.z >= center.z + halfWidth &&
			lowest.x <= center.x.subtractWithMinimum(halfWidth) &&
			lowest.y <= center.y.subtractWithMinimum(halfWidth) &&
			lowest.z <= center.z.subtractWithMinimum(halfWidth);
	}
};