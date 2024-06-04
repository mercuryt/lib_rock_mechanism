#pragma once
#include "../types.h"
#include <cassert>
class BlockIndex;
class Plant;
struct PlantSpecies;
class BlockHasPlant final 
{
	BlockIndex& m_block;
	Plant* m_plant;
public:
	BlockHasPlant(BlockIndex& b) : m_block(b), m_plant(nullptr) { }
	void createPlant(const PlantSpecies& plantSpecies, Percent growthPercent = 0);
	void updateGrowingStatus();
	void clearPointer();
	void setTemperature(Temperature temperature);
	void erase();
	void set(Plant& plant) { assert(!m_plant); m_plant = &plant; }
	Plant& get() { assert(m_plant); return *m_plant; }
	const Plant& get() const { assert(m_plant); return *m_plant; }
	bool canGrowHereCurrently(const PlantSpecies& plantSpecies) const;
	bool canGrowHereAtSomePointToday(const PlantSpecies& plantSpecies) const;
	bool canGrowHereEver(const PlantSpecies& plantSpecies) const;
	bool anythingCanGrowHereEver() const;
	bool exists() const { return m_plant != nullptr; }
};
