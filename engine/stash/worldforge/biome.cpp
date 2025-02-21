#include "biome.h"
#include "../block.h"
#include "../random.h"
#include "../simulation/simulation.h"

// May return nullptr if the ground level is the top of the area.
BlockIndex* getAboveGroundLevel(BlockIndex& block)
{
	BlockIndex* groundLevel = &block;
	while(!groundLevel->isSolid())
		groundLevel = groundLevel->getBlockBelow();
	return groundLevel->getBlockAbove();
}
void Biome::createAnimals(Simulation& simulation, WorldLocation& location, Area& area, Random& random) const
{
	if(!random.chance(getChanceOfAnimals(location)))
		return;
	auto range = getBaseAnimalCountRange(location);
	auto count = random.getInRange(range.first, range.second);
	for(uint i = 0; i < count; ++i)
	{
		auto name = random.getInMap(getAnimalProbabilities(location));
		const AnimalSpecies& species = AnimalSpecies::byName(name);
		Actor& animal = simulation.createActor(species);
		BlockIndex* location = nullptr;
		for(uint i = 0; i < Config::maxAnimalInsertLocationSearchRetries; ++i)
		{
			auto x = random.getInRange(0u, area.m_sizeX - 1);
			auto y = random.getInRange(0u, area.m_sizeY - 1);
			location = getAboveGroundLevel(area.getBlock(x,y,0));
			auto [can, facing] = location->m_hasShapes.canEnterCurrentlyWithAnyFacingReturnFacing(animal);
			if(can)
			{
				animal.m_facing = facing;
				animal.setLocation(*location);
				area.m_hasActors.add(animal);
				break;
			}
		}
		// No place found to put animal.
		simulation.destroyActor(animal);
	}
}
// TODO: Plants for latitude, rock types for geology.
enum class GrasslandOptions { Grass, Shrub, Tree, Rocks, Gravel, Boulder, Nothing };

void GrasslandBiome::createPlantsAndRocks(BlockIndex& block, Simulation& simulation, Random& random) const
{
	BlockIndex* aboveGroundLevel = getAboveGroundLevel(block);
	if(!aboveGroundLevel)
		return;
	std::unordered_map<GrasslandOptions, float> probabilities{
		{GrasslandOptions::Grass, 1.0},
		{GrasslandOptions::Shrub, 0.08},
		{GrasslandOptions::Tree, 0.025},
		{GrasslandOptions::Rocks, 0.015},
		{GrasslandOptions::Gravel, 0.02},
		{GrasslandOptions::Boulder, 0.01},
		{GrasslandOptions::Nothing, 0.1}
	};
	switch(random.getInMap(std::move(probabilities)))
	{
		case GrasslandOptions::Grass:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("centipede grass")))
			{
				Percent growth = std::max(100, random.getInRange(0, 500));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("centipede grass"), growth);
			}
			break;
		case GrasslandOptions::Shrub:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("sage brush")))
			{
				Percent growth = std::max(100, random.getInRange(0, 800));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("sage brush"), growth);
			}
			break;
		case GrasslandOptions::Tree:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("poplar tree")))
			{
				Percent growth = std::max(100, random.getInRange(0, 900));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("poplar tree"), growth);
			}
			break;
		case GrasslandOptions::Rocks:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("chunk"), MaterialType::byName("shist"), random.getInRange(1, 50));
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case GrasslandOptions::Gravel:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("gravel"), MaterialType::byName("shist"), random.getInRange(10, 500));
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case GrasslandOptions::Boulder:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("boulder"), MaterialType::byName("shist"), 1);
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case GrasslandOptions::Nothing:
			break;
	}
}
enum class ForestOptions { Grass, Shrub, Tree, Rocks, Gravel, Boulder, Nothing };

void ForestBiome::createPlantsAndRocks(BlockIndex& block, Simulation& simulation, Random& random) const
{
	BlockIndex* aboveGroundLevel = getAboveGroundLevel(block);
	if(!aboveGroundLevel)
		return;
	std::unordered_map<ForestOptions, float> probabilities{
		{ForestOptions::Grass, 0.4},
		{ForestOptions::Shrub, 0.1},
		{ForestOptions::Tree, 0.4},
		{ForestOptions::Rocks, 0.015},
		{ForestOptions::Gravel, 0.02},
		{ForestOptions::Boulder, 0.01},
		{ForestOptions::Nothing, 0.01}
	};
	switch(random.getInMap(std::move(probabilities)))
	{
		case ForestOptions::Grass:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("centipede grass")))
			{
				Percent growth = std::max(100, random.getInRange(0, 500));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("centipede grass"), growth);
			}
			break;
		case ForestOptions::Shrub:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("sage brush")))
			{
				Percent growth = std::max(100, random.getInRange(0, 800));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("sage brush"), growth);
			}
			break;
		case ForestOptions::Tree:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("poplar tree")))
			{
				Percent growth = std::max(100, random.getInRange(0, 900));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("poplar tree"), growth);
			}
			break;
		case ForestOptions::Rocks:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("chunk"), MaterialType::byName("shist"), random.getInRange(1, 50));
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case ForestOptions::Gravel:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("gravel"), MaterialType::byName("shist"), random.getInRange(10, 500));
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case ForestOptions::Boulder:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("boulder"), MaterialType::byName("shist"), 1);
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case ForestOptions::Nothing:
			break;
	}
}
enum class DesertOptions { Grass, Shrub, Tree, Rocks, Gravel, Boulder, Nothing };

void DesertBiome::createPlantsAndRocks(BlockIndex& block, Simulation& simulation, Random& random) const
{
	BlockIndex* aboveGroundLevel = getAboveGroundLevel(block);
	if(!aboveGroundLevel)
		return;
	std::unordered_map<DesertOptions, float> probabilities{
		{DesertOptions::Grass, 0.01},
		{DesertOptions::Shrub, 0.01},
		{DesertOptions::Tree, 0.002},
		{DesertOptions::Rocks, 0.015},
		{DesertOptions::Gravel, 0.02},
		{DesertOptions::Boulder, 0.01},
		{DesertOptions::Nothing, 1.0}
	};
	switch(random.getInMap(std::move(probabilities)))
	{
		case DesertOptions::Grass:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("centipede grass")))
			{
				Percent growth = std::max(100, random.getInRange(0, 500));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("centipede grass"), growth);
			}
			break;
		case DesertOptions::Shrub:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("sage brush")))
			{
				Percent growth = std::max(100, random.getInRange(0, 800));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("sage brush"), growth);
			}
			break;
		case DesertOptions::Tree:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("palm tree")))
			{
				Percent growth = std::max(100, random.getInRange(0, 900));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("palm tree"), growth);
			}
			break;
		case DesertOptions::Rocks:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("chunk"), MaterialType::byName("shist"), random.getInRange(1, 50));
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case DesertOptions::Gravel:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("gravel"), MaterialType::byName("shist"), random.getInRange(10, 500));
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case DesertOptions::Boulder:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("boulder"), MaterialType::byName("shist"), 1);
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case DesertOptions::Nothing:
			break;
	}
}
enum class SwampOptions { Grass, Shrub, Tree, Rocks, Gravel, Boulder, Water, Nothing };

void SwampBiome::createPlantsAndRocks(BlockIndex& block, Simulation& simulation, Random& random) const
{
	BlockIndex* aboveGroundLevel = getAboveGroundLevel(block);
	if(!aboveGroundLevel)
		return;
	std::unordered_map<SwampOptions, float> probabilities{
		{SwampOptions::Grass, 0.01},
		{SwampOptions::Shrub, 0.01},
		{SwampOptions::Tree, 0.002},
		{SwampOptions::Rocks, 0.015},
		{SwampOptions::Gravel, 0.02},
		{SwampOptions::Boulder, 0.01},
		{SwampOptions::Nothing, 0.01},
		{SwampOptions::Water, 0.4}
	};
	switch(random.getInMap(std::move(probabilities)))
	{
		case SwampOptions::Grass:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("centipede grass")))
			{
				Percent growth = std::max(100, random.getInRange(0, 500));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("centipede grass"), growth);
			}
			break;
		case SwampOptions::Shrub:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("sage brush")))
			{
				Percent growth = std::max(100, random.getInRange(0, 800));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("sage brush"), growth);
			}
			break;
		case SwampOptions::Tree:
			if(aboveGroundLevel->m_hasPlant.canGrowHereEver(PlantSpecies::byName("palm tree")))
			{
				Percent growth = std::max(100, random.getInRange(0, 900));
				aboveGroundLevel->m_hasPlant.createPlant(PlantSpecies::byName("palm tree"), growth);
			}
			break;
		case SwampOptions::Rocks:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("chunk"), MaterialType::byName("shist"), random.getInRange(1, 50));
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case SwampOptions::Gravel:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("gravel"), MaterialType::byName("shist"), random.getInRange(10, 500));
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case SwampOptions::Boulder:
			{
				Item& item = simulation.createItemGeneric(ItemType::byName("boulder"), MaterialType::byName("shist"), 1);
				item.setLocation(*aboveGroundLevel);
			}
			break;
		case SwampOptions::Nothing:
			break;
		case SwampOptions::Water:
			aboveGroundLevel->getBlockBelow()->setNotSolid();
			aboveGroundLevel->getBlockBelow()->addFluid(Config::maxBlockVolume, FluidType::byName("water"));
			break;
	}
}
