#include "makeAnimals.h"
#include "buildArea.h"
#include "world.h"
#include "../config/world.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../plants.h"
#include "../actors/actors.h"
#include "../definitions/animalSpecies.h"
#include "../definitions/plantSpecies.h"
#include "../definitions/moveType.h"

MakeAnimals::MakeAnimals(BuildArea& buildArea) :
	m_buildArea(buildArea)
{
	if(m_buildArea.m_world.m_fluid.queryAny(m_buildArea.m_area->m_location))
	{
		// Underwater: only aquatic life here.
		m_aquatic = true;
	}
	else if(m_buildArea.m_world.m_fluid.queryAny(m_buildArea.m_area->m_location.below()))
	{
		// Fluid surface level: aquatic and flying.
		m_aquatic = true;
		m_arial = true;
	}
	else if(m_buildArea.m_world.m_solid.queryAny(m_buildArea.m_area->m_location))
	{
		// Undergroud, nothing here.
	}
	else if(m_buildArea.m_world.m_solid.queryAny(m_buildArea.m_area->m_location.below()))
	{
		// Ground surface level: terrestrial and flying, possibly arborial and/or aquatic.
		m_terrestrial = true;
		m_arial = true;
		m_arborial = m_buildArea.m_world.m_trees.queryAny(m_buildArea.m_area->m_location);
		m_aquatic =
 			m_buildArea.m_world.m_fluid.queryAny(m_buildArea.m_area->m_location.inflatedHorizontal()) ||
 			m_buildArea.m_world.m_smallRivers.queryAny(m_buildArea.m_area->m_location);
	}
	gatherSpecies();
	makeCandidates();
	divideTarget();
	if(m_terrestrial)
		spawnTerrestrial();
	if(m_arial)
		spawnArial();
	if(m_arborial)
		spawnArborial();
	if(m_aquatic)
		spawnAquatic();
}
void MakeAnimals::gatherSpecies()
{
	Area& area = *m_buildArea.m_area;
	SmallSet<AnimalSpeciesId> specieses = AnimalSpecies::getSpeciesForClimate(
		area.m_hasTemperature.m_maxAmbiant,
		area.m_hasTemperature.m_minAmbiant,
		area.m_hasRain.minHumidity(),
		area.m_hasRain.maxHumidity()
	);
	static FluidTypeId water = FluidType::byName("water");
	for(AnimalSpeciesId species : specieses)
		if(AnimalSpecies::getEatsMeat(species))
		{
			if(AnimalSpecies::getFluidType(species) == water)
				m_aquaticCarnivores.insert(species);
			else if(MoveType::getFly(AnimalSpecies::getMoveType(species)))
				m_arialCarnivores.insert(species);
			// TODO: Something to distinguish arborial climbers from rock climbers?
			else if(MoveType::getClimb(AnimalSpecies::getMoveType(species)))
				m_arborialCarnivores.insert(species);
			else
				m_terrestrialCarnivores.insert(species);
		}
		else
		{
			if(AnimalSpecies::getFluidType(species) == water)
				m_aquaticHerbivores.insert(species);
			else if(MoveType::getFly(AnimalSpecies::getMoveType(species)))
				m_arialHerbivores.insert(species);
			else if(MoveType::getClimb(AnimalSpecies::getMoveType(species)))
				m_arborialHerbivores.insert(species);
			else
				m_terrestrialHerbivores.insert(species);
		}
}
void MakeAnimals::makeCandidates()
{
	static FluidTypeId water = FluidType::byName("water");
	Space& space = m_buildArea.m_area->getSpace();
	m_terrestrialCandidates = space.solid_getAllCuboids();
	m_terrestrialCandidates.shift(Facing6::Above);
	space.solid_removeAllFrom(m_terrestrialCandidates);
	m_aquaticCandidates = space.fluid_getAllCuboidsWithCondition([](Cuboid, FluidData fluid) { return fluid.type == water; });
	SmallSet<PlantIndex> trees = space.plant_getAllTrees();
	Plants& plants = m_buildArea.m_area->getPlants();
	for(PlantIndex plant : trees)
		m_arborialCandidates.add(plants.getOccupied(plant));
	m_arialCandidates = space.boundry().toSet();
	space.solid_removeAllFrom(m_arialCandidates);
	space.fluid_removeAllFrom(m_arialCandidates);
}
void MakeAnimals::divideTarget()
{
	m_targetMass = m_buildArea.m_world.getFoliageMass(m_buildArea.m_area->m_location) * Config::World::ratioOfAnimalMassToFoliageMass;
	int remaining = m_targetMass;
	if(m_arial)
	{
		m_arialTargetMass = m_targetMass * Config::World::ratioOfAnimalMassFlying;
		remaining -= m_arialTargetMass;
	}
	if(m_aquatic)
	{
		float ratioOfFluidToLand = m_aquaticCandidates.volume() / m_terrestrialCandidates.volume();
		m_aquaticTargetMass = remaining * ratioOfFluidToLand;
		remaining -= m_aquaticTargetMass;
	}
	if(m_arborial)
	{
		float ratioOfArborialToTerrestrial = (float)m_arborialCandidates.volume() / (float)m_terrestrialCandidates.volume();
		m_arborialTargetMass = remaining * ratioOfArborialToTerrestrial;
		remaining -= m_arborialTargetMass;
	}
	if(m_terrestrial)
		m_terrestrialTargetMass = std::max(0, remaining);
}
void MakeAnimals::spawn(int targetMass, SmallSet<AnimalSpeciesId>& species, CuboidSet& candidates)
{
	Space& space = m_buildArea.m_area->getSpace();
	Actors& actors = m_buildArea.m_area->getActors();
	Random& random = m_buildArea.m_area->m_simulation.m_random;
	int currentMass = 0;
	if(!species.empty())
	{
		while(currentMass < targetMass && !candidates.empty())
		{
			ActorIndex animal = actors.create({
				.species=random.getInVector(species.getVector()),
				.percentGrown={std::clamp(random.getInRange(0, 250), 5, 100)}
			});
			Point3D location = random.getInCuboidSet(candidates);
			candidates.remove(location);
			Facing4 facing = space.shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacing(location, actors.getShape(animal), actors.getMoveType(animal), {});
			if(facing != Facing4::Null)
			{
				actors.location_set(animal, location, facing);
				currentMass += actors.getMass(animal).get();
			}
			else
				actors.destroy(animal);
		}
	}
}
void MakeAnimals::spawnTerrestrial()
{
	int carnivoreTargetMass = m_terrestrialTargetMass * Config::World::ratioOfCarnivorMassToTotalAnimalMass;
	int herbivoreTargetMass = m_terrestrialTargetMass - carnivoreTargetMass;
	// Terestrial herbivores.
	spawn(herbivoreTargetMass, m_terrestrialHerbivores, m_terrestrialCandidates);
	// Terestrial Carnivors.
	spawn(carnivoreTargetMass, m_terrestrialCarnivores, m_terrestrialCandidates);
}
void MakeAnimals::spawnArial()
{
	int carnivoreTargetMass = m_arialTargetMass * Config::World::ratioOfCarnivorMassToTotalAnimalMass;
	int herbivoreTargetMass = m_arialTargetMass - carnivoreTargetMass;
	// Terestrial herbivores.
	spawn(herbivoreTargetMass, m_arialHerbivores, m_arialCandidates);
	// Terestrial Carnivors.
	spawn(carnivoreTargetMass, m_arialCarnivores, m_arialCandidates);
}
void MakeAnimals::spawnArborial()
{
	int carnivoreTargetMass = m_arborialTargetMass * Config::World::ratioOfCarnivorMassToTotalAnimalMass;
	int herbivoreTargetMass = m_arborialTargetMass - carnivoreTargetMass;
	// Terestrial herbivores.
	spawn(herbivoreTargetMass, m_arborialHerbivores, m_arborialCandidates);
	// Terestrial Carnivors.
	spawn(carnivoreTargetMass, m_arborialCarnivores, m_arborialCandidates);
}
void MakeAnimals::spawnAquatic()
{
	int carnivoreTargetMass = m_aquaticTargetMass * Config::World::ratioOfCarnivorMassToTotalAnimalMass;
	int herbivoreTargetMass = m_aquaticTargetMass - carnivoreTargetMass;
	// Terestrial herbivores.
	spawn(herbivoreTargetMass, m_aquaticHerbivores, m_aquaticCandidates);
	// Terestrial Carnivors.
	spawn(carnivoreTargetMass, m_aquaticCarnivores, m_aquaticCandidates);
}