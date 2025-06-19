#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/area/area.h"
#include "../../engine/items/items.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/plants.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/craft.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/definitions/animalSpecies.h"
#include "reference.h"
#include "types.h"
#include <functional>
TEST_CASE("craft")
{
	MaterialTypeId wood = MaterialType::byName("poplar wood");
	MaterialTypeId bronze = MaterialType::byName("bronze");
	SkillTypeId woodWorking = SkillType::byName("wood working");
	CraftStepTypeCategoryId sawCategory = CraftStepTypeCategory::byName("saw");
	CraftJobTypeId woodBucket = CraftJobType::byName("wood bucket");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));
	BlockIndex chiselLocation = blocks.getIndex_i(3, 5, 1);
	BlockIndex sawingLocation = blocks.getIndex_i(9, 9, 1);
	FactionId faction = simulation.createFaction("Tower Of Power");
	area.m_hasCraftingLocationsAndJobs.addFaction(faction);
	area.m_hasStockPiles.registerFaction(faction);
	area.m_blockDesignations.registerFaction(faction);
	CHECK(!area.m_hasCraftingLocationsAndJobs.getForFaction(faction).hasLocationsFor(woodBucket));
	area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addLocation(CraftStepTypeCategory::byName("carve"), chiselLocation);
	area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addLocation(CraftStepTypeCategory::byName("assemble"), chiselLocation);
	area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addLocation(sawCategory, sawingLocation);
	CHECK(area.m_hasCraftingLocationsAndJobs.getForFaction(faction).hasLocationsFor(woodBucket));
	const CraftObjectiveType& craftObjectiveTypeWoodWorking = static_cast<const CraftObjectiveType&>(ObjectiveType::getByName("craft: wood working"));
	const CraftObjectiveType& craftObjectiveTypeAssembling = static_cast<const CraftObjectiveType&>(ObjectiveType::getByName("craft: assembling"));
	ActorIndex dwarf1 = actors.create({
		.species=AnimalSpecies::byName("dwarf"),
		.location=blocks.getIndex_i(1, 1, 1),
		.faction=faction,
	});
	ActorReference dwarf1Ref = actors.m_referenceData.getReference(dwarf1);
	SmallSet<CraftJob*> emptyJobSet;
	SUBCASE("infastructure")
	{
		area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addJob(woodBucket, wood, Quantity::create(1));
		/*
		// TODO: why doesnt' this method exist anymore?
		auto pair = area.m_hasCraftingLocationsAndJobs.getForFaction(faction).getJobAndLocationForWhileExcluding(dwarf1, woodWorking, emptyJobSet);
		CraftJob* job = pair.first;
		BlockIndex location = pair.second;
		CHECK(job != nullptr);
		CHECK(location.exists());
		CHECK(location == sawingLocation);
		CHECK(&job->craftJobType == woodBucket);
		CHECK(job->workPiece.empty());
		CHECK(job->minimumSkillLevel == 0);
		CHECK(job->totalSkillPoints == 0);
		CHECK(!job->craftJobType.stepTypes.empty());
		CHECK(job == area.m_hasCraftingLocationsAndJobs.getForFaction(faction).getJobForAtLocation(dwarf1, woodWorking, sawingLocation, emptyJobSet));
		area.m_hasCraftingLocationsAndJobs.getForFaction(faction).removeLocation(sawCategory, sawingLocation);
		pair = area.m_hasCraftingLocationsAndJobs.getForFaction(faction).getJobAndLocationForWhileExcluding(dwarf1, woodWorking, emptyJobSet);
		CHECK(pair.first == nullptr);
		*/
	}
	SUBCASE("craft bucket")
	{
		BlockIndex boardLocation = blocks.getIndex_i(3, 4, 1);
		ItemIndex board = items.create({.itemType=ItemType::byName("board"), .materialType=wood, .location=boardLocation, .quantity=Quantity::create(10u)});
		ItemIndex rope = items.create({
			.itemType=ItemType::byName("rope"),
			.materialType=MaterialType::byName("plant matter"),
			.location=blocks.getIndex_i(8, 6, 1),
			.quantity=Quantity::create(10u),
		});
		items.create({
			.itemType=ItemType::byName("saw"),
			.materialType=bronze,
			.location=blocks.getIndex_i(3, 7, 1),
			.quality=Quality::create(25u),
			.percentWear=Percent::create(0)
		});
		items.create({
			.itemType=ItemType::byName("mallet"),
			.materialType=bronze,
			.location=blocks.getIndex_i(4, 9, 1),
			.quality=Quality::create(25u),
			.percentWear=Percent::create(0)
		});
		items.create({
			.itemType=ItemType::byName("chisel"),
			.materialType=bronze,
			.location=blocks.getIndex_i(4, 9, 1),
			.quality=Quality::create(25u),
			.percentWear=Percent::create(0)
		});
		CHECK(!craftObjectiveTypeWoodWorking.canBeAssigned(area, dwarf1));
		area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addJob(woodBucket, wood, Quantity::create(1));
		// There is wood working to be done.
		CHECK(craftObjectiveTypeWoodWorking.canBeAssigned(area, dwarf1));
		// There is not assembling to be done yet.
		CHECK(!craftObjectiveTypeAssembling.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, craftObjectiveTypeWoodWorking.getId(), Priority::create(100));
		actors.objective_setPriority(dwarf1, craftObjectiveTypeAssembling.getId(), Priority::create(100));
		CHECK(actors.objective_getCurrentName(dwarf1) == "craft: wood working");
		CraftJob* job = area.m_hasCraftingLocationsAndJobs.getForFaction(faction).getJobForAtLocation(dwarf1, woodWorking, sawingLocation, emptyJobSet);
		CHECK(job != nullptr);
		CHECK(job->getStep() == 1);
		// Find a project to join, reserve, and activate
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) == "craft: wood working");
		CHECK(job->craftStepProject->reservationsComplete());
		CHECK(job->craftStepProject->getWorkers().contains(dwarf1Ref));
		const ProjectWorker& projectWorker = job->craftStepProject->getProjectWorkerFor(dwarf1Ref);
		// Select a haul subproject.
		simulation.doStep();
		CHECK(projectWorker.haulSubproject != nullptr);
		// Find a path.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		CHECK(items.isAdjacentToLocation(board, actors.move_getDestination(dwarf1)));
		SUBCASE("success")
		{
			simulation.fastForwardUntillActorIsAdjacentToItem(area, dwarf1, board);
			std::function<bool()> predicate = [&](){ return blocks.item_getCount(sawingLocation, ItemType::byName("bucket"), wood) == 1; };
			simulation.fastForwardUntillPredicate(predicate, 11);
			CHECK(actors.getLocation(dwarf1) == sawingLocation);
			CHECK(job->getStep() == 2);
			CHECK(job->workPiece.exists());
			ItemIndex bucket = job->workPiece.getIndex(items.m_referenceData);
			CHECK(items.getLocation(bucket) == sawingLocation);
			CHECK(area.getTotalCountOfItemTypeOnSurface(ItemType::byName("board")) == 9);
			// There is more wood working to be done.
			CHECK(craftObjectiveTypeWoodWorking.canBeAssigned(area, dwarf1));
			// There is still no assembling to be done.
			CHECK(!craftObjectiveTypeAssembling.canBeAssigned(area, dwarf1));
			predicate = [&]() { return items.getLocation(bucket) == chiselLocation && job->getStep() == 3; };
			simulation.fastForwardUntillPredicate(predicate, 21);
			// There is no wood working to be done.
			CHECK(!craftObjectiveTypeWoodWorking.canBeAssigned(area, dwarf1));
			// There is assembling to be done.
			CHECK(craftObjectiveTypeAssembling.canBeAssigned(area, dwarf1));
			// Find a project to join.
			simulation.doStep();
			CHECK(actors.objective_getCurrentName(dwarf1) == "craft: assembling");
			CHECK(job->craftStepProject->reservationsComplete());
			CHECK(job->craftStepProject->getWorkers().contains(dwarf1Ref));
			const ProjectWorker& projectWorker2 = job->craftStepProject->getProjectWorkerFor(dwarf1Ref);
			// Select a haul subproject.
			simulation.doStep();
			CHECK(projectWorker2.haulSubproject != nullptr);
			// Find a path.
			simulation.doStep();
			CHECK(actors.move_getDestination(dwarf1).exists());
			CHECK(items.isAdjacentToLocation(rope, actors.move_getDestination(dwarf1)));
			simulation.fastForwardUntillActorIsAdjacentToItem(area, dwarf1, rope);
			predicate = [&]() { return items.getLocation(bucket) == chiselLocation && !items.isWorkPiece(bucket); };
			simulation.fastForwardUntillPredicate(predicate, 11);
			// There is no longer wood working to be done.
			CHECK(!craftObjectiveTypeWoodWorking.canBeAssigned(area, dwarf1));
		}
		SUBCASE("project canceled")
		{
			SUBCASE("by player undesignating location")
			{
				area.m_hasCraftingLocationsAndJobs.getForFaction(faction).removeLocation(sawCategory, sawingLocation);
				// There is no longer wood working to be done because there are no locations to do it at.
				CHECK(!craftObjectiveTypeWoodWorking.canBeAssigned(area, dwarf1));
			}
			SUBCASE("by setting the location solid")
			{
				blocks.solid_set(sawingLocation, wood, false);
				// There is no longer wood working to be done because there are no locations to do it at.
				CHECK(!craftObjectiveTypeWoodWorking.canBeAssigned(area, dwarf1));
			}
			// Project is canceled and craft objective is reset.
			CHECK(!actors.project_exists(dwarf1));
			CHECK(actors.objective_getCurrent<CraftObjective>(dwarf1).getCraftJob() == nullptr);
			CHECK(actors.move_hasPathRequest(dwarf1));
			// No viable craft job found, objective cannot be completed.
			simulation.doStep();
			CHECK(actors.objective_getCurrentName(dwarf1).substr(0,5) != "craft");
		}
		SUBCASE("by destroying a reserved item")
		{
			items.destroy(board);
			// There is still wood working to be done even though there are no boards to do it with.
			CHECK(craftObjectiveTypeWoodWorking.canBeAssigned(area, dwarf1));
			CHECK(!actors.project_exists(dwarf1));
			CHECK(actors.objective_getCurrent<CraftObjective>(dwarf1).getCraftJob() == nullptr);
			CHECK(actors.move_hasPathRequest(dwarf1));
			CHECK(actors.objective_getCurrentName(dwarf1).substr(0,5) == "craft");
			CHECK(actors.objective_getCurrent<CraftObjective>(dwarf1).getFailedJobs().size() == 1);
			// Dwarf cannot create a new project as the CraftJob is now it CraftObjective.m_failedJobs.
			// TODO: Ideally this situation would result in immideate retry rather then fail, but this is ok for now.
			simulation.doStep();
			CHECK(!actors.project_exists(dwarf1));
			CHECK(actors.objective_getCurrentName(dwarf1).substr(0,5) != "craft");
		}
		SUBCASE("location inaccessable")
		{
			areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 3, 1), blocks.getIndex_i(9, 3, 1), wood);
			simulation.fastForwardUntillActorHasNoDestination(area, dwarf1);
			CHECK(job->craftStepProject == nullptr);
			CHECK(actors.objective_getCurrentName(dwarf1).substr(0,5) != "craft");
			CHECK(!actors.project_exists(dwarf1));
		}
		SUBCASE("worker dies")
		{
			actors.die(dwarf1, CauseOfDeath::thirst);
			ActorIndex dwarf2 = actors.create({
				.species=AnimalSpecies::byName("dwarf"),
				.location=blocks.getIndex_i(1, 5, 1),
				.faction=faction,
			});
			CHECK(craftObjectiveTypeWoodWorking.canBeAssigned(area, dwarf2));
		}
	}
}
