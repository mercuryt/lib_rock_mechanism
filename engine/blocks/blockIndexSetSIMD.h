#pragma once
#include "../types.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop

struct BlockIndexSetSIMD
{
	using Data = Eigen::Array<BlockIndexWidth, 1, Eigen::Dynamic>;
	Data m_data;
	uint8_t m_nextIndex = 0;
	BlockIndexSetSIMD(const Data& data) : m_data(data) { }
	BlockIndexSetSIMD(const Eigen::Array<int, 1, Eigen::Dynamic>& data) { m_data = data.template cast<BlockIndexWidth>(); }
	[[nodiscard]] bool contains(const BlockIndex& block) const { assert(block.exists()); return (m_data == block.get()).any(); }
	[[nodiscard]] BlockIndex operator[](const uint& index) const { assert(index < m_nextIndex); return BlockIndex::create(m_data[index]); }
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
				while(m_index < m_set.m_nextIndex && m_set[m_index] == BlockIndex::null().get())
					++m_index;
		}
		[[nodiscard]] BlockIndex operator*() const { return m_set[m_index]; }
		[[nodiscard]] bool operator==(const ConstIterator<skipNull>& other) const { assert(&m_set == &other.m_set); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator<skipNull>& other) const { return !((*this) == other); }
	};
	auto begin() const { return ConstIterator<true>(*this, 0); }
	auto end() const { return ConstIterator<true>(*this, m_nextIndex); }
	struct WithNullView
	{
		const BlockIndexSetSIMD& m_set;
		auto begin() const { return ConstIterator<false>(m_set, 0); }
		auto end() const { return ConstIterator<false>(m_set, m_set.m_nextIndex); }
	};
	WithNullView includeNull() const { return WithNullView(*this); }
	SmallSet<BlockIndex> toSmallSet() const
	{
		SmallSet<BlockIndex> output;
		output.resize(m_nextIndex);
		for(const BlockIndexWidth& value : m_data)
			if(value != BlockIndex::null().get())
				output.insert(BlockIndex::create(value));
		return output;
	}
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
				output.insert(value);
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