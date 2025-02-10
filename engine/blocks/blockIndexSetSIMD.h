#pragma once
#include "../types.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop

template<uint size>
struct BlockIndexSetSIMD
{
	using Data = Eigen::Array<BlockIndexWidth, 1, size>;
	Data m_data;
	uint8_t m_nextIndex = 0;
	BlockIndexSetSIMD(const Data& data) : m_data(data) { }
	BlockIndexSetSIMD(const Eigen::Array<int, 1, size>& data) { m_data = data.template cast<BlockIndexWidth>(); }
	[[nodiscard]] bool contains(const BlockIndex& block) const { assert(block.exists()); return (m_data == block.get()).any(); }
	[[nodiscard]] BlockIndex operator[](const uint& index) const { assert(index < size); return BlockIndex::create(m_data[index]); }
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
				while(m_index < size && m_set[m_index] == BlockIndex::null().get())
					++m_index;
		}
		[[nodiscard]] BlockIndex operator*() const { return m_set[m_index]; }
		[[nodiscard]] bool operator==(const ConstIterator<skipNull>& other) const { assert(&m_set == &other.m_set); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator<skipNull>& other) const { return !((*this) == other); }
	};
	auto begin() const { return ConstIterator<true>(*this, 0); }
	auto end() const { return ConstIterator<true>(*this, size); }
	struct WithNullView
	{
		const BlockIndexSetSIMD& m_set;
		auto begin() const { return ConstIterator<false>(m_set, 0); }
		auto end() const { return ConstIterator<false>(m_set, size); }
	};
	WithNullView includeNull() const { return WithNullView(*this); }
	SmallSet<BlockIndex> toSmallSet() const
	{
		SmallSet<BlockIndex> output;
		output.resize(size);
		std::ranges::copy(m_data, output);
		return output;
	}
};