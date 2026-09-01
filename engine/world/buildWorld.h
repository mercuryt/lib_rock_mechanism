#pragma once
#include "../numericTypes/idTypes.h"
#include "../geometry/cuboid.h"
#include <memory>

class Simulation;
class random;
struct World;
class Random;

struct WorldParamaters
{
	Cuboid boundry;
	Distance seaLevel;
	float fuzz;
	float chunkiness;
	int oceanFloorAttractorCount;
	int mountainPeakAttractorCount;
	float seaLevelAttractorWeight;
	float oceanFloorAttractorWeight;
	float mountainPeakAttractorWeight;
	float seaLevelAttractorDistanceExponent;
	float oceanFloorAttractorDistanceExponent;
	float mountainPeakAttractorDistanceExponent;
	Distance oceanFloorMaxDepth;
	Distance oceanFloorMinDepth;
	Distance mountainPeakMaxHeight;
	Distance mountainPeakMinHeight;
	FluidTypeId primaryFluid;
	MaterialTypeId primaryBedrockMaterial;
	int riverHeadwatersCount;
	int64_t riverHeadwatersMinVolume;
	int64_t riverMaxHeadwatersVolume;
	Distance riverHeadwatersZLevel;
	float riverFractionToMakeSmallLakes;
};

struct BuildWorld
{
	WorldParamaters m_paramaters;
	std::unique_ptr<World> m_world;
	Random* m_random;
	std::vector<std::tuple<Point3D, float, float>> m_attractorsWithMagnitudesAndDistanceExponents;
	BuildWorld(Simulation& simulation, WorldParamaters& paramaters);
	void makeAttractors();
	void makeLand();
	void makeOceans();
	void makeRivers();
	void makeLakes();
	void makeFoliageMass();
	void makeTrees();
	[[nodiscard]] Distance averageAttractors(Point3D point);
};