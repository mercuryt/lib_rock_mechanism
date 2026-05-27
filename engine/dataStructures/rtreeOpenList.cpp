#include "rtreeOpenList.h"
#include "../threads.h"
#include "../config/config.h"
namespace RTreeOpenListBuffer
{
	std::vector<std::vector<RTreeNodeIndex>> buffer;
	void init()
	{
		assert(buffer.empty());
		buffer.resize(threads::max);
		for(auto& threadBuffer : buffer)
			threadBuffer.reserve(Config::capacityToReserveForRTreeOpenListBufferPerThread);
	}
	std::vector<RTreeNodeIndex>& get() { return buffer[omp_get_thread_num()]; }
}
RTreeOpenList::RTreeOpenList() :
	m_start(RTreeOpenListBuffer::get().size())
{ }
RTreeOpenList::~RTreeOpenList()
{
	auto& buffer = RTreeOpenListBuffer::get();
	assert((int)buffer.size() >= m_start);
	buffer.resize(m_start);
}
void RTreeOpenList::add(RTreeNodeIndex index)
{
	auto& buffer = RTreeOpenListBuffer::get();
	buffer.push_back(index);
}
RTreeNodeIndex RTreeOpenList::next()
{
	assert(!empty());
	auto& buffer = RTreeOpenListBuffer::get();
	RTreeNodeIndex output = buffer.back();
	buffer.pop_back();
	return output;
}
bool RTreeOpenList::empty() const
{
	return (int)RTreeOpenListBuffer::get().size() == m_start;
}