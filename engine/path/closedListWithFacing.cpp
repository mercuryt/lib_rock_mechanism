#include "closedListWithFacing.h"
void ClosedListWithFacing::close(const Point3D point, const Facing4 facing)
{
	assert(!m_closedFromAllFacings.contains(point));
	auto found = m_closedFromSomeFacings.find(point);
	if(found == m_closedFromSomeFacings.end())
	{
		BitSet<uint8_t, 4u> bitset;
		bitset.set((int)facing);
		m_closedFromSomeFacings.insert(point, bitset);
	}
	else
	{
		assert(!found->second[(int)facing]);
		found->second.set((int)facing);
		if(found->second.all())
		{
			m_closedFromAllFacings.add(point);
			(*found) = m_closedFromSomeFacings.back();
			m_closedFromSomeFacings.popBack();
		}
	}
}
void ClosedListWithFacing::closeAllDirections(const Point3D point)
{
	auto found = m_closedFromSomeFacings.find(point);
	if(found != m_closedFromSomeFacings.end())
	{
		(*found) = m_closedFromSomeFacings.back();
		m_closedFromSomeFacings.popBack();
	}
	m_closedFromAllFacings.add(point);
}
void ClosedListWithFacing::clear()
{
	m_closedFromAllFacings.clear();
	m_closedFromSomeFacings.clear();
}
void ClosedListWithFacing::removeFrom(CuboidSet& cuboidSet) const { cuboidSet.maybeRemoveAll(m_closedFromAllFacings); }
bool ClosedListWithFacing::check(const Point3D point, const Facing4 facing) const
{
	if(m_closedFromAllFacings.contains(point))
		return true;
	auto found = m_closedFromSomeFacings.find(point);
	if(found == m_closedFromSomeFacings.end())
		return false;
	return found->second[(int)facing];
}
bool ClosedListWithFacing::checkAndCloseIfNot(const Point3D point, const Facing4 facing)
{
	// Anything closed from all facing should have been filtered out already by removeFrom.
	assert(!m_closedFromAllFacings.contains(point));
	auto found = m_closedFromSomeFacings.find(point);
	if(found == m_closedFromSomeFacings.end())
	{
		BitSet<uint8_t, 4u> bitset;
		bitset.set((int)facing);
		m_closedFromSomeFacings.insert(point, bitset);
	}
	else
	{
		if(found->second[(int)facing])
			return true;
		found->second.set((int)facing);
		if(found->second.all())
		{
			m_closedFromAllFacings.add(point);
			(*found) = m_closedFromSomeFacings.back();
			m_closedFromSomeFacings.popBack();
		}
	}
	return false;
}