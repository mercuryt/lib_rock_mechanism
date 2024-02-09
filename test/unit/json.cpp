#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actor.h"
TEST_CASE("json")
{
	Simulation simulation;
	Faction& faction = simulation.createFaction(L"tower of power");
	const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	const MaterialType& dirt = MaterialType::byName("dirt");
	const MaterialType& wood = MaterialType::byName("poplar wood");
	const MaterialType& bronze = MaterialType::byName("bronze");
	const PlantSpecies& sage = PlantSpecies::byName("sage brush");
	const FluidType& water = FluidType::byName("water");
	const ItemType& axe = ItemType::byName("axe");
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, dirt);
	area.m_hasFarmFields.registerFaction(faction);

	Cuboid farmBlocks{area.getBlock(1,7,1), area.getBlock(1,6,1)};
	FarmField& farm = area.m_hasFarmFields.at(faction).create(farmBlocks);
	area.m_hasFarmFields.at(faction).setSpecies(farm, sage);
	Actor& dwarf1 = simulation.createActor(dwarf, area.getBlock(5,5,1), 90);
	dwarf1.setFaction(&faction);
	std::wstring name = dwarf1.m_name;
	area.getBlock(8, 8, 1).m_hasPlant.createPlant(sage, 99);
	Plant& sage1 = area.getBlock(8,8,1).m_hasPlant.get();
	sage1.setMaybeNeedsFluid();
	area.getBlock(3, 8, 1).addFluid(10, water);
	Item& axe1 = simulation.createItemNongeneric(axe, bronze, 10, 10);
	axe1.setLocation(area.getBlock(1,2,1));
	area.getBlock(1,8,1).m_hasBlockFeatures.construct(BlockFeatureType::stairs, wood);
	area.getBlock(9,1,1).m_hasBlockFeatures.construct(BlockFeatureType::door, wood);
	area.m_fires.ignite(area.getBlock(9,1,1), wood);

	Json areaData = area.toJson();
	Json simulationData = simulation.toJson();
	std::string dataString = areaData.dump();
	REQUIRE(!dataString.empty());
	Simulation simulation2(simulationData);
	Area& area2 = simulation2.loadAreaFromJson(areaData);
	Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");

	REQUIRE(area2.m_sizeX == area2.m_sizeY);
      	REQUIRE(area2.m_sizeX == area2.m_sizeZ);
	REQUIRE(area2.m_sizeX == 10);
	REQUIRE(area2.getBlock(5,5,0).isSolid());
	REQUIRE(area2.getBlock(5,5,0).getSolidMaterial() == dirt);

	REQUIRE(area2.getBlock(8,8,1).m_hasPlant.exists());
	Plant& sage2 = area2.getBlock(8,8,1).m_hasPlant.get();
	REQUIRE(sage2.m_plantSpecies == sage);
	REQUIRE(sage2.getGrowthPercent() == 99);
	REQUIRE(sage2.getStepAtWhichPlantWillDieFromLackOfFluid());
	REQUIRE(sage2.m_fluidEvent.exists());
	REQUIRE(!sage2.m_growthEvent.exists());

	Block& waterLocation = area2.getBlock(3,8,1);
	REQUIRE(waterLocation.m_totalFluidVolume == 10);
	REQUIRE(waterLocation.volumeOfFluidTypeContains(water) == 10);
	FluidGroup& fluidGroup = *waterLocation.getFluidGroup(water);
	REQUIRE(area2.m_unstableFluidGroups.contains(&fluidGroup));

	REQUIRE(!area2.getBlock(5,5,1).m_hasActors.empty());
	Actor* dwarf2 = area2.getBlock(5,5,1).m_hasActors.getAll()[0];
	REQUIRE(&dwarf2->m_species == &dwarf);
	REQUIRE(dwarf2->m_canGrow.growthPercent() == 90);
	REQUIRE(dwarf2->m_name == name);
	REQUIRE(dwarf2->m_mustEat.hasHungerEvent());
	REQUIRE(dwarf2->m_mustDrink.thirstEventExists());
	REQUIRE(dwarf2->m_mustSleep.hasTiredEvent());
	REQUIRE(dwarf2->getFaction() == &faction2);

	REQUIRE(!area2.getBlock(1,2,1).m_hasItems.empty());
	Item* axe2 = area2.getBlock(1,2,1).m_hasItems.getAll()[0];
	REQUIRE(axe2->m_materialType == bronze);
	REQUIRE(axe2->m_quality == 10);
	REQUIRE(axe2->m_percentWear == 10);
	REQUIRE(area2.getBlock(1,8,1).m_hasBlockFeatures.contains(BlockFeatureType::stairs));
	REQUIRE(area2.getBlock(1,8,1).m_hasBlockFeatures.at(BlockFeatureType::stairs)->materialType == &wood);
	REQUIRE(area2.getBlock(9,1,1).m_hasBlockFeatures.contains(BlockFeatureType::door));

	REQUIRE(area2.m_fires.contains(area2.getBlock(9,1,1), wood));
	Fire& fire = area2.m_fires.at(area2.getBlock(9,1,1), wood);
	REQUIRE(fire.m_stage == FireStage::Smouldering);
	REQUIRE(fire.m_hasPeaked == false);
	REQUIRE(fire.m_event.exists());

	REQUIRE(area2.m_hasFarmFields.contains(faction2));
	REQUIRE(area2.getBlock(1,6,1).m_isPartOfFarmField.contains(faction2));
	REQUIRE(area2.getBlock(1,6,1).m_isPartOfFarmField.get(faction2)->plantSpecies == &sage);
	REQUIRE(area2.getBlock(1,7,1).m_isPartOfFarmField.contains(faction2));
}
