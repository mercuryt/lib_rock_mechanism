#pragma once
#include "../fluidGroup.h"
#include <list>
#include <string>
#include <unordered_set>
class Area;
class AreaHasFluidGroups final
{
	std::list<FluidGroup> m_fluidGroups;
	std::unordered_set<FluidGroup*> m_unstableFluidGroups;
	std::unordered_set<FluidGroup*> m_setStable;
	std::unordered_set<FluidGroup*> m_toDestroy;
	Area& m_area;
public:
	AreaHasFluidGroups(Area& area) : m_area(area) { }
	FluidGroup* createFluidGroup(const FluidType& fluidType, std::unordered_set<Block*>& blocks, bool checkMerge = true);
	void readStep();
	void writeStep();
	void removeFluidGroup(FluidGroup& group);
	void clearMergedFluidGroups();
	void validateAllFluidGroups();
	void markUnstable(FluidGroup& group);
	void markStable(FluidGroup& group);
	[[nodiscard]] std::string toS() const;
	[[nodiscard]] std::list<FluidGroup>& getAll() { return m_fluidGroups; }
	[[nodiscard]] std::unordered_set<FluidGroup*>& getUnstable() { return m_unstableFluidGroups; }
};
