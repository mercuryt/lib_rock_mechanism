#include "allocator.h"
#include "config.h"
#include <cstdlib>

Allocator::Allocator(uint32_t sizeInBytes) : m_allocatedBytes(sizeInBytes)
{
	m_begin = std::malloc(sizeInBytes);
	m_end = m_begin;
}
void* Allocator::allocate(uint32_t sizeInBytes)
{
	// Check if any chunks in empty are big enough.
	for(auto iter = m_empty.begin(); iter != m_empty.end(); ++iter)
	{
		char* begin = iter->first;
		char* end = iter->second;
		if(sizeInBytes == std::distance(begin, end))
		{
			// New allocation fills empty space, remove it from m_empty.
			m_empty.erase(iter);
			return begin;
		}
		if(sizeInBytes < std::distance(begin, end))
		{
			// New allocation does not fill empty space, update it's begin.
			iter->first += sizeInBytes;
			return begin;
		}
	}
	// No suitable empty chunks found
	if((char*)m_begin + m_allocatedBytes < (char*)m_end + sizeInBytes)
	{
		// Not enough space left to allocate, grow the buffer.
		uint32_t oldSize = m_allocatedBytes;
		m_allocatedBytes = std::max(uint32_t(m_allocatedBytes * Config::goldenRatio), m_allocatedBytes + sizeInBytes);
		void *newBuffer = malloc(m_allocatedBytes);
		std::memcpy(newBuffer, m_begin, oldSize);
		m_begin = newBuffer;
		m_end = (char*)m_begin + oldSize;
	}
	void* output = m_end;
	m_end = (char*)m_end + sizeInBytes;
	return output;
}
void Allocator::deallocate(char* begin, uint32_t sizeInBytes)
{
	assert((char*)begin >= (char*)m_begin);
	assert((char*)begin + sizeInBytes <= (char*)m_end);
	// Find first pair in empty which is more then begin.
	std::vector<std::pair<char*, char*>>::iterator next = std::ranges::upper_bound(m_empty, begin, {}, &std::pair<char*, char*>::first);
	if(next == m_empty.end())
	{
		// Newly free space is at the end, shrink m_end.
		m_end = (char*)m_end - sizeInBytes;
		if(!m_empty.empty() && m_empty.back().second == m_end)
		{
			// Shrinking exposed another empty space as the end, delete it and shrink again.
			m_end = m_empty.back().first;
			m_empty.pop_back();
		}
		return;
	}
	assert(next->second < begin);
	if (next != m_empty.begin())
	{
		// A previous pair exists.
		auto prev = next - 1;
		if (prev->second == begin)
		{
			// Newly freed space is directly adjacent to free space on the left, combine them.
			prev->second += sizeInBytes;
			if(next->first == begin + sizeInBytes)
			{
				// Newly freed space is also directly adjacent to free space on the right, combine all three.
				prev->second = next->second;
				m_empty.erase(next);
			}
			return;
		}
	}
	// Find next in empty.
	if (next->first == begin + sizeInBytes)
	{
		// Newly freed space is directly adjacent to free space on the right, combine them.
		next->first -= sizeInBytes;
		return;
	}
	// Not added to any existing pair, inserted in order.
	m_empty.emplace(next, begin, begin + sizeInBytes);
}
Allocator::~Allocator()
{
	std::free(m_begin);
}
