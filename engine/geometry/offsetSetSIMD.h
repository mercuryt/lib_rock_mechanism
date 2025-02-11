#pragma once

#include "../types.h"
#include "point3D.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop

template<uint size>
struct BlockIndexSetSIMD;

template<uint size>
struct OffsetSetSIMD
{
	//using OffsetData = std::array<Offset3D, size>;
	using OffsetData = Eigen::Array<int, 3, size>;
	OffsetData m_offsetData;
	using IndexData = Eigen::Array<int, 1, size>;
	IndexData m_indexData;
	uint8_t m_nextIndex = 0;
	template<class Blocks>
	void insert(const Blocks& blocks, const Offset3D& offset)
	{
		assert(m_nextIndex < size);
		Eigen::Array<int, 3, 1> offsetConversionMulitpliers = blocks.m_pointToIndexConversionMultipliers.template cast<int>();
		m_indexData[m_nextIndex] = (offset.data * offsetConversionMulitpliers).sum();
		m_offsetData.col(m_nextIndex++) = offset.data;
	}
	template<class Blocks>
	[[nodiscard]] BlockIndexSetSIMD<size> getIndicesInBounds(const Blocks& blocks, const BlockIndex& location) const
	{
		Eigen::Array<int, 1, size> output = (m_indexData + (int)location.get());
		assert((output >= 0 && output < blocks.size()).all());
		return output;
	}
	template<class Blocks>
	[[nodiscard]] BlockIndexSetSIMD<size> getIndicesMaybeOutOfBounds(const Blocks& blocks, const BlockIndex& location) const
	{
		/*
		Eigen::Array<int, 1, size> output = m_indexData + (int)location.get();
		Eigen::Array<int, 3, 1> locationOffsets = blocks.getCoordinates(location).data.template cast<int>();
		Eigen::Array<int, 3, 1> areaDimensions = blocks.m_dimensions.template cast<int>();
		for(uint i = 0; i < size; ++i)
		{
			Eigen::Array<int, 3, 1> offset = m_offsetData[i].data + locationOffsets;
			if((offset < 0 || offset >= areaDimensions).any())
				output[i] = BlockIndex::null().get();
		}
		return output;
		// TODO: finish this non looping version:
		*/
		// Indices off the edge are represented by null.
		Coordinates coordinates = blocks.getCoordinates(location).data;
		Eigen::Array<int, 3, 1> areaDimensions = blocks.m_dimensions.template cast<int>();
		Eigen::Array<int, 3, size> offsetSum = m_offsetData.colwise() + coordinates.cast<int>();
		Eigen::Array<bool, 1, size> inBounds = (
			(offsetSum < areaDimensions.replicate(1, offsetSum.cols())).colwise().all() &&
			(offsetSum >= 0).colwise().all()
		);
		Eigen::Array<int, 1, size> inBoundsInt = inBounds.template cast<int>();
		Eigen::Array<int, 1, size> outOfBoundsInt = (!inBounds).template cast<int>();
		// Out of bounds indices have an index equal to location.
		Eigen::Array<int, 1, size> output = (m_indexData * inBoundsInt) + (int)location.get();
		int differenceBetweenLocationAndNull = BlockIndex::null().get() - location.get();
		// Add difference to out of bounds indices to set them equal to null.
		output += outOfBoundsInt * differenceBetweenLocationAndNull;
		return output;
	}
};