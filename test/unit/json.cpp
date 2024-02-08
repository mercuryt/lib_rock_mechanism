#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actor.h"
TEST_CASE("json")
{
	Simulation simulation;
	const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	const MaterialType& dirt = MaterialType::byName("dirt");
	const MaterialType& bronze = MaterialType::byName("bronze");
	const PlantSpecies& sage = PlantSpecies::byName("sage brush");
	const FluidType& water = FluidType::byName("water");
	const ItemType& axe = ItemType::byName("axe");
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, dirt);
	Actor& dwarf1 = simulation.createActor(dwarf, area.getBlock(5,5,1), 90);
	std::wstring name = dwarf1.m_name;
	area.getBlock(5, 8, 1).m_hasPlant.createPlant(sage, 99);
	area.getBlock(3, 8, 1).addFluid(10, water);
	Item& axe1 = simulation.createItemNongeneric(axe, bronze, 10, 10);
	axe1.setLocation(area.getBlock(1,2,1));
	Json areaData = area.toJson();
	Simulation simulation2;
	Area& area2 = simulation2.loadAreaFromJson(areaData);
	REQUIRE(area2.m_sizeX == area2.m_sizeY);
      	REQUIRE(area2.m_sizeX == area2.m_sizeZ);
	REQUIRE(area2.m_sizeX == 10);
	REQUIRE(area2.getBlock(5,5,0).isSolid());
	REQUIRE(area2.getBlock(5,5,0).getSolidMaterial() == dirt);
	REQUIRE(area2.getBlock(5,8,1).m_hasPlant.exists());
	REQUIRE(area2.getBlock(5,8,1).m_hasPlant.get().m_plantSpecies == sage);
	REQUIRE(area2.getBlock(5,8,1).m_hasPlant.get().getGrowthPercent() == 99);
	REQUIRE(area2.getBlock(3,8,1).m_totalFluidVolume == 10);
	REQUIRE(area2.getBlock(3,8,1).volumeOfFluidTypeContains(water) == 10);
	REQUIRE(!area2.getBlock(5,5,1).m_hasActors.empty());
	Actor* dwarf2 = area2.getBlock(5,5,1).m_hasActors.getAll()[0];
	REQUIRE(&dwarf2->m_species == &dwarf);
	REQUIRE(dwarf2->m_canGrow.growthPercent() == 90);
	REQUIRE(dwarf2->m_name == name);
	REQUIRE(!area2.getBlock(1,2,1).m_hasItems.empty());
	Item* axe2 = area.getBlock(1,2,1).m_hasItems.getAll()[0];
	REQUIRE(axe2->m_materialType == bronze);
	REQUIRE(axe2->m_quality == 10);
	REQUIRE(axe2->m_percentWear == 10);
}
