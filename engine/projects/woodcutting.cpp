#include "../area/woodcutting.h"
#include "../definitions/itemType.h"
#include "../plants.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../actors/actors.h"
#include "../objectives/woodcutting.h"
#include "../definitions/plantSpecies.h"
WoodCuttingProject::WoodCuttingProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) : Project(data, deserializationMemo, area) { }
std::vector<std::pair<ItemQuery, Quantity>> WoodCuttingProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, Quantity>> WoodCuttingProject::getUnconsumed() const
{
	static const ItemTypeId itemType = ItemType::byName("axe");
	return {{ItemQuery::create(itemType), Quantity::create(1)}};
}
std::vector<ActorReference> WoodCuttingProject::getActors() const { return {}; }
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> WoodCuttingProject::getByproducts() const
{
	PlantIndex plant = m_area.getSpace().plant_get(m_location);
	Plants& plants = m_area.getPlants();
	Percent percentGrown = plants.getPercentGrown(plant);
	PlantSpeciesId species = plants.getSpecies(plant);
	int logs = PlantSpecies::getLogsGeneratedByFellingWhenFullGrown(species).get();
	int branches = PlantSpecies::getBranchesGeneratedByFellingWhenFullGrown(species).get();
	Quantity unitsLogsGenerated = Quantity::create(util::scaleByPercent(logs, percentGrown));
	Quantity unitsBranchesGenerated = Quantity::create(util::scaleByPercent(branches, percentGrown));
	assert(unitsLogsGenerated != 0);
	assert(unitsBranchesGenerated != 0);
	auto woodType = PlantSpecies::getWoodType(species);
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> output{

		{ItemType::byName("branch"), woodType, unitsBranchesGenerated},
		{ItemType::byName("log"), woodType, unitsLogsGenerated}
	};
	return output;
}
SkillTypeId WoodCuttingProject::getSkill() const { static auto output = SkillType::byName("wood cutting"); return output; }
// Static.
int WoodCuttingProject::getWorkerWoodCuttingScore(Area& area, const ActorIndex& actor)
{
	static SkillTypeId woodCuttingType = SkillType::byName("wood cutting");
	Actors& actors = area.getActors();
	int strength = actors.getStrength(actor).get();
	int skill = actors.skill_getLevel(actor, woodCuttingType).get();
	return (strength * Config::woodCuttingStrengthModifier) + (skill * Config::woodCuttingSkillModifier);
}
void WoodCuttingProject::onComplete()
{
	if(m_area.getSpace().plant_exists(m_location))
	{
		PlantIndex plant = m_area.getSpace().plant_get(m_location);
		m_area.getPlants().die(plant);
	}
	// Remove designations for other factions as well as owning faction.
	auto workers = std::move(m_workers);
	Actors& actors = m_area.getActors();
	m_area.m_hasWoodCuttingDesignations.clearAll(m_location);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
void WoodCuttingProject::onCancel()
{
	m_area.m_hasWoodCuttingDesignations.remove(m_faction, m_location);
	Actors& actors = m_area.getActors();
	for(ActorIndex actor : getWorkersAndCandidates())
	{
		// TODO: These two lines seem redundant with the third.
		actors.project_unset(actor);
		actors.objective_getCurrent<WoodCuttingObjective>(actor).reset(m_area, actor);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
void WoodCuttingProject::onDelay()
{
	m_area.getSpace().designation_maybeUnset(m_location, m_faction, SpaceDesignation::WoodCutting);
}
void WoodCuttingProject::offDelay()
{
	m_area.getSpace().designation_set(m_location, m_faction, SpaceDesignation::WoodCutting);
}
// What would the total delay time be if we started from scratch now with current workers?
Step WoodCuttingProject::getDuration() const
{

	int totalScore = 0;
	Actors& actors = m_area.getActors();
	for(auto& pair : m_workers)
		totalScore += getWorkerWoodCuttingScore(m_area, pair.first.getIndex(actors.m_referenceData));
	return std::max(Step::create(1u), Config::woodCuttingMaxSteps / totalScore);
}