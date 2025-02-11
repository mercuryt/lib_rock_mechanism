#pragma once
#include "point3D.h"
#include "../blocks/blocks.h"

template<typename Func>
concept PointCondition = requires(Func f, const Point3D& point)
{
	{ f(point) } -> std::same_as<bool>;
};

struct Line
{
	Point3D m_start;
	Point3D m_end;
	template<PointCondition Condition, uint size = 8>
	bool anyChunked(const Blocks& blocks, Condition&& condition)
	{
		const Eigen::Array<int, 3, 1>& start = m_start.data;
		const Eigen::Array<int, 3, 1>& end = m_end.data;
		Eigen::Array<int, 3, 1> difference = start - end;
		Eigen::Array<float, 3, 1> delta = difference.cast<float> / (float)difference.abs().max();
		// Start at one rather then zero because we don't need to check the starting position.
		float index = 1;
		uint endChunkedIndex = blocks.getIndexChunked(m_end).get();
		static Eigen::Array<float, 1, size> iota = Eigen::Array<int, 1 ,size>::LinSpaced(0, size - 1);
		static Eigen::Array<int, 3, 1> chunkLocalMulitpliers(1, 16, 16 * 16);
		while(true)
		{
			Eigen::Array<int, 3, size> coordinatesPerBlock = ((itoa + index).replicate(3, 1).colwise() * delta).round().cast<int>().colwise() + start;
			Eigen::Array<int, 3, size> chunkOffsets = coordinatesPerBlock / 16;
			Eigen::Array<int, 3, size> remainders = coordinatesPerBlock - (chunkOffsets * 16);
			Eigen::Array<int, 1, size> localComponents = (remainders.colwise() * chunkLocalMulitpliers).colwise().sum();
			Eigen::Array<int, 1, size> globalComponents = (chunkOffsets.colwise() * blocks.m_pointToIndexConversionMultipliersChunked).colwise().sum();
			Eigen::Array<int, 1, size> chunkedIndices = localComponents + globalComponents;
			Eigen::Array<bool, 1, size> isEnd = chunkedIndices == endChunkedIndex;
			uint i = 0;
			for(const int& chunkedIndex : chunkedIndices)
			{
				if(condition(BlockIndexChunked::create(chunkedIndex)))
					return true;
				if(isEnd[i++])
					return false;
			}
			index += size;
			if constexpr(DEBUG)
			{
				const Point3D lastCoordinate = Point3D::create(coordinatesPerBlock.col(7).cast<int>());
				assert((lastCoordinate.data >= 0 ).all() && (lastCoordinate.data < blocks.m_dimensions).all());
			}
		}
	}
};