#include "blocks.h"
#include "area.h"
#include "fluidType.h"
#include "item.h"
void Blocks::temperature_updateDelta(BlockIndex index, int32_t deltaDelta)
{
	m_temperatureDelta[index] += deltaDelta;
	Temperature temperature = m_temperatureDelta[index] + temperature_getAmbient(index);
	if(solid_is(index))
	{
		auto& material = solid_get(index);
		if(material.burnData != nullptr && material.burnData->ignitionTemperature <= temperature && (m_fires.at(index) == nullptr || !m_fires.at(index)->contains(&material)))
			m_area.m_fires.ignite(index, material);
		else if(material.meltingPoint != 0 && material.meltingPoint <= temperature)
			temperature_melt(index);
	}
	else
	{
		plant_setTemperature(index, temperature);
		blockFeature_setTemperature(index, temperature);
		actor_setTemperature(index, temperature);
		item_setTemperature(index, temperature);
		//TODO: FluidGroups.
	}
}
void Blocks::temperature_freeze(BlockIndex index, const FluidType& fluidType)
{
	assert(fluidType.freezesInto != nullptr);
	static const ItemType& chunk = ItemType::byName("chunk");
	uint32_t chunkVolume = item_getCount(index, chunk, *fluidType.freezesInto);
	uint32_t fluidVolume = fluid_volumeOfTypeContains(index, fluidType);
	// If full freeze solid, otherwise generate frozen chunks.
	if(chunkVolume + fluidVolume >= Config::maxBlockVolume)
	{
		solid_set(index, *fluidType.freezesInto, false);
		uint32_t remainder = chunkVolume + fluidVolume - Config::maxBlockVolume;
		(void)remainder;
		//TODO: add remainder to fluid group or above block.
	}
	else
		item_addGeneric(index, chunk, *fluidType.freezesInto,  fluidVolume);
}
void Blocks::temperature_melt(BlockIndex index)
{
	assert(solid_is(index));
	assert(solid_get(index).meltsInto != nullptr);
	const FluidType& fluidType = *solid_get(index).meltsInto;
	solid_setNot(index);
	fluid_add(index, Config::maxBlockVolume, fluidType);
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
const Temperature& Blocks::temperature_getAmbient(BlockIndex index) const 
{
	if(m_underground[index])
	{
		if(getZ(index) <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_area.m_hasTemperature.getAmbientSurfaceTemperature();
}
Temperature Blocks::temperature_getDailyAverageAmbient(BlockIndex index) const 
{
	if(m_underground[index])
	{
		if(getZ(index) <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_area.m_hasTemperature.getDailyAverageAmbientSurfaceTemperature();
}
Temperature Blocks::temperature_get(BlockIndex index) const
{
	return temperature_getAmbient(index) + m_temperatureDelta.at(index);
}
