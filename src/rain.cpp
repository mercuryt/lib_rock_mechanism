#include "rain.h"
#include "plant.h"
#include "area.h"
#include "config.h"
#include "randomUtil.h"
void AreaHasRain::start(const FluidType& fluidType, uint32_t intensityPercent, uint32_t stepsDuration)
{
	m_currentlyRainingFluidType = &fluidType;
	m_intensityPercent = intensityPercent;
	for(Plant* plant : m_area.m_hasPlants.getPlantsOnSurface())
		if(plant->m_plantSpecies.fluidType == fluidType && plant->m_location.m_exposedToSky)
			plant->setHasFluidForNow();
	m_stopEvent.schedule(stepsDuration, *this);
}
void AreaHasRain::stop()
{
	m_currentlyRainingFluidType = nullptr;
	m_intensityPercent = 0;
}
void AreaHasRain::writeStep()
{
	if(m_currentlyRainingFluidType != nullptr)
		for(Block& block : m_area.getZLevel(m_area.m_sizeZ - 1))
			if(randomUtil::chance((float)m_intensityPercent / (float)Config::rainFrequencyModifier))
				block.addFluid(1, *m_currentlyRainingFluidType);
}
