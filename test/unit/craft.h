#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/item.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
#include "../../src/craft.h"
#include "materialType.h"
#include "project.h"
#include "skill.h"
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
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));
	Block& chiselLocation = area.m_blocks[3][5][1];
	Block& sawingLocation = area.m_blocks[9][9][1];
	REQUIRE(!area.m_hasCraftingLocationsAndJobs.hasLocationsFor(woodBucket));
	area.m_hasCraftingLocationsAndJobs.addLocation(CraftStepTypeCategory::byName("carve"), chiselLocation);
	area.m_hasCraftingLocationsAndJobs.addLocation(CraftStepTypeCategory::byName("assemble"), chiselLocation);
	area.m_hasCraftingLocationsAndJobs.addLocation(sawCategory, sawingLocation);
	REQUIRE(area.m_hasCraftingLocationsAndJobs.hasLocationsFor(woodBucket));
	Faction faction(L"tower of power");
	CraftObjectiveType craftObjectiveTypeWoodWorking(woodWorking);
	CraftObjectiveType craftObjectiveTypeAssembling(assembling);
	Actor& dwarf1 = simulation.createActor(AnimalSpecies::byName("dwarf"), area.m_blocks[1][1][1]);
	dwarf1.setFaction(&faction);
	area.m_hasActors.add(dwarf1);
	SUBCASE("infastructure")
	{
		area.m_hasCraftingLocationsAndJobs.addJob(woodBucket, &wood);
		auto pair = area.m_hasCraftingLocationsAndJobs.getJobAndLocationFor(dwarf1, woodWorking);
		CraftJob* job = pair.first;
		Block* location = pair.second;
		REQUIRE(job != nullptr);
		REQUIRE(location != nullptr);
		REQUIRE(location == &sawingLocation);
		REQUIRE(&job->craftJobType == &woodBucket);
		REQUIRE(job->workPiece == nullptr);
		REQUIRE(job->minimumSkillLevel == 0);
		REQUIRE(job->totalSkillPoints == 0);
		REQUIRE(!job->craftJobType.stepTypes.empty());
		REQUIRE(job == area.m_hasCraftingLocationsAndJobs.getJobForAtLocation(dwarf1, woodWorking, sawingLocation));
	}
	SUBCASE("craft bucket")
	{
		Block& boardLocation = area.m_blocks[6][6][1];
		Item& board = simulation.createItem(ItemType::byName("board"), wood, 10u);
		board.setLocation(boardLocation);
		Item& rope = simulation.createItem(ItemType::byName("rope"), MaterialType::byName("plant matter"), 10u);
		rope.setLocation(area.m_blocks[8][6][1]);
		Item& saw = simulation.createItem(ItemType::byName("saw"), bronze, 25u, 0);
		saw.setLocation(area.m_blocks[3][7][1]);
		Item& mallet = simulation.createItem(ItemType::byName("mallet"), bronze, 25u, 0);
		mallet.setLocation(area.m_blocks[4][9][1]);
		Item& chisel = simulation.createItem(ItemType::byName("chisel"), bronze, 25u, 0);
		chisel.setLocation(area.m_blocks[4][9][1]);
		REQUIRE(!craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
		area.m_hasCraftingLocationsAndJobs.addJob(woodBucket, &wood);
		// There is wood working to be done.
		REQUIRE(craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
		// There is not assembling to be done yet.
		REQUIRE(!craftObjectiveTypeAssembling.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(craftObjectiveTypeWoodWorking, 100);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(craftObjectiveTypeAssembling, 100);
		CraftJob* job = area.m_hasCraftingLocationsAndJobs.getJobForAtLocation(dwarf1, woodWorking, sawingLocation);
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
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		REQUIRE(board.isAdjacentTo(*dwarf1.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToHasShape(dwarf1, board);
		std::function<bool()> predicate = [&](){ return sawingLocation.m_hasItems.getCount(ItemType::byName("bucket"), wood) == 1; };
		simulation.fastForwardUntillPredicate(predicate, 11);
		REQUIRE(job->getStep() == 2);
		REQUIRE(job->workPiece != nullptr);
		Item& bucket = *job->workPiece;
		REQUIRE(bucket.m_location == &sawingLocation);
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(ItemType::byName("board")) == 9);
		// There is more wood working to be done.
		REQUIRE(craftObjectiveTypeWoodWorking.canBeAssigned(dwarf1));
		// There is still no assembling to be done.
		REQUIRE(!craftObjectiveTypeAssembling.canBeAssigned(dwarf1));
		predicate = [&]() { return bucket.m_location == &chiselLocation && job->getStep() == 3; };
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
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		REQUIRE(rope.isAdjacentTo(*dwarf1.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToHasShape(dwarf1, rope);
		predicate = [&]() { return bucket.m_location == &chiselLocation && bucket.m_craftJobForWorkPiece == nullptr; };
		simulation.fastForwardUntillPredicate(predicate, 11);
	}
}
