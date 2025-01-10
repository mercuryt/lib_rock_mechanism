#pragma once

#include "../types.h"
#include "../config.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <algorithm>
class BlockIndex;
class Random;
class Simulation;
class Area;
class AnimalSpecies;
struct WorldLocation;
enum class BiomeId { Grassland, Forest, Desert, Swamp };
class Biome
{
public:
	void createAnimals(Simulation& simulation, WorldLocation& location, Area& area, Random& random) const;
	virtual BiomeId getType() const = 0;
	virtual double getChanceOfAnimals(WorldLocation& location) const = 0;
	virtual std::pair<uint32_t, uint32_t> getBaseAnimalCountRange(WorldLocation& location) const = 0;
	virtual void createPlantsAndRocks(BlockIndex& block, Simulation& simulation, Random& random) const = 0;
	virtual std::unordered_map<std::wstring, float> getAnimalProbabilities(WorldLocation& location) const = 0;
	virtual ~Biome() = default;
	// Infastructure
	inline static std::vector<Biome> data;
	inline static const Biome& byType(BiomeId biomeId)
	{
		return *std::find_if(data.begin(), data.end(), [&](Biome& biome){ return biome.getType() == biomeId; });
	}
};
inline void to_json(Json& data, const Biome* const& biomePointer){ data = biomePointer->getType(); }
inline void from_json(const Json& data, const Biome*& biomePointer){ biomePointer = &Biome::byType(data.get<BiomeId>()); }
class GrasslandBiome final : public Biome
{
public:
	BiomeId getType() const { return BiomeId::Grassland; }
	double getChanceOfAnimals([[maybe_unused]] WorldLocation& location) const { return 0.2; }
	std::pair<uint32_t, uint32_t> getBaseAnimalCountRange([[maybe_unused]] WorldLocation& location) const { return {0,3}; }
	std::unordered_map<std::wstring, float> getAnimalProbabilities([[maybe_unused]] WorldLocation& location) const
	{
		return {
			{"dwarf rabbit", 0.01},
			{"red fox", 0.01},
			{"red deer", 0.1},
			{"jackstock donkey", 0.1},
			{"black bear", 0.1},
			{"golden eagle", 0.1},
		};
	}
	void createPlantsAndRocks(BlockIndex& block, Simulation& simulation, Random& random) const;
};

class ForestBiome final : public Biome
{
public:
	BiomeId getType() const { return BiomeId::Forest; }
	double getChanceOfAnimals([[maybe_unused]] WorldLocation& location) const { return 0.2; }
	std::pair<uint32_t, uint32_t> getBaseAnimalCountRange([[maybe_unused]] WorldLocation& location) const { return {0,3}; }
	std::unordered_map<std::wstring, float> getAnimalProbabilities([[maybe_unused]] WorldLocation& location) const
	{
		return {
			{"dwarf rabbit", 0.01},
			{"red fox", 0.01},
			{"red deer", 0.1},
			{"jackstock donkey", 0.1},
			{"black bear", 0.1},
			{"golden eagle", 0.1},
		};
	}
	void createPlantsAndRocks(BlockIndex& block, Simulation& simulation, Random& random) const;
};

class DesertBiome final : public Biome
{
public:
	BiomeId getType() const { return BiomeId::Desert; }
	double getChanceOfAnimals([[maybe_unused]] WorldLocation& location) const { return 0.02; }
	std::pair<uint32_t, uint32_t> getBaseAnimalCountRange([[maybe_unused]] WorldLocation& location) const { return {0,3}; }
	std::unordered_map<std::wstring, float> getAnimalProbabilities([[maybe_unused]] WorldLocation& location) const
	{
		return {
			{"dwarf rabbit", 0.01},
			{"red fox", 0.01},
			{"golden eagle", 0.1},
		};
	}
	void createPlantsAndRocks(BlockIndex& block, Simulation& simulation, Random& random) const;
};

class SwampBiome final : public Biome
{
public:
	BiomeId getType() const { return BiomeId::Swamp; }
	double getChanceOfAnimals([[maybe_unused]] WorldLocation& location) const { return 0.15; }
	std::pair<uint32_t, uint32_t> getBaseAnimalCountRange([[maybe_unused]] WorldLocation& location) const { return {0,3}; }
	std::unordered_map<std::wstring, float> getAnimalProbabilities([[maybe_unused]] WorldLocation& location) const
	{
		return {
			{"dwarf rabbit", 0.01},
			{"red fox", 0.01},
			{"golden eagle", 0.1},
		};
	}
	void createPlantsAndRocks(BlockIndex& block, Simulation& simulation, Random& random) const;
};
