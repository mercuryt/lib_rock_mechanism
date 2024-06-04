// One BlockIndex has xyz dimensions of 1 meter by 1 meter by 2 meters.
//
#pragma once
#include <cmath>
#include <cstdint>
#include <functional>
using Step = uint64_t;
using Temperature = uint32_t; // degrees kelvin.
using CollisionVolume = uint32_t; // 20 liters, onehundredth of a block.
inline CollisionVolume COLLISION_VOLUME_MAX = UINT32_MAX;
using Volume = uint32_t; // cubic centimeters.
using Mass = uint32_t; // grams.
using Density = float; // grams per cubic centimeter.
using Force = uint32_t;
using Percent = int32_t;
inline Percent nullPercent = INT32_MAX;
using Facing = uint8_t;
inline Facing FACING_MAX = 4;
using AreaId = uint32_t;
using ItemId = uint32_t;
using ActorId = uint32_t;
using HaulSubprojectId = uint32_t;
using ProjectId = uint32_t;
using Kilometers = uint32_t;
using Meters = int32_t;
using Latitude = double;
using Longitude = double;
using Bearing = double;
using DistanceInBlocks = uint32_t;
inline DistanceInBlocks BLOCK_DISTANCE_MAX = UINT32_MAX;
using BlockIndex = uint32_t;
inline BlockIndex BLOCK_INDEX_MAX = UINT32_MAX;
using Speed = uint32_t;
using MoveCost = Speed;
using Quantity = uint32_t;
using VisionFacadeIndex = uint32_t;
inline VisionFacadeIndex VISION_FACADE_INDEX_MAX = UINT32_MAX;
using VisionCuboidId = uint32_t;
inline VisionCuboidId VISION_CUBOID_ID_MAX = UINT32_MAX;
struct Vector3D;
struct Point3D
{
	DistanceInBlocks x = BLOCK_DISTANCE_MAX;
	DistanceInBlocks y = BLOCK_DISTANCE_MAX;
	DistanceInBlocks z = BLOCK_DISTANCE_MAX;
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
using DestinationCondition = std::function<bool(BlockIndex, Facing)>;
using AccessCondition = std::function<bool(BlockIndex, Facing)>;
using OpenListPush = std::function<void(BlockIndex)>;
using OpenListPop = std::function<BlockIndex()>;
using OpenListEmpty = std::function<bool()>;
