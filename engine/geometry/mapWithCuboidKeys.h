#pragma once

#include "cuboidSet.h"
#include "../dataStructures/smallSet.h"
#include "../json.h"

template<typename T, typename CuboidType, typename CuboidSetType>
struct MapWithCuboidKeysBase
{
	using PointType = typename CuboidType::PointType;
	using This = MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>;
	using Pair = std::pair<CuboidType, T>;
	SmallSet<Pair> data;
	using ConstIterator = SmallSet<Pair>::const_iterator;
	using Iterator = SmallSet<Pair>::iterator;
	// If the cuboid is adjacent to another such that they can merge with the resulting cuboid having a volume equal to the sum of the two merged cuboid's volumes and where the two cuboids have the same value then overwrite the existing cuboid with the merged cuboid. Otherwise add to end.
	void insertOrMerge(const CuboidType& cuboid, const T& value);
	void insertOrMerge(const Pair& pair);
	void insert(const CuboidType& cuboid, const T& value);
	void insert(const Pair& pair);
	void insertAll(const This& other);
	void maybeRemove(const CuboidType& cuboid);
	void maybeRemoveAll(const CuboidSetType& cuboids);
	void clear();
	void reserve(const int& capacity) { data.reserve(capacity); }
	void sort();
	[[nodiscard]] ConstIterator begin() const { return data.begin(); }
	[[nodiscard]] ConstIterator end() const { return data.end(); }
	[[nodiscard]] Iterator begin() { return data.begin(); }
	[[nodiscard]] Iterator end() { return data.end(); }
	[[nodiscard]] auto size() const { return data.size(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] T& operator[](const CuboidType& cuboid);
	[[nodiscard]] Iterator find(const CuboidType& cuboid);
	[[nodiscard]] CuboidType boundry() const;
	[[nodiscard]] CuboidSetType getCuboids() const;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Offset3D& point) const;
	[[nodiscard]] Pair front() const { return data.front(); }
	[[nodiscard]] CuboidSetType makeCuboidSet() const;
	[[nodiscard]] Quantity volume() const;
	[[nodiscard]] std::string toString() const;
};

template<typename T>
struct MapWithCuboidKeys final : public MapWithCuboidKeysBase<T, Cuboid, CuboidSet> { };
template<typename T>
struct MapWithOffsetCuboidKeys final : public MapWithCuboidKeysBase<T, OffsetCuboid, OffsetCuboidSet>
{
	//TODO: this is defined here rather then the base class to simplify returning the derived type, but the logic could be useful for the non offset variant as well.
	[[nodiscard]] MapWithOffsetCuboidKeys<T> shiftAndRotateAndRemoveIntersectionWithOriginal(const Offset3D& offset, const Facing4& previousRotation, const Facing4& newRotation) const;
	[[nodiscard]] MapWithOffsetCuboidKeys<T> applyOffset(const Offset3D& offset) const;
	[[nodiscard]] MapWithCuboidKeys<T> relativeTo(const Point3D& location) const;
};

template<typename T>
void to_json(Json& data, const MapWithCuboidKeys<T>& map)
{
	for(const auto& [cuboid, value] : map)
		data.push_back({cuboid, value});
}
template<typename T>
void from_json(const Json& data, MapWithCuboidKeys<T>& map)
{
	for(const Json& pair : data)
	{
		Cuboid cuboid = (pair[0].size() == 2) ?
			Cuboid::fromPointPair(pair[0][0].get<Point3D>(), pair[0][1].get<Point3D>()) :
			Cuboid(pair[0].get<Point3D>(), pair[0].get<Point3D>());
		assert(cuboid.m_high >= cuboid.m_low);
		const T volume = pair[1].get<T>();
		map.insert(cuboid, volume);
	}
}
template<typename T>
void to_json(Json& data, const MapWithOffsetCuboidKeys<T>& map)
{
	for(const auto& [cuboid, value] : map)
		data.push_back({cuboid, value});
}
template<typename T>
void from_json(const Json& data, MapWithOffsetCuboidKeys<T>& map)
{
	for(const Json& pair : data)
	{
		OffsetCuboid cuboid = (pair[0].size() == 2) ?
			OffsetCuboid::create(pair[0][0].get<Point3D>(), pair[0][1].get<Point3D>()) :
			OffsetCuboid(pair[0].get<Offset3D>(), pair[0].get<Offset3D>());
		assert(cuboid.m_high >= cuboid.m_low);
		const T volume = pair[1].get<T>();
		map.insert(cuboid, volume);
	}
}