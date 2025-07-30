#include "space.h"
#include "../area/area.h"
#include "../fluidType.h"
#include "../definitions/itemType.h"
#include "../definitions/materialType.h"
#include "../numericTypes/types.h"
void Space::temperature_updateDelta(const Point3D& point, const TemperatureDelta& deltaDelta)
{
	TemperatureDelta delta = m_temperatureDelta.queryGetOneOr(point, TemperatureDelta::create(0));
	delta += deltaDelta;
	if(delta == 0)
		m_temperatureDelta.maybeRemove(point);
	else
		m_temperatureDelta.maybeInsertOrOverwrite(point, delta);
	//TODO: delta is queried twice here.
	Temperature temperature = temperature_get(point);
	if(solid_is(point))
	{
		auto material = solid_get(point);
		if(
			MaterialType::canBurn(material) &&
			MaterialType::getIgnitionTemperature(material) <= temperature
		)
			fire_maybeIgnite(point, material);
		else if(
			MaterialType::getMeltingPoint(material).exists() &&
			MaterialType::getMeltingPoint(material) <= temperature
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
			item_addGeneric(point, chunk, FluidType::getFreezesInto(fluidType), createChunkQuantity);
			fluidVolume -= chunkVolumeSingle.get() * createChunkQuantity.get();
		}
		Quantity createPileQuantity = Quantity::create((fluidVolume / pileVolumeSingle).get());
		if(createPileQuantity != 0)
			item_addGeneric(point, pile, FluidType::getFreezesInto(fluidType), createPileQuantity);
		assert(fluidVolume == createPileQuantity.get() * pileVolumeSingle.get());
	}
}
void Space::temperature_melt(const Point3D& point)
{
	assert(solid_is(point));
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
	if(delta < 0 && (uint)delta.absoluteValue().get() > ambiant.get())
		return Temperature::create(0);
	return Temperature::create(ambiant.get() + delta.get());
}
bool Space::temperature_transmits(const Point3D& point) const
{
	if(solid_is(point))
		return false;
	const PointFeature* door = pointFeature_at(point, PointFeatureTypeId::Door);
	if(door != nullptr && door->closed)
		return false;
	const PointFeature* flap = pointFeature_at(point, PointFeatureTypeId::Flap);
	if(flap != nullptr && flap->closed)
		return false;
	const PointFeature* hatch = pointFeature_at(point, PointFeatureTypeId::Hatch);
	if(hatch != nullptr && hatch->closed)
		return false;
	return true;
}