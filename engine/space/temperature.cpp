#include "space.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../fluidType.h"
#include "../definitions/itemType.h"
#include "../definitions/materialType.h"
#include "../numericTypes/types.h"
#include "../config/physics.h"
void Space::temperature_freeze(const CuboidSet& cuboids, const FluidTypeId fluidType)
{
	for(const Cuboid cuboid : cuboids)
		for(const Point3D point : cuboid)
			temperature_freeze(point, fluidType);
}
void Space::temperature_freeze(const Point3D point, const FluidTypeId fluidType)
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
void Space::temperature_meltSolid(const Point3D point)
{
	assert(solid_isAny(point));
	assert(MaterialType::getMeltsInto(solid_get(point)).exists());
	FluidTypeId fluidType = MaterialType::getMeltsInto(solid_get(point));
	solid_setNot(point);
	fluid_add(point, Config::maxPointVolume, fluidType);
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
void Space::temperature_meltSolid(const CuboidSet& cuboids, const MaterialTypeId materialType)
{
	assert(solid_get(cuboids) == materialType);
	solid_setNotAll(cuboids);
	FluidTypeId fluidType = MaterialType::getMeltsInto(materialType);
	fluid_add(cuboids, Config::maxPointVolume, fluidType);
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
void Space::temperature_meltFeature(const Point3D point, const MaterialTypeId materialType, const PointFeatureTypeId featureType)
{
	assert(pointFeature_getMaterialType(point, featureType) == materialType);
	FluidTypeId fluidType = MaterialType::getMeltsInto(materialType);
	pointFeature_remove(point, featureType);
	fluid_add(point, Config::Physics::volumeOfFluidToGenerateWhenAPointFeatureMelts, fluidType);
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
void Space::temperature_meltFeatures(const CuboidSet& cuboids, const MaterialTypeId materialType)
{
	FluidTypeId fluidType = MaterialType::getMeltsInto(materialType);
	pointFeature_removeAllWithCondition(cuboids, [&](const PointFeature feature) { return feature.materialType == materialType; });
	fluid_add(cuboids, Config::Physics::volumeOfFluidToGenerateWhenAPointFeatureMelts, fluidType);
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
void Space::temperature_meltItems(const CuboidSet& cuboids, const MaterialTypeId materialType)
{
	FluidTypeId fluidType = MaterialType::getMeltsInto(materialType);
	Items& items = m_area.getItems();
	SmallMap<Point3D, CollisionVolume> volumeMelted;
	SmallSet<ItemIndex> itemsToMelt = m_items.queryGetAllWithCondition(cuboids, [&](const ItemIndex item){ return items.getMaterialType(item) == materialType; });
	SmallSet<ItemReference> itemReferences;
	itemReferences.reserve(itemsToMelt.size());
	for(const ItemIndex item : itemsToMelt)
		itemReferences.insert(items.getReference(item));
	for(const ItemReference ref : itemReferences)
	{
		const ItemIndex item = ref.getIndex(items.m_referenceData);
		const CuboidSet& occupied = items.getOccupied(item);
		const CollisionVolume volume = Shape::getTotalCollisionVolume(items.getShape(item)) / occupied.volume();
		for(const Cuboid cuboid : occupied)
			for(const Point3D point : cuboid)
				volumeMelted.getOrInsert(point, {0}) += volume;
		items.destroy(item);
	}
	for(const auto& [point, volume] : volumeMelted)
		fluid_add(point, volume, fluidType);
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
}
const Temperature Space::temperature_getAmbient(const Point3D point) const
{
	if(!m_exposedToSky.check(point))
	{
		if(point.z() <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_area.m_hasTemperature.m_ambiant;
}
Temperature Space::temperature_getDailyAverageAmbient(const Point3D point) const
{
	if(!m_exposedToSky.check(point))
	{
		if(point.z() <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_area.m_hasTemperature.getDailyAverageAmbientSurfaceTemperature(m_area);
}
Temperature Space::temperature_get(const Point3D point) const
{
	return m_area.m_hasTemperature.get(m_area, point);
}
bool Space::temperature_transmits(const Point3D point) const
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
CuboidSet Space::temperature_queryTransmitsCuboidsIntersection(const CuboidSet& cuboids) const
{
	CuboidSet output = cuboids;
	m_solid.queryForEachCuboid(cuboids, [&](const Cuboid solidCuboid){
		output.maybeRemove(solidCuboid);
	});
	m_features.queryForEachWithCuboids(output, [&](const Cuboid featureCuboid, const PointFeature feature){
		if(feature.isClosed())
			output.maybeRemove(featureCuboid);
		else
			assert(
				feature.pointFeatureType == PointFeatureTypeId::Door ||
				feature.pointFeatureType == PointFeatureTypeId::Flap ||
				feature.pointFeatureType == PointFeatureTypeId::Hatch
			);
	});
	return output;
}