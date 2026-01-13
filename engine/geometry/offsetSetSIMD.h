#pragma once

#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "geometry/point3D.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop

class Space;

struct OffsetSetSIMD
{
	//using OffsetData = std::array<Offset3D, size>;
	using OffsetData = Eigen::Array<int, 3, Eigen::Dynamic>;
	OffsetData m_offsetData;
	using IndexData = Eigen::Array<int, 1, Eigen::Dynamic>;
	IndexData m_indexData;
	int m_nextIndex = 0;
	void insert(const Space& space, const Offset3D& offset);
	[[nodiscard]] int size() const;
};
template<int size>
struct OffsetArraySIMD
{
	//using OffsetData = std::array<Offset3D, size>;
	using OffsetData = Eigen::Array<int, 3, size>;
	OffsetData m_offsetData;
	using IndexData = Eigen::Array<int, 1, size>;
	IndexData m_indexData;
	int m_nextIndex = 0;
	template<class Space>
	void insert(const Space& space, const Offset3D& offset)
	{
		assert(m_nextIndex < size);
		Eigen::Array<int, 3, 1> offsetConversionMulitpliers = space.m_pointToIndexConversionMultipliers.template cast<int>();
		m_indexData[m_nextIndex] = (offset.data * offsetConversionMulitpliers).sum();
		m_offsetData.col(m_nextIndex++) = offset.data;
	}
};