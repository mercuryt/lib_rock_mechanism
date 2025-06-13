#include "blocks.h"
#include "../area/area.h"
#include "../fluidType.h"
#include "../itemType.h"
#include "materialType.h"
#include "types.h"
void Blocks::temperature_updateDelta(const BlockIndex& index, const TemperatureDelta& deltaDelta)
{
	m_temperatureDelta[index] += deltaDelta;
	Temperature temperature = Temperature::create(m_temperatureDelta[index].get() + temperature_getAmbient(index).get());
	if(solid_is(index))
	{
		auto material = solid_get(index);
		if(MaterialType::canBurn(material) && MaterialType::getIgnitionTemperature(material) <= temperature && (m_fires[index] == nullptr || !m_fires[index]->contains(material)))
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
void Blocks::temperature_freeze(const BlockIndex& index, const FluidTypeId& fluidType)
{
	assert(FluidType::getFreezesInto(fluidType).exists());
	static ItemTypeId chunk = ItemType::byName("chunk");
	static ItemTypeId pile = ItemType::byName("pile");
	Quantity chunkQuantity = item_getCount(index, chunk, FluidType::getFreezesInto(fluidType));
	Quantity pileQuantity = item_getCount(index, chunk, FluidType::getFreezesInto(fluidType));
	CollisionVolume chunkVolumeSingle = Shape::getCollisionVolumeAtLocationBlock(ItemType::getShape(chunk));
	CollisionVolume pileVolumeSingle = Shape::getCollisionVolumeAtLocationBlock(ItemType::getShape(pile));
	CollisionVolume chunkVolume = chunkVolumeSingle * chunkQuantity;
	CollisionVolume pileVolume = pileVolumeSingle * pileQuantity;
	CollisionVolume fluidVolume = fluid_volumeOfTypeContains(index, fluidType);
	fluid_removeSyncronus(index, fluidVolume, fluidType);
	// If full freeze solid, otherwise generate frozen chunks.
	if(chunkVolume + pileVolume + fluidVolume >= Config::maxBlockVolume)
	{
		solid_set(index, FluidType::getFreezesInto(fluidType), false);
		CollisionVolume remainder = chunkVolume + fluidVolume - Config::maxBlockVolume;
		(void)remainder;
		//TODO: add remainder to fluid group or above block.
	}
	else
	{
		Quantity createChunkQuantity = Quantity::create((fluidVolume / chunkVolumeSingle).get());
		if(createChunkQuantity != 0)
		{
			item_addGeneric(index, chunk, FluidType::getFreezesInto(fluidType), createChunkQuantity);
			fluidVolume -= chunkVolumeSingle.get() * createChunkQuantity.get();
		}
		Quantity createPileQuantity = Quantity::create((fluidVolume / pileVolumeSingle).get());
		if(createPileQuantity != 0)
			item_addGeneric(index, pile, FluidType::getFreezesInto(fluidType), createPileQuantity);
		assert(fluidVolume == createPileQuantity.get() * pileVolumeSingle.get());
	}
}
void Blocks::temperature_melt(const BlockIndex& index)
{
	assert(solid_is(index));
	assert(MaterialType::getMeltsInto(solid_get(index)).exists());
	FluidTypeId fluidType = MaterialType::getMeltsInto(solid_get(index));
	solid_setNot(index);
	fluid_add(index, Config::maxBlockVolume, fluidType);
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
const Temperature& Blocks::temperature_getAmbient(const BlockIndex& index) const
{
	if(!m_exposedToSky.check(index))
	{
		if(getZ(index) <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_area.m_hasTemperature.getAmbientSurfaceTemperature();
}
Temperature Blocks::temperature_getDailyAverageAmbient(const BlockIndex& index) const
{
	if(!m_exposedToSky.check(index))
	{
		if(getZ(index) <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_area.m_hasTemperature.getDailyAverageAmbientSurfaceTemperature();
}
Temperature Blocks::temperature_get(const BlockIndex& index) const
{
	Temperature ambiant = temperature_getAmbient(index);
	TemperatureDelta delta = m_temperatureDelta[index];
	if(delta < 0 && (uint)delta.absoluteValue().get() > ambiant.get())
		return Temperature::create(0);
	return Temperature::create(ambiant.get() + delta.get());
}
bool Blocks::temperature_transmits(const BlockIndex& block) const
{
	if(solid_is(block))
		return false;
	const BlockFeature* door = blockFeature_atConst(block, BlockFeatureTypeId::Door);
	if(door != nullptr && door->closed)
		return false;
	const BlockFeature* flap = blockFeature_atConst(block, BlockFeatureTypeId::Flap);
	if(flap != nullptr && flap->closed)
		return false;
	const BlockFeature* hatch = blockFeature_atConst(block, BlockFeatureTypeId::Hatch);
	if(hatch != nullptr && hatch->closed)
		return false;
	return true;
}