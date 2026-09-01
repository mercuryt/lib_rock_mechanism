#pragma once
#include "../geometry/cuboidSet.h"
struct World;
class Simulation;
class Area;
struct BuildArea
{
	inline static std::array<std::pair<int, int>, 8> offsetsByIndex{
		std::pair(1,1),
		std::pair(1,-1),
		std::pair(-1,-1),
		std::pair(-1,1),
		std::pair(0,1),
		std::pair(1,0),
		std::pair(0,-1),
		std::pair(-1,0),
	};
	CuboidSet m_surface = { };
	// Control overall slope to align with world map.
	std::array<Point3D, 8u> m_edgeAttractors;
	// random high and low points.
	std::vector<Point3D> m_chaosAttractors;
	Area* m_area = nullptr;
	World& m_world;
	Distance m_topSoilOrSandDepth;
	Distance m_endBedrockDepth;
	Distance m_lavaDepth;
	MaterialTypeId m_bedrockType;
	MaterialTypeId m_sandOrSoilType;
	FluidTypeId m_oceanFluidType;
	BuildArea(World& world, Simulation& simulation, Point3D location);
	void makeUnderFluidSurfaceArea();
	void makeFluidSurfaceArea();
	void makeTerrestrialArea();
	void makeClimate();
	void makeAttractors();
	void makeSurface();
	void makeSoilOrSand();
	void makeBedrock();
	void makeLava();
	[[nodiscard]] std::array<int64_t, 8> makeAdjacentFluidSources() const;
	[[nodiscard]] CuboidSet makeSmallRivers(const std::array<int64_t, 8>& adjacentFluidSources) const;
	[[nodiscard]] CuboidSet makeSmallLakes(CuboidSet rivers) const;
	void maybeMakeSmallRiversAndLakes();
	void maybeMakeShore();
	void makePlants();
	void makeAnimals();
	void maybeMakeRoads();
	void maybeMakeSettlements();
	void smooth(float factor);
};