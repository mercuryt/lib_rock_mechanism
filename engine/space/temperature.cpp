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
	Items& items = m_area.getItems();
	assert(FluidType::getFreezesInto(fluidType).exists());
	MaterialTypeId materialType = FluidType::getFreezesInto(fluidType);
	SmallSet<FluidGroupId> groups;
	for(FluidData fluid : m_fluid.queryGetAllWithCondition(cuboids, [fluidType](FluidData fluid){ return fluid.type == fluidType; }))
		groups.maybeInsert(fluid.group);
	for(FluidGroupId id : groups)
	{
		FluidGroup& group = m_area.m_hasFluidGroups.byId(id);
		CuboidSet toFreeze = group.m_occupied.intersection(cuboids);
		int64_t fluidVolume = (group.m_volume * toFreeze.volume()) / group.m_occupied.volume();
		m_fluid.removeWithCondition(toFreeze, [fluidType](FluidData fluid) { return fluid.type == fluidType; });
		group.m_volume -= fluidVolume;
		Distance zLevel = toFreeze.boundry().m_low.z();
		// Iterate untill all fluidVolume is solidified.
		while(fluidVolume > 0 && zLevel < m_sizeZ)
		{
			CuboidSet zSlice = toFreeze.slicedAtZ(zLevel);
			int64_t zSliceVolume = zSlice.volume();
			int64_t zSliceCapacity = zSliceVolume * Config::maxPointVolume.get();
			int64_t volumeOfItemsWithFluidType{0};
			SmallSet<ItemIndex> itemsInZSlice = m_items.queryAllWithCondition(zSlice, [materialType, &items](ItemIndex item){
				// TODO: It would be strange if a large object was destroyed by the freezing of a small fluid group.
				return items.getMaterialType(item) == materialType;
			});
			for(ItemIndex item : itemsInZSlice)
				volumeOfItemsWithFluidType += items.getVolume(item).get();
			zSliceCapacity -= volumeOfItemsWithFluidType;
			if(zSliceCapacity <= fluidVolume)
			{
				// There is enough fluidVolume to fill zSlice solid.
				fluidVolume -= zSliceCapacity;
				solid_setAll(zSlice, materialType, false);
				++zLevel;
			}
			else
			{
				// There is not enough to fill zSlice, create chunks and piles instead.
				static ItemTypeId chunk = ItemType::byName("chunk");
				static ItemTypeId pile = ItemType::byName("pile");
				int64_t chunkVolume = Shape::getCollisionVolumeAtLocation(ItemType::getShape(chunk)).get();
				int64_t numberOfChunks = fluidVolume / chunkVolume;
				fluidVolume -= numberOfChunks * chunkVolume;
				int64_t volumeTurnedIntoPile = fluidVolume; // Pile full displacement is 1.
				Quantity chunksPerPoint = {(QuantityWidth)std::max(1L, numberOfChunks / zSliceVolume)};
				Quantity pilePerPoint = {(QuantityWidth)std::max(1L, volumeTurnedIntoPile / zSliceVolume)};
				for(Cuboid cuboid : zSlice)
					for(Point3D point : cuboid)
					{
						if(numberOfChunks != 0)
						{
							items.create({
								.itemType=chunk,
								.materialType=materialType,
								.location=point,
								.quantity=chunksPerPoint,
							});
							numberOfChunks -= chunksPerPoint.get();
						}
						if(volumeTurnedIntoPile == 0)
							break;
						items.create({
							.itemType=pile,
							.materialType=materialType,
							.location=point,
							.quantity=pilePerPoint,
						});
						volumeTurnedIntoPile -= pilePerPoint.get();
					}
				fluidVolume = 0;
			}
		}
		m_area.m_hasTemperature.maybeRemoveFreezeableFluidGroupAboveGround(fluidType, id);
	}
}
void Space::temperature_meltSolid(const CuboidSet& cuboids, const MaterialTypeId materialType)
{
	auto condition = [materialType](MaterialTypeId solidType) { return solidType == materialType; };
	CuboidSet toMelt = m_solid.queryGetAllCuboidsWithCondition(cuboids, condition);
	if(toMelt.empty())
		return;
	solid_setNotAll(toMelt);
	FluidTypeId fluidType = MaterialType::getMeltsInto(materialType);
	fluid_add(toMelt, Config::maxPointVolume.get() * toMelt.volume(), fluidType);
}
void Space::temperature_meltFeatures(const CuboidSet& cuboids, const MaterialTypeId materialType)
{
	FluidTypeId fluidType = MaterialType::getMeltsInto(materialType);
	auto condition = [&](PointFeature feature) { return feature.materialType == materialType; };
	CuboidSet toMelt = m_features.queryGetAllCuboidsWithCondition(cuboids, condition);
	pointFeature_removeAllWithCondition(cuboids, condition);
	fluid_add(cuboids, Config::Physics::volumeOfFluidToGenerateWhenAPointFeatureMelts.get() * toMelt.volume(), fluidType);
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
		ItemIndex item = ref.getIndex(items.m_referenceData);
		CuboidSet occupied = items.getOccupied(item);
		const CollisionVolume volume = Shape::getTotalCollisionVolume(items.getShape(item)) / occupied.volume();
		for(const Cuboid cuboid : occupied)
			for(const Point3D point : cuboid)
				volumeMelted.getOrInsert(point, {0}) += volume;
		items.destroy(item);
	}
	for(const auto& [point, volume] : volumeMelted)
		// TODO:(optimization) do this by cuboids rather then by points.
		fluid_add(CuboidSet::create(point), volume.get(), fluidType);
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