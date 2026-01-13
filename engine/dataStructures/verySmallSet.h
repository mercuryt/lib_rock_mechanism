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
#include <utility>
// Threshold is the maximum which can be stored before a vector is used.
template<typename Contained, int threshold>
class VerySmallSet
{
	using This = VerySmallSet<Contained, threshold>;
	union Data
	{
		std::array<Contained, threshold + 1> array;
		std::vector<Contained> vector;
		Data(Data&& other) noexcept
		{
			if(other.isArray())
				array = other.array;
			else
				vector = std::move(other.vector);
		}
		auto operator=(const Data& other) -> Data&
		{
			if(other.isArray())
				array = other.array;
			else if(isArray())
				// Use placement new to prevent any junk data from when then union was storing an array from being interpreted as vector stack component data.
				new(&vector) std::vector<Contained>(other.vector);
			else
				// If this is already storing a vector use regular assignment, as placement new would cause a memory leak.
				vector = other.vector;
			return *this;
		}
		auto operator=(Data&& other) noexcept -> Data&
		{
			if(other.isArray())
				array = other.array;
			else if(isArray())
				// Use placement new to prevent any junk data from when then union was storing an array from being interpreted as vector stack component data.
				new(&vector) std::vector<Contained>(std::move(other.vector));
			else
				// If this is already storing a vector use regular assignment, as placement new would cause a memory leak.
				vector = std::move(other.vector);
			return *this;
		}
		[[nodiscard]] bool isArray() const
		{
			// If the front of the union data is T::null() when cast to array then the data is currently an array.
			// We can be confident that this is not a vector because a vector is made up of three pointers and T::null() is all 1s and a pointer starting with a 1 would be pointing to an address above the 140th terrabyte.
			// This is dependent on null being a large value, which is provied by StrongInteger.
			// TODO: This is technically undefined behaviour, as we ought not make assumpitions about the internal structure of std::vector, or the ammount of RAM the system has avaliable.
			return array.front() == Contained::null();
		}
		Data() { array.fill(Contained::null()); }
		~Data() { if(!isArray()) vector.clear(); }
	} data;
	[[nodiscard]] bool isArray() const { return data.isArray(); }
	[[nodiscard]] std::array<Contained, threshold + 1>::iterator findFirstArrayNull()
	{
		assert(isArray());
		// Skip index 0 because it is a flag.
		return std::ranges::find(data.array.begin() + 1, data.array.end(), Contained::null());
	}
	[[nodiscard]] std::array<Contained, threshold + 1>::const_iterator findFirstArrayNull() const
	{
		assert(isArray());
		// Skip index 0 because it is a flag.
		return std::ranges::find(data.array.begin() + 1, data.array.end(), Contained::null());
	}
public:
	VerySmallSet() = default;
	VerySmallSet(const This& other) { if(other.isArray()) data.array = other.data.array; else data.vector = other.data.vector; }
	VerySmallSet(This&& other) noexcept { data = other.data; }
	class iterator;
	class const_iterator;
	auto operator=(const This& other) -> This& { data = other.data; return *this; }
	auto operator=(const This&& other) noexcept -> This& { data = std::move(other.data); return *this; }
	void insert(const Contained& value)
	{
		assert(!contains(value));
		insertNonunique(value);
	}
	void maybeInsert(const Contained& value)
	{
		if(!contains(value))
			insertNonunique(value);
	}
	void insertNonunique(const Contained& value)
	{
		if(isArray())
		{
			auto iter = findFirstArrayNull();
			// Cannot fit another value in the array, convert to vector.
			if(iter == data.array.end())
			{
				auto copy = data.array;
				// Use placement new to prevent any junk data from when then union was storing an array from being interpreted as vector stack component data.
				// Skip index 0 because it is a flag.
				new(&data.vector) std::vector<Contained>(copy.begin() + 1, copy.end());
				assert(!isArray());
				data.vector.push_back(value);
			}
			else
				(*iter) = value;
		}
		else
			data.vector.push_back(value);
	}
	void remove(const Contained& value)
	{
		// TODO: This code is checking isArray() multiple times.
		auto iter = find(value);
		remove(iter);
	}
	void maybeRemove(const Contained& value)
	{
		if(contains(value))
			remove(value);
	}
	void remove(iterator& iter)
	{
		if(isArray())
		{
			auto last = findFirstArrayNull();
			--last;
			*iter = *last;
			*last = Contained::null();
		}
		else
		{
			util::removeFromVectorByIndexUnordered(data.vector, iter.getIndex());
			if(data.vector.size() == threshold)
			{
				// If the size has shrunk to the threshold then convert into an array.
				auto vector = std::move(data.vector);
				// Set the array flag.
				data.array.front() = Contained::null();
				// Skip index 0 because it is a flag.
				std::ranges::copy(vector, data.array.begin() + 1);
				assert(isArray());
			}
		}
	}
	void makeUnique()
	{
		if(isArray())
		{
			auto arrayBegin = data.array.begin() + 1;
			auto arrayEnd = findFirstArrayNull();
			std::sort(arrayBegin, arrayEnd);
			auto uniqueEnd = std::unique(arrayBegin, arrayEnd);
			std::fill(uniqueEnd, arrayEnd, Contained::null());
		}
		else
			util::removeDuplicates(data.vector);
	}
	void reserve(int size)
	{
		if(isArray())
		{
			if(size > threshold)
			{
				auto arrayEnd = findFirstArrayNull();
				std::vector<Contained> vector(data.array.begin() + 1, arrayEnd);
				data.vector.swap(vector);
				assert(!isArray());
			}
		}
		else
			data.vector.reserve(size);
	}
	void clear()
	{
		if(!isArray())
			data.vector.clear();
		data.array.fill(Contained::null());
	}
	[[nodiscard]] bool contains(const Contained& value) const
	{
		if(isArray())
			// Skip index 0 because it is a flag.
			return std::ranges::find(data.array.begin() + 1, data.array.end(), value) != data.array.end();
		else
			return std::ranges::find(data.vector, value) != data.vector.end();
	}
	[[nodiscard]] bool empty() const
	{
		if(!isArray())
			return false;
		return data.array[1].empty();
	}
	[[nodiscard]] int size() const
	{
		if(isArray())
			// Skip index 0 because it is a flag.
			return findFirstArrayNull() - (data.array.begin() + 1);
		else
			return data.vector.size();
	}
	[[nodiscard]] Contained& operator[](int offset)
	{
		assert(offset < size());
		if(isArray())
		{
			// Skip index 0 because it is a flag.
			assert(offset + 1u < data.array.size());
			return data.array[offset + 1];
		}
		else
		{
			assert(offset < data.vector.size());
			return data.vector[offset];
		}
	}
	[[nodiscard]] const Contained& operator[](int offset) const
	{
		return (const_cast<This&>(*this))[offset];
	}
	[[nodiscard]] Contained& front() { if(isArray()) return data.array[1]; else return data.vector[0]; }
	[[nodiscard]] Contained& back() { if(isArray()) return data.array[findFirstArrayNull() - 1]; else return data.vector.back(); }
	[[nodiscard]] const Contained& front() const { if(isArray()) return data.array[1]; else return data.vector[0]; }
	[[nodiscard]] const Contained& back() const { if(isArray()) return data.array[findFirstArrayNull() - 1]; else return data.vector.back(); }
	class iterator
	{
		This& set;
		int offset;
	public:
		iterator(This& d, int o) : set(d), offset(o) { }
		[[nodiscard]] Contained& operator*() { assert(offset < set.size()); return set[offset]; }
		[[nodiscard]] const Contained& operator*() const { assert(offset < set.size()); return set[offset]; }
		[[nodiscard]] Contained* operator->() { assert(offset < set.size()); return &set[offset]; }
		iterator& operator++() { ++offset; return *this; }
		iterator& operator++(int) { ++offset; return *this; }
		[[nodiscard]] bool operator==(const iterator& other) const { assert(&set == &other.set); return offset == other.offset; }
		[[nodiscard]] bool operator!=(const iterator& other) const { assert(&set == &other.set); return offset != other.offset; }
		[[nodiscard]] std::strong_ordering operator<=>(const iterator& other) const { assert(&set == &other.set); return offset <=> other.offset; }
		[[nodiscard]] iterator operator-(const iterator& other) const { assert(&other.set == &set); assert(offset >= other.offset); return iterator(set, int(offset - other.offset)); }
		[[nodiscard]] iterator operator+(const iterator& other) const { assert(&other.set == &set); return iterator(set, offset + int(other.offset)); }
		[[nodiscard]] int getIndex() const { return offset; }
	};
	// TODO: very repetetive here.
	class const_iterator
	{
		const This& set;
		int offset;
	public:
		const_iterator(const This& d, int o) : set(d), offset(o) { }
		[[nodiscard]] const Contained& operator*() { assert(offset < set.size()); return set[offset]; }
		[[nodiscard]] const Contained& operator*() const { assert(offset < set.size()); return set[offset]; }
		[[nodiscard]] const Contained* operator->() { assert(offset < set.size()); return &set[offset]; }
		const_iterator& operator++() { ++offset; return *this; }
		const_iterator& operator++(int) { ++offset; return *this; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { assert(&set == &other.set); return offset == other.offset; }
		[[nodiscard]] bool operator!=(const const_iterator& other) const { assert(&set == &other.set); return offset != other.offset; }
		[[nodiscard]] std::strong_ordering operator<=>(const const_iterator& other) const { assert(&set == &other.set); return offset <=> other.offset; }
		[[nodiscard]] const_iterator operator-(const const_iterator& other) const { assert(&other.set == &set); assert(offset >= other.offset); return const_iterator(set, offset - other.offset); }
		[[nodiscard]] const_iterator operator+(const const_iterator& other) const { assert(&other.set == &set); return const_iterator(set, offset + other.offset); }
		[[nodiscard]] int getIndex() const { return offset; }
	};
	[[nodiscard]] iterator begin() { return iterator(*this, 0); }
	[[nodiscard]] iterator end() { return iterator(*this, size()); }
	[[nodiscard]] const_iterator begin() const { return const_iterator(*this, 0); }
	[[nodiscard]] const_iterator end() const { return const_iterator(*this, size()); }
	[[nodiscard]] iterator find(const Contained& contained) { return iterator(*this, indexOf(contained)); }
	[[nodiscard]] const_iterator find(const Contained& contained) const { return const_iterator(*this, indexOf(contained)); }
	template<typename Condition>
	[[nodiscard]] iterator find_if(Condition&& condition) { return iterator(*this, indexOf(condition)); }
	template<typename Condition>
	[[nodiscard]] const_iterator find_if(Condition&& condition) const { return const_iterator(*this, indexOf(condition)); }
	[[nodiscard]] int indexOf(const Contained& value) const
	{
		if(isArray())
			return std::ranges::find(data.array.begin() + 1, data.array.end(), value) - (data.array.begin() + 1);
		else
			return std::ranges::find(data.vector, value) - data.vector.begin();
	}
	template<typename Condition>
	[[nodiscard]] int indexOf(Condition&& condition) const
	{
		if(isArray())
		{
			auto found = std::ranges::find_if(data.array.begin() + 1, data.array.end(), [&](const Contained& value){
				// When no value fulfills the condition return one past the end. This represents end for the iterators.
				return !value.exists() || condition(value);
			});
			// One is added to array begining due to flag.
			return found - (data.array.begin() + 1);
		}
		else
		{
			auto found = std::ranges::find_if(data.vector, condition);
			return found - data.vector.begin();
		}
	}
	[[nodiscard]] Json toJson() const
	{
		Json output;
		if(isArray())
			output["array"] = data.array;
		else
			output["vector"] = data.vector;
		return output;
	}
	void fromJson(const Json& jsonData)
	{
		if(jsonData.contains("array"))
			jsonData["array"].get_to(data.array);
		else
			jsonData["vector"].get_to(data.vector);
	}
	template<typename Collection>
	static VerySmallSet fromCollection(const Collection& collection)
	{
		VerySmallSet output;
		output.reserve(collection.size());
		for(const Contained& value : collection)
			output.insert(value);
		return output;
	}
};
template<typename Contained, int threashold>
inline void to_json(Json& data, const VerySmallSet<Contained, threashold>& set) { data = set.toJson(); }
template<typename Contained, int threashold>
inline void from_json(const Json& data, VerySmallSet<Contained, threashold>& set) { set.fromJson(data); }