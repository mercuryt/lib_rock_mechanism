#pragma once
#include "../dataStructures/smallMap.h"
#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"

struct AreaHasPhaseChanges
{
	SmallMap<MaterialTypeId, CuboidSet> m_melting;
	SmallMap<FluidTypeId, CuboidSet> m_freezing;
	void doStep(Area& area);
	void doMelt(Area& area, MaterialTypeId materialType, CuboidSet cuboidSet);
	void doFreeze(Area& area, FluidTypeId fluidType, CuboidSet cuboidSet);
	void onFluidEnters(Area& area, FluidTypeId fluidType, const CuboidSet& cuboidSet);
	void onFluidExits(FluidTypeId fluidType, const CuboidSet& cuboidSet);
	void onSolidSet(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet);
	void onSolidSetNot(MaterialTypeId materialType, const CuboidSet& cuboidSet);
	void onFeatureSet(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet);
	void onFeatureSetNot(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet);
	void onItemEnter(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet);
	void onItemExit(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet);
	void setFreezing(FluidTypeId fluid, const CuboidSet& cuboids);
	void setNotFreezing(FluidTypeId fluid, const CuboidSet& cuboids);
	void setMelting(MaterialTypeId material, const CuboidSet& cuboids);
	void setNotMelting(MaterialTypeId material, const CuboidSet& cuboids);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasPhaseChanges, m_melting, m_freezing);
};