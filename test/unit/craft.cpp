#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/item.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/craft.h"
#include "../../engine/materialType.h"
#include <functional>
TEST_CASE("craft")
{
	const MaterialType& wood = MaterialType::byName("poplar wood");
	const MaterialType& bronze = MaterialType::byName("bronze");
	const SkillType& woodWorking = SkillType::byName("wood working");
	const SkillType& assembling = SkillType::byName("assembling");
	const CraftStepTypeCategory& sawCategory = CraftStepTypeCategory::byName("saw");
	const CraftJobType& woodBucket = CraftJobType::byName("wood bucket");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));
	BlockIndex chiselLocation = blocks.getIndex({3, 5, 1});
	BlockIndex sawingLocation = blocks.getIndex({9, 9, 1});
	Faction faction(L"tower of power");
	area.m_hasCraftingLocationsAndJobs.addFaction(faction);
	REQUIRE(!area.m_hasCraftingLocationsAndJobs.at(faction).hasLocationsFor(woodBucket));
	area.m_hasCraftingLocationsAndJobs.at(faction).addLocation(CraftStepTypeCategory::byName("carve"), chiselLocation);
	area.m_hasCraftingLocationsAndJobs.at(faction).addLocation(CraftStepTypeCategory::byName("assemble"), chiselLocation);
	area.m_hasCraftingLocationsAndJobs.at(faction).addLocation(sawCategory, sawingLocation);
	REQUIRE(area.m_hasCraftingLocationsAndJobs.at(faction).hasLocationsFor(woodBucket));
	CraftObjectiveType craftObjectiveTypeWoodWorking(woodWorking);
	CraftObjectiveType craftObjectiveTypeAssembling(assembling);
	Actor& dwarf1 = simulation.m_hasActors->createActor(ActorParamaters{
		.species=AnimalSpecies::byName("dwarf"), 
		.location=blocks.getIndex({1, 1, 1}),
		.area=&area,
	});
	dwarf1.setFaction(&faction);
	std::unordered_set<CraftJob*> emptyJobSet;
	SUBCASE("infastructure")
	{
		area.m_hasCraftingLocationsAndJobs.at(faction).addJob(woodBucket, &wood, 1);
		auto pair = area.m_hasCraftingLocationsAndJobs.at(faction).getJobAndLocationForWhileExcluding(dwarf1, woodWorking, emptyJobSet);
		CraftJob* job = pair.first;
		BlockIndex location = pair.second;
		REQUIRE(job != nullptr);
		REQUIRE(location != BLOCK_INDEX_MAX);
		REQUIRE(location == sawingLocation);
		REQUIRE(&job->craftJobType == &woodBucket);
		REQUIRE(job->workPiece == nullptr);
		REQUIRE(job->minimumSkillLevel == 0);
		REQUIRE(job->totalSkillPoints == 0);
		REQUIRE(!job->craftJobType.stepTypes.empty());
		REQUIRE(job == area.m_hasCraftingLocationsAndJobs.at(faction).getJobForAtLocation(dwarf1, woodWorking, sawingLocation, emptyJobSet));
		area.m_hasCraftingLocationsAndJobs.at(faction).removeLocation(sawCategory, sawingLocation);
		pair = area.m_hasCraftingLocationsAndJobs.at(faction).getJobAndLocationForWhileExcluding(dwarf1, woodWorking, emptyJobSet);
		REQUIRE(pair.first == nullptr);
	}
	SUBCASE("craft bucket")
	{
		BlockIndex boardLocation = blocks.getIndex({6, 6, 1});
		Item& board = simulation.m_hasItems->createItemGeneric(ItemType::byName("board"), wood, 10u);
		board.setLocation(boardLocation);
		Item& rope = simulation.m_hasItems->createItemGeneric(ItemType::byName("rope"), MaterialType::byName("plant matter"), 10u);
		rope.setLocation(blocks.getIndex({8, 6, 1}));
		Item& saw = simulation.m_hasItems->createItemNongeneric(ItemType::byName("saw"), bronze, 25u, 0);
		saw.setLocation(blocks.getIndex({3, 7, 1}));
		Item& mallet = simulation.m_hasItems->createItemNongeneric(ItemType::byName("mallet"), bronze, 25u, 0);
		mallet.setLocation(blocks.getIndex({4, 9, 1}));
		Item& chisel = simulation.m_hasItems->createItemNongeneric(ItemType::byName("chisel"), bronze, 25u, 0);
		chisel.setLocation(blocks.getIndex({4, 9, 1}));
		REQUIRE(!craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
		area.m_hasCraftingLocationsAndJobs.at(faction).addJob(woodBucket, &wood, 1);
		// There is wood working to be done.
		REQUIRE(craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
		// There is not assembling to be done yet.
		REQUIRE(!craftObjectiveTypeAssembling.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(craftObjectiveTypeWoodWorking, 100);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(craftObjectiveTypeAssembling, 100);
		CraftJob* job = area.m_hasCraftingLocationsAndJobs.at(faction).getJobForAtLocation(dwarf1, woodWorking, sawingLocation, emptyJobSet);
		REQUIRE(job != nullptr);
		REQUIRE(job->getStep() == 1);
		// Find a project to join.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "craft");
		REQUIRE(job->craftStepProject.get()->hasCandidate(dwarf1));
		// Reserve required.
		simulation.doStep();
		REQUIRE(job->craftStepProject->reservationsComplete());
		REQUIRE(job->craftStepProject->getWorkers().contains(&dwarf1));
		ProjectWorker& projectWorker = job->craftStepProject->getProjectWorkerFor(dwarf1);
		// Select a haul subproject.
		simulation.doStep();
		REQUIRE(projectWorker.haulSubproject != nullptr);
		// Find a path.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != BLOCK_INDEX_MAX);
		REQUIRE(board.isAdjacentTo(dwarf1.m_canMove.getDestination()));
		SUBCASE("success")
		{
			simulation.fastForwardUntillActorIsAdjacentToHasShape(dwarf1, board);
			std::function<bool()> predicate = [&](){ return blocks.item_getCount(sawingLocation, ItemType::byName("bucket"), wood) == 1; };
			simulation.fastForwardUntillPredicate(predicate, 11);
			REQUIRE(dwarf1.m_location == sawingLocation);
			REQUIRE(job->getStep() == 2);
			REQUIRE(job->workPiece != nullptr);
			Item& bucket = *job->workPiece;
			REQUIRE(bucket.m_location == sawingLocation);
			REQUIRE(area.getTotalCountOfItemTypeOnSurface(ItemType::byName("board")) == 9);
			// There is more wood working to be done.
			REQUIRE(craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
			// There is still no assembling to be done.
			REQUIRE(!craftObjectiveTypeAssembling.canBeAssigned(dwarf1));
			predicate = [&]() { return bucket.m_location == chiselLocation && job->getStep() == 3; };
			simulation.fastForwardUntillPredicate(predicate, 21);
			// There is no wood working to be done.
			REQUIRE(!craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
			// There is assembling to be done.
			REQUIRE(craftObjectiveTypeAssembling.canBeAssigned(dwarf1));
			// Find a project to join.
			simulation.doStep();
			REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "craft");
			REQUIRE(job->craftStepProject.get()->hasCandidate(dwarf1));
			// Reserve required.
			simulation.doStep();
			REQUIRE(job->craftStepProject->reservationsComplete());
			REQUIRE(job->craftStepProject->getWorkers().contains(&dwarf1));
			ProjectWorker& projectWorker2 = job->craftStepProject->getProjectWorkerFor(dwarf1);
			// Select a haul subproject.
			simulation.doStep();
			REQUIRE(projectWorker2.haulSubproject != nullptr);
			// Find a path.
			simulation.doStep();
			REQUIRE(dwarf1.m_canMove.getDestination() != BLOCK_INDEX_MAX);
			REQUIRE(rope.isAdjacentTo(dwarf1.m_canMove.getDestination()));
			simulation.fastForwardUntillActorIsAdjacentToHasShape(dwarf1, rope);
			predicate = [&]() { return bucket.m_location == chiselLocation && bucket.m_craftJobForWorkPiece == nullptr; };
			simulation.fastForwardUntillPredicate(predicate, 11);
			// There is no longer wood working to be done.
			REQUIRE(!craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
		}
		SUBCASE("project canceled")
		{
			SUBCASE("by player undesignating location")
			{
				area.m_hasCraftingLocationsAndJobs.at(faction).removeLocation(sawCategory, sawingLocation);
				// There is no longer wood working to be done because there are no locations to do it at.
				REQUIRE(!craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
			}
			SUBCASE("by setting the location solid")
			{
				blocks.solid_set(sawingLocation, wood, false);
				// There is no longer wood working to be done because there are no locations to do it at.
				REQUIRE(!craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
			}
			// Project is canceled and craft objective is reset.
			REQUIRE(dwarf1.m_project == nullptr);
			REQUIRE(static_cast<CraftObjective&>(dwarf1.m_hasObjectives.getCurrent()).getCraftJob() == nullptr);
			REQUIRE(static_cast<CraftObjective&>(dwarf1.m_hasObjectives.getCurrent()).hasThreadedTask());
			// No viable craft job found, objective cannot be completed.
			simulation.doStep();
			REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "craft");
		}
		SUBCASE("by destroying a reserved item")
		{
			board.destroy();
			// There is still wood working to be done even though there are no boards to do it with.
			REQUIRE(craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
			REQUIRE(dwarf1.m_project == nullptr);
			REQUIRE(static_cast<CraftObjective&>(dwarf1.m_hasObjectives.getCurrent()).getCraftJob() == nullptr);
			REQUIRE(static_cast<CraftObjective&>(dwarf1.m_hasObjectives.getCurrent()).hasThreadedTask());
			REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "craft");
			REQUIRE(static_cast<CraftObjective&>(dwarf1.m_hasObjectives.getCurrent()).getFailedJobs().size() == 1);
			// Dwarf cannot create a new project as the CraftJob is now it CraftObjective.m_failedJobs.
			// TODO: Ideally this situation would result in immideate retry rather then fail, but this is ok for now.
			simulation.doStep();
			REQUIRE(dwarf1.m_project == nullptr);
			REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "craft");
		}
		SUBCASE("location inaccessable")
		{
			areaBuilderUtil::setSolidWall(area, blocks.getIndex({0, 3, 1}), blocks.getIndex({9, 3, 1}), wood);
			simulation.fastForwardUntillActorHasNoDestination(dwarf1);
			REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "craft");
			REQUIRE(dwarf1.m_project != nullptr);
			REQUIRE(board.getQuantity() == 10);
			REQUIRE(dwarf1.m_canMove.hasThreadedTask());
			simulation.doStep();
			REQUIRE(job->craftStepProject == nullptr);
			REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "craft");
			REQUIRE(dwarf1.m_project == nullptr);
		}
		SUBCASE("worker dies")
		{
			dwarf1.die(CauseOfDeath::thirst);
			Actor& dwarf2 = simulation.m_hasActors->createActor(ActorParamaters{
				.species=AnimalSpecies::byName("dwarf"), 
				.location=blocks.getIndex({1, 5, 1}),
				.area=&area,
			});
			dwarf2.setFaction(&faction);
			REQUIRE(craftObjectiveTypeWoodWorking.canBeAssigned(dwarf2));
		}
	}
}
