#include "offsetCuboidSet.h"
void OffsetCuboidSet::maybeMergeExisting(SmallSet<OffsetCuboid>::iterator iter)
{
	OffsetCuboid& toMerge = *iter;
	const auto end = m_data.end();
	for(auto iter2 = m_data.begin(); iter2 < end; ++iter2)
		if(iter2 != iter && iter2->canMerge(toMerge))
		{
			iter2->extend(toMerge);
			if(iter2 == end - 1)
			{
				// iter2 is the last item, handle seperately because calling erase will swap it with iter.
				m_data.erase(iter);
				maybeMergeExisting(iter);
			}
			else
			{
				m_data.erase(iter);
				maybeMergeExisting(iter2);
			}
			return;
		}
}
void OffsetCuboidSet::addOrMerge(const OffsetCuboid& newCuboid)
{
	auto end = m_data.end();
	for(auto iter = m_data.begin(); iter < end; ++iter)
		if(iter->canMerge(newCuboid))
		{
			iter->extend(newCuboid);
			maybeMergeExisting(iter);
			return;
		}
	m_data.insert(newCuboid);
}
void OffsetCuboidSet::addOrMerge(const Offset3D& offset)
{
	addOrMerge({offset, offset});
}
void OffsetCuboidSet::maybeRemove(const Offset3D& offset)
{
	// Don't cache data end, check it every iteration, it may change.
	for(auto iter = m_data.begin(); iter < m_data.end(); ++iter)
		if(iter->contains(offset))
		{
			if(iter->size() == 1)
				m_data.erase(iter);
			else
			{
				auto newCuboids = iter->getChildrenWhenSplitByCuboid({offset, offset});
				m_data.erase(iter);
				for(const OffsetCuboid& cuboid : newCuboids)
					addOrMerge(cuboid);
			}
			return;
		}
}
OffsetCuboidSet::ConstIterator::ConstIterator(const OffsetCuboidSet& cuboidSet, bool end) :
	m_cuboidSet(const_cast<OffsetCuboidSet*>(&cuboidSet)),
	m_outerIter(end ? cuboidSet.m_data.end() : cuboidSet.m_data.begin())
{
	if(end || cuboidSet.empty())
		m_end = true;
	else
		m_innerIter = cuboidSet.m_data.begin()->begin();
}
bool OffsetCuboidSet::ConstIterator::operator==(const ConstIterator& other) const
{
	assert(m_cuboidSet == other.m_cuboidSet);
	if(m_end && other.m_end)
		return true;
	return m_outerIter == other.m_outerIter && m_innerIter == other.m_innerIter;
}
Offset3D OffsetCuboidSet::ConstIterator::operator*() const { return *m_innerIter; }
OffsetCuboidSet::ConstIterator& OffsetCuboidSet::ConstIterator::operator++()
{
	if(m_innerIter != m_outerIter->end())
		++m_innerIter;
	else
	{
		++m_outerIter;
		if(m_outerIter != m_cuboidSet->m_data.end())
		{
			OffsetCuboid::ConstIterator newInnerBegin = m_outerIter->begin();
			m_innerIter = newInnerBegin;
		}
		else
			m_end = true;
	}
	return *this;
}
OffsetCuboidSet::ConstIterator OffsetCuboidSet::ConstIterator::operator++(int)
{
	auto copy = *this;
	++(*this);
	return copy;
}
Json OffsetCuboidSet::toJson() const
{
	Json output = Json::array();
	for(const OffsetCuboid& cuboid : m_data)
		output.push_back(cuboid);
	return output;
}
void OffsetCuboidSet::load(const Json& data)
{
	for(const Json& cuboid : data)
		m_data.emplace(cuboid);
}