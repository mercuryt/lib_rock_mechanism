#include "lineOfSight.h"
#include "../blocks/blocks.h"
#include "../area/area.h"
constexpr auto size = 16;
bool lineOfSight(const Area& area, const Point3D& startPoint, const Point3D& endPoint)
{
	assert(startPoint != endPoint);
	const Eigen::Array<int, 3, 1>& start = startPoint.data.cast<int>();
	const Eigen::Array<int, 3, 1>& end = endPoint.data.cast<int>();
	const Eigen::Array<float, 3, 1> difference = (end - start).cast<float>();
	const Eigen::Array<float, 3, 1> delta = difference / difference.abs().maxCoeff();
	// Start at one rather then zero because we don't need to check the starting position.
	float i= 1;
	const Blocks& blocks = area.getBlocks();
	int endChunkedIndex = blocks.getIndexChunked(endPoint).get();
	static const Eigen::Array<float, 1, size> iota = Eigen::Array<float, 1 ,size>::LinSpaced(0, size - 1);
	static const Eigen::Array<int, 3, 1> chunkLocalMulitpliers(1, 16, 16 * 16);
	const auto& opacityFacade = area.m_opacityFacade;
	BlockIndexChunked previous = blocks.getIndexChunked(startPoint);
	DistanceInBlocks oldZ = startPoint.z();
	while(true)
	{
		// Get coordinates of blocks on the line.
		const Eigen::Array<int, 3, size> coordinatesPerBlock = ((iota + i).replicate(3, 1).colwise() * delta).round().cast<int>().colwise() + start;
		// Convert coordinates into BlockIndexChunked.
		const Eigen::Array<int, 3, size> chunkOffsets = coordinatesPerBlock / 16;
		const Eigen::Array<int, 1, size> globalComponents = (chunkOffsets * blocks.m_pointToIndexConversionMultipliersChunked.cast<int>().replicate(1, size)).colwise().sum();
		const Eigen::Array<int, 3, size> remainders = coordinatesPerBlock - (chunkOffsets * 16);
		const Eigen::Array<int, 1, size> localComponents = (remainders * chunkLocalMulitpliers.replicate(1, size)).colwise().sum();
		const Eigen::Array<int, 1, size> chunkedIndices = localComponents + globalComponents;
		uint j = 0;
		for(const int& chunkedIndex : chunkedIndices)
		{
			auto index = BlockIndexChunked::create(chunkedIndex);
			auto z = DistanceInBlocks::create(coordinatesPerBlock.col(j++)[2]);
			if(!opacityFacade.canSeeIntoFrom(previous, index, oldZ, z))
				return false;
			if(chunkedIndex == endChunkedIndex)
				return true;
			oldZ = z;
			previous = index;
		}
		i+= size;
		assert((coordinatesPerBlock >= 0).all() && (coordinatesPerBlock < blocks.m_dimensions.cast<int>().replicate(1, size)).all());
	}
}