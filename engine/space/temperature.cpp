#include "space.h"
#include "../area/area.h"
#include "../fluidType.h"
#include "../definitions/itemType.h"
#include "../definitions/materialType.h"
#include "../numericTypes/types.h"
void Space::temperature_updateDelta(const Point3D& point, const TemperatureDelta& deltaDelta)
{
	assert(deltaDelta != 0);
	TemperatureDelta delta = m_temperatureDelta.queryGetOneOr(point, TemperatureDelta::create(0));
	delta += deltaDelta;
	if(delta == 0)
		m_temperatureDelta.maybeRemove(point);
	else
		m_temperatureDelta.maybeInsertOrOverwrite(point, delta);
	Temperature temperature = temperature_getAmbient(point) + delta;
	const MaterialTypeId& solidMaterial = solid_get(point);
	if(solidMaterial.exists())
	{
		if(
			MaterialType::canBurn(solidMaterial) &&
			MaterialType::getIgnitionTemperature(solidMaterial) <= temperature
		)
			fire_maybeIgnite(point, solidMaterial);
		else if(
			MaterialType::getMeltingPoint(solidMaterial).exists() &&
			MaterialType::getMeltingPoint(solidMaterial) <= temperature
		)
			temperature_melt(point);
	}
	else
	{
		plant_setTemperature(point, temperature);
		pointFeature_setTemperature(point, temperature);
		actor_setTemperature(point, temperature);
		item_setTemperature(point, temperature);
		//TODO: FluidGroups.
	}
}
void Space::temperature_freeze(const Point3D& point, const FluidTypeId& fluidType)
{
	assert(FluidType::getFreezesInto(fluidType).exists());
	static ItemTypeId chunk = ItemType::byName("chunk");
	static ItemTypeId pile = ItemType::byName("pile");
	Quantity chunkQuantity = item_getCount(point, chunk, FluidType::getFreezesInto(fluidType));
	Quantity pileQuantity = item_getCount(point, chunk, FluidType::getFreezesInto(fluidType));
	CollisionVolume chunkVolumeSingle = Shape::getCollisionVolumeAtLocation(ItemType::getShape(chunk));
	CollisionVolume pileVolumeSingle = Shape::getCollisionVolumeAtLocation(ItemType::getShape(pile));
	CollisionVolume chunkVolume = chunkVolumeSingle * chunkQuantity;
	CollisionVolume pileVolume = pileVolumeSingle * pileQuantity;
	CollisionVolume fluidVolume = fluid_volumeOfTypeContains(point, fluidType);
	fluid_removeSyncronus(point, fluidVolume, fluidType);
	// If full freeze solid, otherwise generate frozen chunks.
	if(chunkVolume + pileVolume + fluidVolume >= Config::maxPointVolume)
	{
		solid_set(point, FluidType::getFreezesInto(fluidType), false);
		CollisionVolume remainder = chunkVolume + fluidVolume - Config::maxPointVolume;
		(void)remainder;
		//TODO: add remainder to fluid group or above point.
	}
	else
	{
		Quantity createChunkQuantity = Quantity::create((fluidVolume / chunkVolumeSingle).get());
		if(createChunkQuantity != 0)
		{
			[[maybe_unused]] const ItemIndex item = item_addGeneric(point, chunk, FluidType::getFreezesInto(fluidType), createChunkQuantity);
			fluidVolume -= chunkVolumeSingle.get() * createChunkQuantity.get();
		}
		Quantity createPileQuantity = Quantity::create((fluidVolume / pileVolumeSingle).get());
		if(createPileQuantity != 0)
			[[maybe_unused]] const ItemIndex item = item_addGeneric(point, pile, FluidType::getFreezesInto(fluidType), createPileQuantity);
		assert(fluidVolume == createPileQuantity.get() * pileVolumeSingle.get());
	}
}
void Space::temperature_melt(const Point3D& point)
{
	assert(solid_isAny(point));
	assert(MaterialType::getMeltsInto(solid_get(point)).exists());
	FluidTypeId fluidType = MaterialType::getMeltsInto(solid_get(point));
	solid_setNot(point);
	fluid_add(point, Config::maxPointVolume, fluidType);
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
const Temperature& Space::temperature_getAmbient(const Point3D& point) const
{
	if(!m_exposedToSky.check(point))
	{
		if(point.z() <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_area.m_hasTemperature.getAmbientSurfaceTemperature();
}
Temperature Space::temperature_getDailyAverageAmbient(const Point3D& point) const
{
	if(!m_exposedToSky.check(point))
	{
		if(point.z() <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_area.m_hasTemperature.getDailyAverageAmbientSurfaceTemperature();
}
Temperature Space::temperature_get(const Point3D& point) const
{
	Temperature ambiant = temperature_getAmbient(point);
	TemperatureDelta delta = m_temperatureDelta.queryGetOne(point);
	if(!delta.exists())
		delta = TemperatureDelta::create(0);
	if(delta < 0 && (int)delta.absoluteValue().get() > ambiant.get())
		return Temperature::create(0);
	return Temperature::create(ambiant.get() + delta.get());
}
bool Space::temperature_transmits(const Point3D& point) const
{
	if(solid_isAny(point))
		return false;
	const PointFeature& door = pointFeature_at(point, PointFeatureTypeId::Door);
	if(door.exists() && door.isClosed())
		return false;
	const PointFeature& flap = pointFeature_at(point, PointFeatureTypeId::Flap);
	if(flap.exists() && flap.isClosed())
		return false;
	const PointFeature& hatch = pointFeature_at(point, PointFeatureTypeId::Hatch);
	if(hatch.exists() && hatch.isClosed())
		return false;
	return true;
}