#pragma once
/*
 * Uses a union to store a vector in the low slots of an array of the same type.
 */

#include "util.h"
#include <array>
#include <iterator>
#include <vector>
#include <ranges>
template <typename Contained, Contained null, uint capacity>
class HybridSequence
{
	using This = HybridSequence<Contained, null, capacity>;
	static constexpr uint offset = std::ceil((float)sizeof(std::vector<Contained>) / (float)sizeof(Contained));
	union Data{ std::array<Contained, capacity + offset> array; std::vector<Contained> vector; } data;
	[[nodiscard]] auto firstEmptyInArray() const { return std::ranges::find(begin(), data.array.end(), null); }
public:
	class iterator;
	class const_iterator;
	HybridSequence() = default;
	HybridSequence(const This& other)
	{
		// Since the vector is stored in line with the array the array assignment will assign the vector as well.
		data.array = other.array;
	}
	HybridSequence(const This&& other)
	{
		data.array = std::move(other.array);
	}
	void operator=(const This& other)
	{
		data.array = other.array;
	}
	void add(Contained value)
	{
		assert(value != null);
		auto iter = firstEmptyInArray();
		if(iter != data.array.end())
			(*iter) = value;
		else
			data.vector.push_back(value);
	}
	void maybeAdd(Contained value)
	{
		if(!contains(value))
			add(value);
	}
	void remove(Contained value)
	{
		assert(contains(value));
		auto iter = std::ranges::find(begin(), data.array.end(), value);
		if(iter != data.array.end())
		{
			auto last = std::ranges::find(begin(), data.array.end(), Contained::null()) - 1;
			if(last != iter)
				*iter = *last;
			*last = Contained::null();
		}
		else
			util::removeFromVectorByValueUnordered(data.vector, value);
	}
	void swap(const This& other)
	{
		// Since the vector is stored in line with the array the array swap will swap the vector as well.
		data.array.swap(other.data.array);
	}
	[[nodiscard]] bool contains(Contained value)
	{
		auto iter = std::ranges::find(begin(), data.array.end(), value);
		if(iter != data.array.end())
			return true;
		return data.ranges::find(data.vector, value) != data.vector.end();
	}
	[[nodiscard]] uint16_t size()
	{
		uint16_t output = (std::ranges::find(data.array, null) - 1) - begin();
		if(output == capacity)
			output += data.vector.size();
		return output;
	}
	[[nodiscard]] This::iterator find(Contained value)
	{
		auto iter = std::ranges::find(begin(), data.array.end(), value);
		if(iter != data.array.end())
			return iterator(*this, iter - data.array.begin());
		auto iter2 = data.ranges::find(data.vector, value);
		if(iter2 == data.vector.end())
			return (*this, capacity + data.vector.size());
	}
	[[nodiscard]] This::const_iterator find(Contained value) const
	{
		return const_cast<This&>(*this).find(value);
	}
	[[nodiscard]] This::iterator begin() { return data.array.begin() + offset; }
	[[nodiscard]] This::iterator end() { return data.vector.end(); }
	[[nodiscard]] This::const_iterator begin() const { return data.array.begin() + offset; }
	[[nodiscard]] This::const_iterator end() const { return data.vector.end(); }
	class iterator
	{
		This& data;
		uint16_t offset = 0;
	public:
		iterator(This& d, uint16_t o) : data(d), offset(o) { }
		[[nodiscard]] Contained& operator*()
		{
			if(offset <= capacity)
				return (*data.begin() + offset);
			return (*data.vector.begin() + offset);
		}
		iterator& operator++() { offset++; return *this; }
		[[nodiscard]] iterator& operator++(int) { auto copy = *this; offset++; return copy; }
	};
	class const_iterator
	{
		const This& data;
		uint16_t offset = 0;
	public:
		const_iterator(This& d, uint16_t o) : data(d), offset(o) { }
		[[nodiscard]] const Contained& operator*()
		{
			if(offset <= capacity)
				return (*data.begin() + offset);
			return (*data.vector.begin() + offset);
		}
		iterator& operator++() { offset++; return *this; }
		[[nodiscard]] iterator& operator++(int) { auto copy = *this; offset++; return copy; }
	};
};
