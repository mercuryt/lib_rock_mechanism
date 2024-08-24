#pragma once
#include "../fluidGroup.h"
#include "index.h"
#include "vectorContainers.h"
#include <list>
#include <string>
class Area;
class AreaHasFluidGroups final
{
	//TODO: Should be deque?
	std::list<FluidGroup> m_fluidGroups;
	SmallSet<FluidGroup*> m_unstableFluidGroups;
	SmallSet<FluidGroup*> m_setStable;
	SmallSet<FluidGroup*> m_toDestroy;
	Area& m_area;
public:
	AreaHasFluidGroups(Area& area) : m_area(area) { }
	FluidGroup* createFluidGroup(FluidTypeId fluidType, BlockIndices& blocks, bool checkMerge = true);
	void doStep();
	void removeFluidGroup(FluidGroup& group);
	void clearMergedFluidGroups();
	void validateAllFluidGroups();
	void markUnstable(FluidGroup& group);
	void markStable(FluidGroup& group);
	[[nodiscard]] std::string toS() const;
	[[nodiscard]] std::list<FluidGroup>& getAll() { return m_fluidGroups; }
	[[nodiscard]] SmallSet<FluidGroup*>& getUnstable() { return m_unstableFluidGroups; }
};
