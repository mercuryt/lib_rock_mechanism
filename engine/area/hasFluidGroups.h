#pragma once
#include "../fluidGroups/fluidGroup.h"
#include "../fluidGroups/fluidAllocator.h"
#include "index.h"
#include "vectorContainers.h"
#include <list>
#include <string>
class Area;
class AreaHasFluidGroups final
{
	//TODO: Should be deque?
	FluidAllocator m_allocator;
	std::pmr::list<FluidGroup> m_fluidGroups;
	SmallSet<FluidGroup*> m_unstableFluidGroups;
	SmallSet<FluidGroup*> m_setStable;
	SmallSet<FluidGroup*> m_toDestroy;
	Area& m_area;
public:
	AreaHasFluidGroups(Area& area) : m_fluidGroups(&m_allocator), m_area(area) { }
	FluidGroup* createFluidGroup(const FluidTypeId& fluidType, BlockIndices& blocks, bool checkMerge = true);
	FluidGroup* createFluidGroup(const FluidTypeId& fluidType, BlockIndices&& blocks, bool checkMerge = true);
	void doStep(bool parallel = true);
	void removeFluidGroup(FluidGroup& group);
	void clearMergedFluidGroups();
	void validateAllFluidGroups();
	void markUnstable(FluidGroup& group);
	void markStable(FluidGroup& group);
	[[nodiscard]] std::wstring toS() const;
	[[nodiscard]] std::pmr::list<FluidGroup>& getAll() { return m_fluidGroups; }
	[[nodiscard]] SmallSet<FluidGroup*>& getUnstable() { return m_unstableFluidGroups; }
	[[nodiscard]] const SmallSet<FluidGroup*>& getUnstable() const { return m_unstableFluidGroups; }
	// For assert.
	[[nodiscard]] bool contains(const FluidGroup& fluidGroup);
};
