#pragma once
#include "../dataStructures/rtreeData.h"
#include "../dataStructures/rtreeBoolean.h"
#include "../config/world.h"
#include "settlement.h"
#include "smallRivers.h"

class Simulation;

struct World
{
	RTreeData<MaterialTypeId> m_solid;
	// TODO: deserts and grasslands.
	RTreeData<FluidTypeId> m_fluid;
	RTreeData<BitSet<uint8_t, 8u>> m_roads;
	RTreeData<SmallRiverData> m_smallRivers;
	RTreeData<SettlementId> m_settlement;
	RTreeData<AreaId> m_areas;
	RTreeData<Quantity> m_trees;
	RTreeData<ExpeditionId> m_expeditions;
	RTreeBoolean m_ocean; // track what part is salt water, or equivalent.
	RTreeBoolean m_smallLakes;
	Cuboid m_boundry;
	Distance m_seaLevel;
	Temperature m_maxAmbiantTemperature{Config::World::defaultHighAmbiantTemperature};
	Temperature m_minAmbiantTemperature{Config::World::defaultLowAmbiantTemperature};
	FluidTypeId m_oceanFluidType;
	World(Cuboid boundry);
	AreaId createArea(Simulation& simulation, Point3D location, bool proceduralGeneration = true);
	[[nodiscard]] Percent getHumidity(Point3D location) const;
	[[nodiscard]] MaterialTypeId getBedrockType(Point3D location) const;
	[[nodiscard]] Temperature getAverageTemperature(Point3D location) const;
	[[nodiscard]] int getFoliageMass(Point3D location) const;
	[[nodiscard]] Area& getOrCreateArea(Simulation& simulation, Point3D location);
};
