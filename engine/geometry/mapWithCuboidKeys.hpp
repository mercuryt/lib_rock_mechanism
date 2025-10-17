#pragma once

#include "mapWithCuboidKeys.h"
#include "cuboidSet.h"
template<typename T, typename CuboidType, typename CuboidSetType>
void MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::insertOrMerge(const CuboidType& cuboid, const T& value)
{
	for(auto& [existingCuboid, existingValue] : data)
		if (existingValue == value && cuboid.isTouching(existingCuboid) && cuboid.canMerge(existingCuboid))
		{
			existingCuboid.maybeExpand(cuboid);
			return;
		}
	data.insert({cuboid, value});
}
template<typename T, typename CuboidType, typename CuboidSetType>
void MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::insertOrMerge(const Pair& pair) { insertOrMerge(pair.first, pair.second); }
template<typename T, typename CuboidType, typename CuboidSetType>
void MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::insert(const CuboidType& cuboid, const T& value) { data.insert({cuboid, value}); }
template<typename T, typename CuboidType, typename CuboidSetType>
void MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::insert(const Pair& pair) { data.insert(pair); }
template<typename T, typename CuboidType, typename CuboidSetType>
void MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::insertAll(const This& other)
{
	for(const auto& pair : other)
		insert(pair);
}
template<typename T, typename CuboidType, typename CuboidSetType>
void MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::removeContainedAndFragmentIntercepted(const CuboidType& cuboid)
{
	auto copy = data;
	copy.reserve(data.size());
	data.clear();
	for(auto& [existingCuboid, existingValue] : copy)
	{
		if(!cuboid.intersects(existingCuboid))
			data.insert({existingCuboid, existingValue});
		else if(cuboid.contains(existingCuboid))
			continue;
		else
			for(const CuboidType& fragment : existingCuboid.getChildrenWhenSplitBy(cuboid))
				data.insert({fragment, existingValue});
	}
}
template<typename T, typename CuboidType, typename CuboidSetType>
void MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::removeContainedAndFragmentInterceptedAll(const CuboidSetType& cuboids)
{
	for(const CuboidType& cuboid : cuboids)
		removeContainedAndFragmentIntercepted(cuboid);
}
template<typename T, typename CuboidType, typename CuboidSetType>
void MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::clear() { data.clear(); }
template<typename T, typename CuboidType, typename CuboidSetType>
void MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::sort()
{
	data.sort([&](const Pair& a, const Pair& b){ return a.first.getCenter().hilbertNumber() < b.first.getCenter().hilbertNumber(); });
}
template<typename T, typename CuboidType, typename CuboidSetType>
T& MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::operator[](const CuboidType& cuboid) { auto found = find(cuboid); assert(found != data.end()); return found->second; }
template<typename T, typename CuboidType, typename CuboidSetType>
MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::Iterator MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::find(const CuboidType& cuboid) { return std::ranges::find(data.m_data, cuboid, &std::pair<CuboidType, T>::first); }
template<typename T, typename CuboidType, typename CuboidSetType>
CuboidType MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::boundry() const
{
	assert(!empty());
	PointType high = PointType::min();
	PointType low = PointType::max();
	for(const auto& [cuboid, value] : data)
	{
		high = high.max(cuboid.m_high);
		low = low.min(cuboid.m_low);
	}
	return {high, low};
}
template<typename T, typename CuboidType, typename CuboidSetType>
CuboidSetType MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::getCuboids() const
{
	CuboidSetType output;
	output.reserve(data.size());
	for(const auto& pair : data)
		output.add(pair.first);
	return output;
}
template<typename T, typename CuboidType, typename CuboidSetType>
bool MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::contains(const Point3D& point) const
{
	for(const Pair& pair : data)
		if(pair.first.contains(point))
			return true;
	return false;
}
template<typename T, typename CuboidType, typename CuboidSetType>
bool MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::contains(const Offset3D& point) const
{
	for(const Pair& pair : data)
		if(pair.first.contains(point))
			return true;
	return false;
}
template<typename T, typename CuboidType, typename CuboidSetType>
CuboidSetType MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::makeCuboidSet() const
{
	CuboidSetType output;
	output.reserve(data.size());
	for(const auto& [cuboid, value] : data)
		output.add(cuboid);
	return output;
}
template<typename T, typename CuboidType, typename CuboidSetType>
Quantity MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::volume() const
{
	Quantity output{0};
	for(const auto& pair : data)
		output += pair.first.volume();
	return output;
}
template<typename T, typename CuboidType, typename CuboidSetType>
std::string MapWithCuboidKeysBase<T, CuboidType, CuboidSetType>::toString() const
{
	std::string output;
	for(const auto& [cuboid, value] : data)
	{
		output += cuboid.toString() + " : ";
		if constexpr(HasToStringMethod<T>)
			output += value.toString() + ", ";
		else if constexpr(std::is_pointer<T>())
			output += std::to_string((uintptr_t)value) + ", ";
		else if constexpr(Numeric<T>)
			output += std::to_string(value) + ", ";
		else
			assert(false);
	}
	return output;
}
template<typename T>
MapWithOffsetCuboidKeys<T> MapWithOffsetCuboidKeys<T>::applyOffset(const Offset3D& offset) const
{
	auto output = *this;
	for(auto& [cuboid, value] : output)
		cuboid.shift(offset, {1});
	return output;
}
template<typename T>
MapWithOffsetCuboidKeys<T> MapWithOffsetCuboidKeys<T>::shiftAndRotateAndRemoveIntersectionWithOriginal(const Offset3D& offset, const Facing4& previousRotation, const Facing4& newRotation) const
{
	MapWithOffsetCuboidKeys<T> output;
	for(const auto& [cuboid, value] : this->data)
	{
		OffsetCuboid shifted = cuboid;
		shifted.shift(offset, {1});
		shifted.rotate2D(previousRotation, newRotation);
		output.insert({shifted, value});
	}
	for(const auto& [cuboid, value] : this->data)
		output.removeContainedAndFragmentIntercepted(cuboid);
	return output;
}
template<typename T>
MapWithCuboidKeys<T> MapWithOffsetCuboidKeys<T>::relativeTo(const Point3D& location) const
{
	MapWithCuboidKeys<T> output;
	for(const auto& [cuboid, value] : this->data)
		output.insert(Cuboid::create(cuboid.relativeToPoint(location)), value);
	return output;
}