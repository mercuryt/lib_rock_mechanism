#pragma once
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop

struct BlockIndexSetSIMD
{
	using Data = Eigen::Array<BlockIndexWidth, 1, Eigen::Dynamic>;
	// Data may be padded with null values, as well as contain null values.
	Data m_data;
	BlockIndexSetSIMD(const Data& data) : m_data(data) { }
	BlockIndexSetSIMD(const Eigen::Array<int, 1, Eigen::Dynamic>& data);
	[[nodiscard]] bool contains(const BlockIndex& block) const;
	[[nodiscard]] BlockIndex operator[](const uint& index) const;
	// For debugging.
	[[nodiscard]] uint size() const;
	[[nodiscard]] uint sizeWithNull() const;
	[[nodiscard]] std::string toString() const;
	template<bool skipNull>
	class ConstIterator
	{
		const BlockIndexSetSIMD& m_set;
		uint8_t m_index = 0;
	public:
		ConstIterator(const BlockIndexSetSIMD& set, uint8_t index) :
			m_set(set),
			m_index(index)
		{
			skipTillNext();
		}
		ConstIterator operator++()
		{
			++m_index;
			skipTillNext();
			return *this;
		}
		[[nodiscard]] ConstIterator operator++(int)
		{
			auto copy = this;
			++(*this);
			return copy;
		}
		void skipTillNext()
		{
			if constexpr (skipNull)
			{
				auto size = m_set.m_data.size();
				while(m_index < size && m_set[m_index] == BlockIndex::null().get())
					++m_index;
			}
		}
		[[nodiscard]] BlockIndex operator*() const { return m_set[m_index]; }
		[[nodiscard]] bool operator==(const ConstIterator<skipNull>& other) const { assert(&m_set == &other.m_set); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator<skipNull>& other) const { return !((*this) == other); }
	};
	auto begin() const { return ConstIterator<true>(*this, 0); }
	auto end() const { return ConstIterator<true>(*this, m_data.size()); }
	struct WithNullView
	{
		const BlockIndexSetSIMD& m_set;
		auto begin() const { return ConstIterator<false>(m_set, 0); }
		auto end() const { return ConstIterator<false>(m_set, m_set.m_data.size()); }
	};
	WithNullView includeNull() const;
	SmallSet<BlockIndex> toSmallSet() const;
};

template<uint size>
struct BlockIndexArraySIMD
{
	using Data = Eigen::Array<BlockIndexWidth, 1, size>;
	Data m_data;
	uint8_t m_nextIndex = 0;
	BlockIndexArraySIMD(const Data& data) : m_data(data) { }
	BlockIndexArraySIMD(const Eigen::Array<int, 1, size>& data) { m_data = data.template cast<BlockIndexWidth>(); }
	[[nodiscard]] bool contains(const BlockIndex& block) const { assert(block.exists()); return (m_data == block.get()).any(); }
	[[nodiscard]] BlockIndex operator[](const uint& index) const { assert(index < size); return BlockIndex::create(m_data[index]); }
	template<bool skipNull>
	class ConstIterator
	{
		const BlockIndexArraySIMD& m_array;
		uint8_t m_index = 0;
	public:
		ConstIterator(const BlockIndexArraySIMD& set, uint8_t index) :
			m_array(set),
			m_index(index)
		{
			skipTillNext();
		}
		ConstIterator operator++()
		{
			++m_index;
			skipTillNext();
			return *this;
		}
		[[nodiscard]] ConstIterator operator++(int)
		{
			auto copy = this;
			++(*this);
			return copy;
		}
		void skipTillNext()
		{
			if constexpr (skipNull)
				while(m_index < size && m_array[m_index] == BlockIndex::null().get())
					++m_index;
		}
		[[nodiscard]] BlockIndex operator*() const { return m_array[m_index]; }
		[[nodiscard]] bool operator==(const ConstIterator<skipNull>& other) const { assert(&m_array == &other.m_array); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator<skipNull>& other) const { return !((*this) == other); }
	};
	auto begin() const { return ConstIterator<true>(*this, 0); }
	auto end() const { return ConstIterator<true>(*this, size); }
	struct WithNullView
	{
		const BlockIndexArraySIMD& m_array;
		auto begin() const { return ConstIterator<false>(m_array, 0); }
		auto end() const { return ConstIterator<false>(m_array, size); }
	};
	WithNullView includeNull() const { return WithNullView(*this); }
	SmallSet<BlockIndex> toSmallSet() const
	{
		SmallSet<BlockIndex> output;
		output.resize(size);
		for(const BlockIndexWidth& value : m_data)
			if(value != BlockIndex::null().get())
				output.insert(BlockIndex::create(value));
		return output;
	}
	auto toArray() const -> std::array<BlockIndex, size>
	{
		std::array<BlockIndex, size> output;
		uint i = 0;
		for(const BlockIndexWidth& index : m_data)
			output[i++] = BlockIndex::create(index);
		return output;
	}
};