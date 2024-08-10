/*
 * Holds a set of Strong values ( or anything else which provides a null() static method ).
 * Converts between array and vector based on size.
 * Encodes current status as array by setting offset 0 to T::null().
 * T::null() should be the maximum value which can be represented by type T.
 * A vector should never be misinterperented as a value because a real world address will never set the high bits in 64 space. ( 2^64 - 1 should be enough memory for anyone).
 */
#pragma once

#include "util.h"
#include "json.h"
#include <array>
#include <iterator>
#include <vector>
// Threshold defaults to the max number of Contained which could fit in the same stack space as a vector.
template<typename Contained, int threshold = (sizeof(std::vector<Contained>) / sizeof(Contained)) - 1>
class VerySmallSet
{
	using This = VerySmallSet<Contained, threshold>;
	union Data{ std::array<Contained, threshold> array; std::vector<Contained> vector; } data;
	[[nodiscard]] bool isArray() const
	{
		// If the front of the union data is T::null() when cast to array then the data is currently an array.
		// TODO: We only really need to read / write the first bit.
		return data.array.front() == Contained::null();
	}
	[[nodiscard]] auto findFirstArrayNull() const
	{
		assert(isArray());
		// Skip index 0 because it is a flag.
		return std::ranges::find(data.array.begin() + 1, data.array.end(), Contained::null());
	}
public:
	void add(Contained value)
	{
		assert(!contains(value));
		if(isArray())
		{
			auto iter = findFirstArrayNull();
			// Cannot fit another value in the array, convert to vector.
			if(iter == data.array.end())
			{
				// Skip index 0 because it is a flag.
				std::vector<Contained> vector(data.array.begin() + 1, data.array.end());
				data.vector.swap(vector);
			}
			else
				(*iter) = value;
		}
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
		if(isArray())
			util::removeFromArrayByValueUnordered(data.array, value);
		else
		{
			util::removeFromVectorByValueUnordered(data.vector, value);
			if(data.vector.size() == threshold)
			{
				// If the size has shrunk to the threshold then convert into an array.
				std::array<Contained, threshold> array;
				// Set the array flag.
				array.front = Contained::null();
				// Skip index 0 because it is a flag.
				std::ranges::copy(data.vector, array.begin() + 1);
				data.array.swap(array);
			}
		}
	}
	void maybeRemove(Contained value)
	{
		if(contains(value))
			remove(value);
	}
	void fromJson(const Json& json)
	{
		if(data.size() > threshold)
			data.get_to(data.vector);
		else
		{
			data.array.front() = Contained::null();
			auto iter = data.array.begin() ;
			for(const Json& item : json)
				item.get_to((*++iter));
		}
	}
	[[nodiscard]] Json& toJson() const
	{
		Json output;
		for(Contained& value : data.array)
			output.push_back(value);
	}
	[[nodiscard]] bool contains(Contained value) const
	{
		if(isArray())
			// Skip index 0 because it is a flag.
			return std::ranges::find(data.array.begin() + 1, data.array.end(), value) != data.array.end();
		else
			return std::ranges::find(data.vector, value) != data.vector.end();
	}
	[[nodiscard]] uint16_t size() const
	{
		if(isArray())
			// Skip index 0 because it is a flag.
			return std::ranges::find(data.array, Contained::null()) - (data.array.begin() + 1);
		else
			return data.vector.size();
	}
	[[nodiscard]] Contained& operator[](uint16_t offset)
	{
		assert(offset < size());
		if(isArray())
		{
			// Skip index 0 because it is a flag.
			assert(offset + 1 < threshold);
			return data.array[offset + 1];
		}
		else
			return data.vector[offset];
	}
	[[nodiscard]] const Contained& operator[](uint16_t offset) const
	{
		return const_cast<This*>(this)->at(offset);
	}
	class iterator
	{
		This& set;
		uint16_t offset;
	public:
		iterator(This& d, uint16_t o = 0) : set(d), offset(o) { }
		[[nodiscard]] iterator& operator++() { ++offset; return *this; }
		[[nodiscard]] Contained& operator*() { return set[offset]; }
		[[nodiscard]] bool operator==(const This& other) { assert(&set == &other.set); return offset == other.offset; }
	};
	class const_iterator : public iterator
	{
	public:
		const_iterator(This& d, uint16_t o = 0) : iterator(d, o) { }
		[[nodiscard]] const Contained& operator*() { return iterator::set[iterator::offset]; }
	};
	[[nodiscard]] This::iterator begin() { return Iterator(*this); }
	[[nodiscard]] This::iterator end() { return Iterator(*this, size()); }
	[[nodiscard]] This::const_iterator begin() const { return Iterator(*this); }
	[[nodiscard]] This::const_iterator end() const { return Iterator(*this, size()); }
};
template<typename T, uint16_t threshold>
inline void to_json(Json& data, const VerySmallSet<T, threshold>& set) { data = set.toJson(); }
template<typename T, uint16_t threshold>
inline void from_json(const Json& data, VerySmallSet<T, threshold>& set) { set.from_json(data); }
