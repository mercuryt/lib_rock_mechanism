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
int OffsetSetSIMD::size() const { return m_nextIndex; }