#pragma once
#include "plant.h"
#include <list>
#include <unordered_set>
struct FarmField
{
	std::unordered_set<Block*> blocks;
	const PlantType* plantType;
};
class HasFarmFieldsForPlayer
{
	Player& m_player;
	std::list<FarmField> m_farmFields;
	std::list<Plant*> m_plantsNeedingFluid;
	std::unordered_set<Block*> m_blocksNeedingSeedsSewn;
	bool m_plantsNeedingFluidIsSorted;
public:
	bool hasGivePlantsFluidDesignations() const
	{
		return !m_plantsNeedingFluid.empyt();
	}
	Plant* getHighPriorityPlantForGivingFluidIfAny()
	{
		if(!m_plantsNeedingFluidIsSorted)
		{
			std::ranges::sort_by(m_plantsNeedingFluid, [](Plant* a, Plant* b){ return a->m_needsFluid.getDeathStep() < b->m_needsFluid.getDeathStep(); });
			m_plantsNeedingFluidIsSorted = true;
		}
		Plant* output = m_plantsNeedingFluid.first();
		if(output.m_needsFluid.getDeathStep() - ::s_step > Config::stepsTillDiePlantPriorityOveride)
			return nullptr;
		return *m_plantsNeedingFluid.first();
	}
	bool hasSowSeedDesignations() const
	{
		return !m_blocksNeedingSeedsSewn.empty();
	}
	const PlantType& getPlantTypeFor(Block& block) const
	{
		for(FarmField& farmField : m_farmFields)
			if(farmField.blocks.contains(&block))
				return farmField.plantType;
		assert(false);
	}
	void addGivePlantFluidDesignation(Plant& plant)
	{
		assert(!m_plantsNeedingFluid.contains(plant));
		m_plantsNeedingFluidIsSorted = false;
		m_plantsNeedingFluid.push_back(&plant);
	}
	void removeGivePlantFluidDesignation(Plant& plant)
	{
		assert(m_plantsNeedingFluid.contains(plant));
		m_plantsNeedingFluid.remove(plant);
		m_plant.m_location->m_hasDesignations.remove(m_player, BlockDesignation::GiveFluid);
	}
	void addSowSeedsDesignation(Block* block)
	{
		assert(!m_blocksNeedingSeedsSewn.contains(&block));
		m_blocksNeedingSeedsSewn.insert(&block);
		block.m_hasDesignations.insert(m_player, BlockDesignation::SowSeeds);
	}
	void removeSowSeedsDesignation(Block* block)
	{
		assert(m_blocksNeedingSeedsSewn.contains(&block));
		m_blocksNeedingSeedsSewn.remove(&block);
		block.m_hasDesignations.remove(m_player, BlockDesignation::SowSeeds);
	}
	void setDayOfYear(uint32_t dayOfYear)
	{
		//TODO: Add sow designation when blocking item or plant is removed.
		for(FarmField& farmField : m_farmFields)
			if(farmField.plantType != nullptr && farmField.plantType.dayOfYearForSow == dayOfYear)
				for(Block* block : farmField.blocks)
					if(block->m_hasPlants.canGrowHere(farmField.plantType))
						addSowSeedsDesignation(block);
	}
};
class HasFarmFields
{
	std::unordered_map<Player*, HasFarmFieldsForPlayer> m_data;
public:
	void at(Player& player)
	{
		assert(m_data.contains(player));
		return m_data.at(player);
	}
	void registerPlayer(Player& player)
	{
		assert(!m_data.contains(player));
		m_data[&player](player);
	}
	void unregisterPlayer(Player& player)
	{
		assert(m_data.contains(player));
		m_data.erase(&player);
	}
};
