/*
 * FiFo queue backed by vector. Start is tracked by iterator.
 * When reserved space is exhaused it either copies it's over the already used slots in the vector or allocates a new one and replaces.
 * After copy or reallocate the iterator is reset to the begining.
 */
#pragma once
#include <vector>
#include <cassert>

constexpr float GROWTH_FACTOR = 1.5;

template <typename T, std::size_t initalCapacity>
class OnePassVector
{
	std::vector<T> data;
	std::vector<T>::iterator iter;
public:
	OnePassVector() : iter(data.begin()) { }
	void add(T t)
	{
		if(data.capacity() == data.size())
		{
			std::vector<T> newData;
			newData.reserve(data.size() * GROWTH_FACTOR);
			std::copy(iter, data.end(), newData.begin());
			data = newData;
			iter = data.begin();
		}
		data.push_back(t);
	}
	void clear() { data.clear(); iter = data.begin(); }
	[[nodiscard]] T pop() { assert(!empty()); T output = *iter; ++iter; return output; }
	[[nodiscard]] bool empty() const { return data.empty() || iter == data.end(); }
};
