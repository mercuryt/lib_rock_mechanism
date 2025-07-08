#include "offsetSetSIMD.h"
#include "../space/space.h"
void OffsetSetSIMD::insert(const Space& space, const Offset3D& offset)
{
	if(m_nextIndex == m_offsetData.cols())
	{
		m_offsetData.conservativeResize(3, std::max(8, m_nextIndex * 2));
		m_indexData.conservativeResize(1, std::max(8, m_nextIndex * 2));
	}
	Eigen::Array<int, 3, 1> offsetConversionMulitpliers = space.m_pointToIndexConversionMultipliers.template cast<int>();
	m_indexData[m_nextIndex] = (offset.data * offsetConversionMulitpliers).sum();
	m_offsetData.col(m_nextIndex++) = offset.data;
}
BlockIndexSetSIMD OffsetSetSIMD::getIndicesInBounds([[maybe_unused]] const Space& space, const Point3D& location) const
{
	Eigen::Array<int, 1, Eigen::Dynamic> output = (m_indexData + (int)location.get());
	assert((output >= 0 && output < space.size()).all());
	return output;
}
BlockIndexSetSIMD OffsetSetSIMD::getIndicesMaybeOutOfBounds(const Space& space, const Point3D& location) const
{
	// Indices off the edge are represented by null.
	Coordinates coordinates = location.data;
	Eigen::Array<int, 3, 1> areaDimensions = space.m_dimensions.template cast<int>();
	Eigen::Array<int, 3, Eigen::Dynamic> offsetSum = m_offsetData.colwise() + coordinates.cast<int>();
	Eigen::Array<bool, 1, Eigen::Dynamic> inBounds = (
		(offsetSum < areaDimensions.replicate(1, offsetSum.cols())).colwise().all() &&
		(offsetSum >= 0).colwise().all()
	);
	Eigen::Array<int, 1, Eigen::Dynamic> inBoundsInt = inBounds.template cast<int>();
	Eigen::Array<int, 1, Eigen::Dynamic> outOfBoundsInt = (!inBounds).template cast<int>();
	// Out of bounds indices have an index equal to location.
	Eigen::Array<int, 1, Eigen::Dynamic> output = (m_indexData * inBoundsInt) + (int)location.get();
	int differenceBetweenLocationAndNull = Point3D::null().get() - location.get();
	// Add difference to out of bounds indices to set them equal to null.
	output += outOfBoundsInt * differenceBetweenLocationAndNull;
	return output;
}
uint OffsetSetSIMD::size() const { return m_nextIndex; }