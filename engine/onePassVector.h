#pragma once
#include <cstdint>
#include <vector>
#include <cassert>

template <typename T>
class OnePassVector
{
	std::vector<T> data;
	uint32_t offset = 0;
public:
	void add(T t) { data.push_back(t); }
	[[nodiscard]] T pop() { assert(!empty()); T output = data.at(offset); ++offset; return output; }
	[[nodiscard]] bool empty() { return data.empty() || offset == data.size() - 1; }

};
