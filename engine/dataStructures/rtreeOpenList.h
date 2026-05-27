#pragma once
#include "../numericTypes/index.h"
#include <omp.h>

namespace RTreeOpenListBuffer
{
	void init();
}
class RTreeOpenList
{
	int m_start;
public:
	RTreeOpenList();
	~RTreeOpenList();
	[[nodiscard]] RTreeNodeIndex next();
	[[nodiscard]] bool empty() const;
	void add(RTreeNodeIndex);
};
