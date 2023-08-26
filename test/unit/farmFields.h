#include "../../lib/doctest.h"
#include "../../src/simulation.h"
#include "../../src/animalSpecies.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/plant.h"
#include "../../src/farmFields.h"
#include "../../src/cuboid.h"
#include "../../src/faction.h"
#include "../../src/sowSeeds.h"
#include "../../src/threadedTask.h"
TEST_CASE("farm fields")
{
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	uint16_t dayOfYear = wheatGrass.dayOfYearForSowStart - 1u;
	DateTime now = {12, dayOfYear, 1200 };
	Simulation simulation(now);
	Area& area = simulation.createArea(10,10,10);
	Block& block = area.m_blocks[4][4][1];
	areaBuilderUtil::setSolidLayer(area, 0, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(block, block);
	FarmField& field = area.m_hasFarmFields.at(faction).create(cuboid);
	REQUIRE(field.plantSpecies == nullptr);
	area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
	REQUIRE(field.plantSpecies == &wheatGrass);
	REQUIRE(block.m_isPartOfFarmField.contains(faction));
	REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
	// Skip ahead to planting time.
	simulation.m_hourlyEvent.unschedule();
	simulation.m_now = {1, wheatGrass.dayOfYearForSowStart, 1200 };
	area.setDateTime(simulation.m_now);
	REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	REQUIRE(block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
	REQUIRE(!block.m_reservable.isFullyReserved(faction));
	Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][1]);
	actor.setFaction(&faction);
	const SowSeedsObjectiveType objectiveType;
	REQUIRE(objectiveType.canBeAssigned(actor));
	actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
	REQUIRE(simulation.m_threadedTaskEngine.m_tasks.empty());
	actor.m_hasObjectives.getNext();
	REQUIRE(actor.m_hasObjectives.hasCurrent());
	REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sow seeds");
	REQUIRE(simulation.m_threadedTaskEngine.m_tasks.size() == 1);
	simulation.doStep();
	REQUIRE(block.m_reservable.isFullyReserved(faction));
	REQUIRE(!actor.m_canMove.getPath().empty());
	while(actor.m_canMove.getDestination() != nullptr)
		simulation.doStep();
	REQUIRE(actor.m_canMove.getPath().empty());
	REQUIRE(simulation.m_threadedTaskEngine.m_tasks.empty());
	REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
	const Step eventStep = simulation.m_step + Config::sowSeedsStepsDuration;
	while(simulation.m_step <= eventStep)
		simulation.doStep();
	REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	REQUIRE(!area.m_hasPlants.getAll().empty());
	REQUIRE(!objectiveType.canBeAssigned(actor));
	Plant& plant = area.m_hasPlants.getAll().front();
	REQUIRE(plant.m_percentGrown == 0);
	REQUIRE(&plant.m_plantSpecies == &wheatGrass);
	REQUIRE(actor.m_hasObjectives.getCurrent().name() != "sow seeds");
}
