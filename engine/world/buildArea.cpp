#include "buildArea.h"
#include "world.h"
#include "makeAnimals.h"
#include "climate.h"
#include "../simulation/simulation.h"
#include "../simulation/hasAreas.h"
#include "../area/area.h"
#include "../config/world.h"
#include "../geometry/cuboidSetHelper.h"
#include "../plants.h"
#include "../actors/actors.h"
#include "../definitions/plantSpecies.h"
#include "../definitions/animalSpecies.h"
#include "../space/adjacentOffsets.h"
BuildArea::BuildArea(World& world, Simulation& simulation, Point3D location) :
	m_world(world)
{
	Distance size = Config::World::defaultAreaSize;
	m_area = &simulation.m_hasAreas->createArea(size, size, size, true);
	makeClimate();
	bool midair = !m_world.m_solid.queryAny(location) && (location.z() == 0 || !m_world.m_solid.queryAny(location.below()));
	if(m_world.m_fluid.queryAny(location))
		makeUnderFluidSurfaceArea();
	else if(location.z() != 0 && m_world.m_fluid.queryAny(location.below()))
		makeFluidSurfaceArea();
	else if(!midair)
		makeTerrestrialArea();
}
void BuildArea::makeUnderFluidSurfaceArea()
{
	Space& space = m_area->getSpace();
	Cuboid toFill = space.boundry();
	space.fluid_add(toFill.toSet(), toFill.volume() * Config::maxPointVolume.get(), m_world.m_fluid.queryGetOne(m_area->m_location));
	// TODO: fish and seafloor.
}
void BuildArea::makeFluidSurfaceArea()
{
	Space& space = m_area->getSpace();
	Cuboid boundry = space.boundry();
	Cuboid toFill = boundry;
	toFill.m_high.setZ(boundry.sizeZ() * Config::World::ratioOfSurfaceToTotalAreaHeight);
	space.fluid_add(toFill.toSet(), toFill.volume() * Config::maxPointVolume.get(), m_world.m_fluid.queryGetOne(m_area->m_location));
	// TODO: fish and birds.
}
void BuildArea::makeTerrestrialArea()
{
	makeAttractors();
	makeSurface();
	Random& random = m_area->m_simulation.m_random;
	m_topSoilOrSandDepth = {random.getInRange(Config::World::minBedrockStartDepth.get(), Config::World::maxBedrockStartDepth.get())};
	if(m_topSoilOrSandDepth != 0)
		makeSoilOrSand();
	m_endBedrockDepth = m_area->m_location.z() == 0 ? Config::World::lavaDepth : Distance{0};
	makeBedrock();
	smooth(Config::World::smoothingFactor);
	makeLava();
	maybeMakeSmallRiversAndLakes();
	maybeMakeShore();
	makePlants();
	makeAnimals();
	maybeMakeRoads();
	maybeMakeSettlements();
}
void BuildArea::makeClimate()
{
	m_area->m_hasRain.m_humidityBySeason = climate::humidityBySeason(m_world, m_area->m_location);
	auto [min, max] = climate::temperatureMinAndMax(m_world, m_area->m_location);
	m_area->m_hasTemperature.m_maxAmbiant = max;
	m_area->m_hasTemperature.m_minAmbiant = min;
}
void BuildArea::makeAttractors()
{
	Point3D location = m_area->m_location;
	Space& space = m_area->getSpace();
	Cuboid areaBoundry = space.boundry();
	Distance zLevel = space.m_sizeZ * Config::World::ratioOfSurfaceToTotalAreaHeight;
	m_edgeAttractors = {
		// Corners.
		Point3D(areaBoundry.m_high.x(), areaBoundry.m_high.y(), zLevel), // NorthEast
		Point3D(areaBoundry.m_low.x(), areaBoundry.m_high.y(), zLevel), // SouthEast
		Point3D(areaBoundry.m_low.x(), areaBoundry.m_low.y(), zLevel), // SouthWest
		Point3D(areaBoundry.m_high.x(), areaBoundry.m_low.y(), zLevel), // NorthWest
		// Edges
		Point3D(areaBoundry.m_high.x() / 2, areaBoundry.m_high.y(), zLevel), // North
		Point3D(areaBoundry.m_high.x(), areaBoundry.m_high.y() / 2, zLevel), // East
		Point3D(areaBoundry.m_high.x() / 2, areaBoundry.m_low.y(), zLevel), // South
		Point3D(areaBoundry.m_low.x(), areaBoundry.m_high.y() / 2, zLevel), // West
	};
	// Adjust attractor zLevels according to surrounding areas.
	Distance delta = Config::World::distanceAreaLandIsWarpedBySlopeOnWorldMap;
	for(int i{0}; i < 8; ++i)
	{
		auto [x, y] = offsetsByIndex[i];
		if(m_world.m_solid.queryAny(Point3D::create(location.applyOffset({x,y,1}))))
			m_edgeAttractors[0].setZ(zLevel + delta);
		else if(!m_world.m_solid.queryAny(Point3D::create(location.applyOffset({x,y,0}))))
			m_edgeAttractors[0].setZ(zLevel - delta);
	}
	// Make chaos attractors.
	Random& random = m_area->m_simulation.m_random;
	int chaosAttractorCount = random.getInRange(0, Config::World::maxChaosAttractorCountArea);
	m_chaosAttractors.reserve(chaosAttractorCount);
	for(int i{0}; i < chaosAttractorCount; ++i)
	{
		Point3D attractor = random.getInCuboid(areaBoundry.getFaceAbove());
		attractor.setZ(
			{random.getInRange(
				(zLevel - Config::World::maxChaosAttractorDeltaArea).get(),
				(zLevel + Config::World::maxChaosAttractorDeltaArea).get()
			)}
		);
		m_chaosAttractors.push_back(attractor);
	}
}
void BuildArea::makeSurface()
{
	Space& space = m_area->getSpace();
	Cuboid areaBoundry = space.boundry();
	int64_t spaceToFill{areaBoundry.sizeX().get() * areaBoundry.sizeY().get()};
	int64_t filledSpace{0};
	int64_t spaceToFillBroadPhase = spaceToFill * Config::World::fractionOfHeightmapToGenerateBroadPhase;
	Random& random = m_area->m_simulation.m_random;
	// Filled surface extends to the top and bottom, it is used to make surface pretend to be 2d.
	CuboidSet filledSurface;
	auto getZLevel = [this](Point3D point) -> Distance
	{
		int64_t weightedAccum{0};
		float weightAccum{0.f};
		for(Point3D attractor : m_edgeAttractors)
		{
			DistanceFractional distance = attractor.distanceToFractional(point);
			if(distance == 0)
				return attractor.z();
			float weight = 1.0 / distance.get();
			weightAccum += weight;
			weightedAccum += weight * attractor.z().get();
		}
		for(Point3D attractor : m_chaosAttractors)
		{
			DistanceFractional distance = attractor.distanceToFractional(point);
			if(distance == 0)
				return attractor.z();
			float weight = 1.0 / distance.get();
			weightAccum += weight;
			weightedAccum += weight * attractor.z().get();
		}
		return {(DistanceWidth)(weightedAccum / weightAccum)};
	};
	while(filledSpace < spaceToFillBroadPhase)
	{
		Cuboid cuboid = Cuboid::create(random.getInCuboid(areaBoundry), random.getInCuboid(areaBoundry));
		Point3D center = cuboid.getCenter();
		Distance cuboidZLevel = getZLevel(center);
		cuboidZLevel = {random.getInRange((cuboidZLevel - Config::World::terrainFuzzIntensity).get(), (cuboidZLevel + Config::World::terrainFuzzIntensity).get())};
		cuboid.m_high.setZ(cuboidZLevel);
		cuboid.m_low.setZ(areaBoundry.m_low.z());
		CuboidSet newlyAddedXY = cuboid.toSet();
		newlyAddedXY.maybeRemove(filledSurface);
		filledSpace += newlyAddedXY.volume();
		m_surface.maybeAdd(cuboid);
		cuboid.m_high.setZ(areaBoundry.m_high.z());
		filledSurface.maybeAdd(cuboid);
	}
	// Broadphase terrain generation done, fill in what's left.
	CuboidSet remainder = areaBoundry.toSet();
	remainder.maybeRemove(filledSurface);
	for(Cuboid cuboid : remainder)
	{
		Point3D center = cuboid.getCenter();
		Distance cuboidZLevel = getZLevel(center);
		cuboidZLevel = {random.getInRange(
			(cuboidZLevel - Config::World::terrainFuzzIntensity).get(),
			(cuboidZLevel + Config::World::terrainFuzzIntensity).get()
		)};
		cuboid.m_high.setZ(cuboidZLevel);
		cuboid.m_low.setZ(areaBoundry.m_low.z());
		m_surface.add(cuboid);
	}
}
void BuildArea::makeSoilOrSand()
{
	Space& space = m_area->getSpace();
	CuboidSet copy = m_surface;
	for(Cuboid& cuboid : copy)
		cuboid.m_low.setZ(std::max(cuboid.m_low.z(), cuboid.m_high.z() - m_topSoilOrSandDepth));
	Percent averageHumidity = std::accumulate(m_area->m_hasRain.m_humidityBySeason.begin(), m_area->m_hasRain.m_humidityBySeason.end(), Percent{0}) / 4;
	m_sandOrSoilType = averageHumidity < Config::World::minimumHumidityForDirt ?
		MaterialType::byName("sand"):
		MaterialType::byName("dirt");
	space.solid_setAll(copy, m_sandOrSoilType, false);
}
void BuildArea::makeBedrock()
{
	Space& space = m_area->getSpace();
	CuboidSet copy = m_surface;
	for(Cuboid& cuboid : copy)
	{
		cuboid.m_high.setZ(std::max(cuboid.m_low.z(), cuboid.m_high.z() - m_topSoilOrSandDepth - 1));
		cuboid.m_low.setZ(std::max(cuboid.m_low.z(), cuboid.m_high.z() - m_endBedrockDepth));
	}
	m_bedrockType = m_world.getBedrockType(m_area->m_location);
	space.solid_setAll(copy, m_bedrockType, false);
}
void BuildArea::makeLava()
{
	if(m_endBedrockDepth == 0)
		return;
	Cuboid cuboid  = m_area->getSpace().boundry();
	cuboid.m_high.setZ(m_endBedrockDepth - 1);
	m_area->getSpace().fluid_add(cuboid.toSet(), Config::maxPointVolume.get() * cuboid.volume(), FluidType::byName("lava"));
}
std::array<int64_t, 8> BuildArea::makeAdjacentFluidSources() const
{
	std::array<int64_t, 8> adjacentFluidSources;
	const Point3D location = m_area->m_location;
	SmallRiverData riverDataForThisLocation = m_world.m_smallRivers.queryGetOne(location);
	// Collect attractors which connect to fluid networks.
	for(int i{0}; i < 8; ++i)
	{
		auto [x, y] = offsetsByIndex[i];
		if(
			(x < 0 && location.x() == m_world.m_boundry.m_low.x()) ||
			(y < 0 && location.y() == m_world.m_boundry.m_low.x())
		)
				continue;
		Cuboid query{
			Point3D(location.x() + x, location.y() + y, location.z() + 1),
			Point3D(location.x() + x, location.y() + y, location.z() - 1)
		};
		SmallRiverData riverData = m_world.m_smallRivers.queryGetOne(query);
		if(!riverData.empty())
		{
			if(riverData.distanceFromStart > riverDataForThisLocation.distanceFromStart)
				// Adjacent location is a source.
				adjacentFluidSources[i] = riverData.flowRate;
			else
				// Adjacent location is a destination.
				adjacentFluidSources[i] = riverDataForThisLocation.flowRate;
		}
		else if(m_world.m_ocean.query(query))
			// Oceans are always destinations.
			adjacentFluidSources[i] = riverDataForThisLocation.flowRate;
		else
			adjacentFluidSources[i] = -1;
		// No need to query small lakes, they must by colocated with small rivers.
	}
	return adjacentFluidSources;
}
CuboidSet BuildArea::makeSmallRivers(const std::array<int64_t, 8>& adjacentFluidSources) const
{
	// Generate riverbeds.
	CuboidSet riverbeds;
	Point3D center = m_area->getSpace().getCenterAtGroundLevel();
	for(int i{0}; i < 8; ++i)
	{
		if(adjacentFluidSources[i] == -1)
			continue;
		if(adjacentFluidSources[i] != 0)
		{
			Point3D source = m_edgeAttractors[i];
			source.setZ(center.z());
			ParamaterizedLine line(source, center);
			CuboidSet riverbed = line.toSet();
			int requiredCrossSection = adjacentFluidSources[i] / Config::maxPointVolume.get();
			Distance currentCrossSection{1};
			while(currentCrossSection * currentCrossSection < requiredCrossSection)
				++currentCrossSection;
			riverbed.inflateDirection(Facing6::East, currentCrossSection);
			riverbed.inflateDirection(Facing6::Below, currentCrossSection);
			riverbeds.maybeAdd(riverbed);
		}
	}
	return riverbeds;
}
CuboidSet BuildArea::makeSmallLakes(CuboidSet rivers) const
{
	Point3D center = m_area->getSpace().getCenterAtGroundLevel();
	Random& random = m_area->m_simulation.m_random;
	const Space& space = m_area->getSpace();
	Cuboid boundry = space.boundry();
	Distance deflateDistance = std::min(boundry.sizeX(), boundry.sizeY()) / 2;
	Cuboid cuboid = boundry.deflated(deflateDistance);
	cuboid.m_high.z() = center.z();
	cuboid.m_low.z() = center.z() - random.getInRange(Config::World::minSmallLakeDepth.get(), Config::World::maxSmallLakeDepth.get());
	CuboidSet lakes = cuboidSetHelper::randomCluster(random,{
		.source=rivers, // This is a copy, randomCluster will modify it.
		.area=cuboid,
		.count=random.getInRange(Config::World::minSmallLakeBeds, Config::World::maxSmallLakeBeds),
		.maxRatioOfLongToShortDimension=Config::World::maxRatioOfLongToShortDimensionForLakes,
		.maxDimension=deflateDistance/Config::World::maxRatioOfSmallLakeSizeToAreaSize,
		.minDimension=deflateDistance/Config::World::minRatioOfSmallLakeSizeToAreaSize
	});
	for(Cuboid& lakeCuboid : lakes)
		lakeCuboid.m_high.setZ(center.z());
	return lakes;
}
void BuildArea::maybeMakeSmallRiversAndLakes()
{
	std::array<int64_t, 8> adjacentFluidSources = makeAdjacentFluidSources();
	CuboidSet toFill = makeSmallRivers(adjacentFluidSources);
	if(m_world.m_smallLakes.query(m_area->m_location))
		toFill.maybeAdd(makeSmallLakes(toFill));
	// Shift river and lake beds down to ensure they are fully below ground level.
	while(true)
	{
		CuboidSet adjacent = toFill;
		adjacent.inflateHorizontal({1});
		adjacent.remove(toFill);
		// Check if adjacent is below ground level.
		if(m_world.m_solid.queryGetIntersection(adjacent).volume() == adjacent.volume())
			break;
		toFill.shift({0,0,-1},{1});
	}
	CuboidSet toRemove = toFill;
	toRemove.inflateDirection(Facing6::Above, Config::World::spaceToClearAboveRiver);
	m_world.m_solid.removeAll(toRemove);
	static FluidTypeId water = FluidType::byName("water");
	Space& space = m_area->getSpace();
	space.fluid_add(toFill, toFill.volume() * Config::maxPointVolume.get(), water);
	// Add fluid sources.
	CuboidSet seeds;
	for(int i{0}; i < 8; ++i)
	{
		if(adjacentFluidSources[i] == -1)
			continue;
		if(adjacentFluidSources[i] != 0)
		{
			Point3D source = m_edgeAttractors[i];
			while(!space.fluid_contains(source, water) && source.z() > space.boundry().m_low.z())
				source.setZ(source.z() - 1);
			assert(space.fluid_contains(source, water));
			seeds.add(source);
		}
	}
	CuboidSet candidates = space.boundry().toSet();
	candidates.remove(space.boundry().deflated());
	candidates = space.fluid_queryGetCuboidsWithType(candidates, water);
	std::vector<CuboidSet> groups = cuboidSetHelper::splitIntoTouchingGroups(candidates);
	for(const CuboidSet& group : groups)
	if(group.intersects(seeds))
		space.fluid_addSource(group, water, Config::maxPointVolume);
}
void BuildArea::makePlants()
{
	SmallSet<PlantSpeciesId> specieses = PlantSpecies::getSpeciesForClimate(
		m_area->m_hasTemperature.m_maxAmbiant,
		m_area->m_hasTemperature.m_minAmbiant,
		m_area->m_hasRain.minHumidity(),
		m_area->m_hasRain.maxHumidity()
	);
	SmallSet<PlantSpeciesId> treeSpecieses;
	SmallSet<PlantSpeciesId> nontreeSpecieses;
	for(PlantSpeciesId species : specieses)
		if(PlantSpecies::getIsTree(species))
			treeSpecieses.insert(species);
		else
			nontreeSpecieses.insert(species);
	Random& random = m_area->m_simulation.m_random;
	Space& space = m_area->getSpace();
	Plants& plants = m_area->getPlants();
	Cuboid boundry = space.boundry();
	CuboidSet candidates = boundry.getFaceAbove().toSet();
	CuboidSet fluidCandidates;
	space.fluid_forEachCuboidAll([boundry, &fluidCandidates](Cuboid cuboid){
		cuboid.m_high.setZ(boundry.m_high.z());
		fluidCandidates.add(cuboid);
	});
	candidates.remove(fluidCandidates);
	Quantity trees = m_world.m_trees.queryGetOne(m_area->m_location);
	int foliageMass{0};
	int targetFoliageMass{m_world.getFoliageMass(m_area->m_location)};
	if(!treeSpecieses.empty())
	{
		for(Quantity i{0}; i < trees; ++i)
		{
			// Make a tree.
			Point3D location = random.getInCuboidSet(candidates);
			candidates.remove(location);
			PlantIndex tree = plants.create({
				.location=location,
				.species=random.getInVector(treeSpecieses.getVector()),
				.percentGrown={std::clamp(random.getInRange(0, 250), 5, 100)},
			});
			foliageMass += plants.getFoliageMass(tree).get();
			if(candidates.empty() || foliageMass >= targetFoliageMass)
				break;
		}
	}
	if(!nontreeSpecieses.empty())
	{
		while(!candidates.empty() && foliageMass < targetFoliageMass)
		{
			// Make a nontree.
			Point3D location = random.getInCuboidSet(candidates);
			candidates.remove(location);
			PlantIndex nonTree = plants.create({
				.location=location,
				.species=random.getInVector(nontreeSpecieses.getVector()),
				.percentGrown={std::clamp(random.getInRange(0, 250), 5, 100)},
			});
			foliageMass += plants.getFoliageMass(nonTree).get();
		}
	}
	// TODO: aquatic plants.
}
void BuildArea::makeAnimals()
{
	MakeAnimals(*this);
}
void BuildArea::maybeMakeRoads()
{
	Space& space = m_area->getSpace();
	Point3D center = space.getCenterAtGroundLevel();
	Point3D location = m_area->m_location;
	Distance range = std::max<Distance>({space.m_sizeX, space.m_sizeY, space.m_sizeZ});
	static FluidTypeId water = FluidType::byName("water");
	center = space.getPointInRangeWithCondition(center, range, [&space](Point3D point){
		return point.z() != 0 && !space.fluid_contains(point.below(), water);
	});
	CuboidSet roads;
	BitSet<uint8_t, 8> directions = m_world.m_roads.queryGetOne(location);
	for(int i {0}; i != 8; ++i)
		if(directions[i])
		{
			Point3D end = m_edgeAttractors[i];
			CuboidSet set = ParamaterizedLine(end, center).toSet();
			set.inflateDirection(Facing6::South);
			set.inflateDirection(Facing6::East);
			roads.maybeAdd(set);
		}
	space.solid_setNotAll(roads);
	CuboidSet bridges = roads.shifted({0,0,-1}, {1});
	bridges = space.fluid_queryGetCuboids(bridges).intersection(bridges);
	bridges.shift({0,0,1}, {1});
	roads.remove(bridges);
	MaterialTypeId woodType = MaterialType::byName("pine wood");
	space.pointFeature_construct(roads, PointFeatureTypeId::Floor, m_bedrockType);
	space.pointFeature_construct(bridges, PointFeatureTypeId::Floor, woodType);
	// Set any open space under a road (but not a bridge) as constructed dirt.
	roads.shift({0, 0, -1}, {1});
	roads.inflateDirection(Facing6::Below);
	space.solid_removeAllFrom(roads);
	if(!roads.empty())
		space.solid_setAll(roads, m_sandOrSoilType, true);
}
void BuildArea::maybeMakeSettlements()
{
	// TODO
}
void BuildArea::smooth(float factor)
{
	Space& space = m_area->getSpace();
	Random& random = m_area->m_simulation.m_random;
	std::array<CuboidSet, 4> toSmooth; // North, East, South, West.
	for(Cuboid cuboid : space.solid_getAllCuboids())
	{
		cuboid.m_low.setZ(cuboid.m_high.z());
		for(Facing6 facing{Facing6::North}; facing != Facing6::Null; facing = Facing6((int)facing + 1))
			if(facing != Facing6::Above && facing != Facing6::Below)
			{
				CuboidSet candidates = cuboid.getFace(facing).shifted(facing).toSet();
				space.solid_removeAllFrom(candidates);
				if(candidates.empty())
					continue;
				int toSkipRemaining = std::max(candidates.volume() * (1 - factor), 1.f);
				while(toSkipRemaining != 0 && !candidates.empty())
				{
					candidates.remove(random.getInCuboidSet(candidates));
					--toSkipRemaining;
				}
				if(candidates.empty())
					continue;
				candidates.shift(flipFacing6(facing));
				toSmooth[(int)facing].add(candidates);
			}
	}
	for(int i{0}; i != 4; ++i)
	{
		for(auto [cuboid, materialType] : space.solid_getAllWithCuboids(toSmooth[i]))
		{
			space.solid_setNotCuboid(cuboid);
			space.pointFeature_add(cuboid, PointFeature::create(materialType, PointFeatureTypeId::Ramp));
		}
	}
}
void BuildArea::maybeMakeShore()
{
	Space& space = m_area->getSpace();
	Random& random = m_area->m_simulation.m_random;
	Cuboid boundry = space.boundry();
	static MaterialTypeId sand = MaterialType::byName("sand");
	for(int i{0}; i != 8; ++i)
	{
		auto [x, y] = offsetsByIndex[i];
		Point3D adjacent = Point3D::create(m_area->m_location.applyOffset({x,y,0}));
		FluidTypeId fluidType = m_world.m_fluid.queryGetOne(adjacent);
		if(fluidType.exists())
		{
			Point3D attractor = m_edgeAttractors[i];
			Cuboid seed = Cuboid::create(attractor);
			Cuboid topOfBoundry = boundry.slicedAtZ(attractor.z());
			switch(i)
			{
				// case 0-3 are corners, we can skip those.
				case 4: seed = topOfBoundry.getFaceNorth(); break;
				case 5: seed = topOfBoundry.getFaceEast(); break;
				case 6: seed = topOfBoundry.getFaceSouth(); break;
				case 7: seed = topOfBoundry.getFaceWest(); break;
			}
			// Some small lake constants are reused here.
			CuboidSet toFill = cuboidSetHelper::randomCluster(random, {
				.source=seed.toSet(),
				.area=seed.inflated(Config::World::maxDepthOfSmallCove).intersection(boundry),
				.count=random.getInRange(Config::World::maxSmallCoveCuboids, Config::World::minSmallCoveCuboids),
				.maxRatioOfLongToShortDimension=Config::World::maxRatioOfLongToShortDimensionForLakes,
				.maxDimension=Config::World::maxSmallLakeDepth,
				.minDimension=Config::World::minSmallLakeDepth,
			});
			CuboidSet beach = toFill.inflated({random.getInRange(Config::World::shoreSandMax.get(), Config::World::shoreSandMin.get())});
			beach.remove(toFill);
			beach = space.solid_queryCuboids(beach).intersection(beach);
			space.solid_setAll(beach, sand, false);
		}
	}
}