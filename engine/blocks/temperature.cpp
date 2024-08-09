#include "blocks.h"
#include "../area.h"
#include "../fluidType.h"
#include "../itemType.h"
#include "materialType.h"
#include "types.h"
void Blocks::temperature_updateDelta(BlockIndex index, TemperatureDelta deltaDelta)
{
	m_temperatureDelta.at(index) += deltaDelta;
	Temperature temperature = Temperature::create(m_temperatureDelta.at(index).get() + temperature_getAmbient(index).get());
	if(solid_is(index))
	{
		auto material = solid_get(index);
		if(MaterialType::canBurn(material) && MaterialType::getIgnitionTemperature(material) <= temperature && (m_fires.at(index) == nullptr || !m_fires.at(index)->contains(material)))
			m_area.m_fires.ignite(index, material);
		else if(MaterialType::getMeltingPoint(material).exists() && MaterialType::getMeltingPoint(material) <= temperature)
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
void Blocks::temperature_freeze(BlockIndex index, FluidTypeId fluidType)
{
	assert(FluidType::getFreezesInto(fluidType).exists());
	static ItemTypeId chunk = ItemType::byName("chunk");
	Quantity chunkQuantity = item_getCount(index, chunk, FluidType::getFreezesInto(fluidType));
	CollisionVolume chunkVolume = ItemType::getVolume(chunk).toCollisionVolume() * chunkQuantity;
	CollisionVolume fluidVolume = fluid_volumeOfTypeContains(index, fluidType);
	// If full freeze solid, otherwise generate frozen chunks.
	if(chunkVolume + fluidVolume >= Config::maxBlockVolume)
	{
		solid_set(index, FluidType::getFreezesInto(fluidType), false);
		CollisionVolume remainder = chunkVolume + fluidVolume - Config::maxBlockVolume;
		(void)remainder;
		//TODO: add remainder to fluid group or above block.
	}
	else
		item_addGeneric(index, chunk, FluidType::getFreezesInto(fluidType),  chunkQuantity);
}
void Blocks::temperature_melt(BlockIndex index)
{
	assert(solid_is(index));
	assert(MaterialType::getMeltsInto(solid_get(index)).exists());
	FluidTypeId fluidType = MaterialType::getMeltsInto(solid_get(index));
	solid_setNot(index);
	fluid_add(index, Config::maxBlockVolume, fluidType);
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
const Temperature& Blocks::temperature_getAmbient(BlockIndex index) const 
{
	if(m_underground.at(index))
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
	if(m_underground.at(index))
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
	Temperature ambiant = temperature_getAmbient(index);
	TemperatureDelta delta = m_temperatureDelta.at(index);
	if(delta < 0 && (uint)delta.absoluteValue().get() > ambiant.get())
		return Temperature::create(0);
	return Temperature::create(ambiant.get() + delta.get());
}
