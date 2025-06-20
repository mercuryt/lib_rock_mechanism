#pragma once
#include <cassert>
#include <cstdint>
#include "numericTypes/types.h"
class u16FixedDistanceInBlocksAccumulator
{
	uint16_t m_data;
public:
	u16FixedDistanceInBlocksAccumulator(DistanceInBlocks data) : m_data(data * 10) { }
	void operator+=(u16FixedDistanceInBlocksAccumulator other)
	{
		assert((uint32_t)m_data + (uint32_t)other.m_data <= UINT16_MAX);
		m_data += other.m_data;
	}
	DistanceInBlocks round()
	{
		auto output =  m_data / 10;
		if(m_data - (output * 10) >= 5)
			++output;
		return output;
	}
};
