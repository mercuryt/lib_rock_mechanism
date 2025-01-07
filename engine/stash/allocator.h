#pragma once
#include <vector>
#include <cstdint>

class Allocator
{
	void* m_begin;
	void* m_end;
	uint32_t m_allocatedBytes;
	//TODO: benchmark with deque.
	std::vector<std::pair<char*, char*>> m_empty;
public:
	Allocator(uint32_t sizeInBytes);
	void* allocate(uint32_t sizeInBytes);
	void deallocate(char* start, uint32_t sizeInBytes);
	~Allocator();
	Allocator(const Allocator&) = delete;
	Allocator(Allocator&&) noexcept = delete;
	Allocator& operator=(const Allocator&) = delete;
	Allocator& operator=(Allocator&&) = delete;
};