#include "givePlantFluid.h"
#include "../plants.h"
#include "../area/area.h"
#include "../blocks/blocks.h"
#include "../actors/actors.h"

GivePlantFluidProject::GivePlantFluidProject(const BlockIndex& location, Area& area, const FactionId& faction) :
	Project(faction, area, location, Quantity::create(1)),
	m_plantLocation(location)
{
	setFluidTypeAndVolume();
	m_area.m_hasFarmFields.getForFaction(m_faction).removeGivePlantFluidDesignation(area, m_plantLocation);
}
GivePlantFluidProject::GivePlantFluidProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	Project(data, deserializationMemo, area),
	m_plantLocation(data["plantLocation"].get<BlockIndex>()),
	m_volume(data["volume"].get<CollisionVolume>()),
	m_fluidType(data["fluidType"].get<FluidTypeId>())
{ }
void GivePlantFluidProject::setFluidTypeAndVolume()
{
	const Plants& plants = m_area.getPlants();
	const Blocks& blocks = m_area.getBlocks();
	const PlantIndex& plant = blocks.plant_get(m_location);
	assert(plant.exists());
	const PlantSpeciesId& species = plants.getSpecies(plant);
	m_fluidType = PlantSpecies::getFluidType(species);
	m_volume = plants.getVolumeFluidRequested(plant);
}
void GivePlantFluidProject::onComplete()
{
	Plants& plants = m_area.getPlants();
	Actors& actors = m_area.getActors();
	const Blocks& blocks = m_area.getBlocks();
	const PlantIndex& plant = blocks.plant_get(m_location);
	if(plant.exists())
		plants.addFluid(plant, m_volume, m_fluidType);
	for(auto& [actor, projectWorker] : m_workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
void GivePlantFluidProject::onCancel()
{
	Blocks& blocks = m_area.getBlocks();
	Plants& plants = m_area.getPlants();
	PlantIndex plant = blocks.plant_get(m_plantLocation);
	if(
		plant.exists() &&
		m_area.getPlants().getVolumeFluidRequested(plant) != 0 &&
		blocks.farm_contains(m_plantLocation, m_faction) &&
		blocks.farm_get(m_plantLocation, m_faction)->plantSpecies == plants.getSpecies(plant)
	)
		m_area.m_hasFarmFields.getForFaction(m_faction).addGivePlantFluidDesignation(m_area, m_plantLocation);
}
Json GivePlantFluidProject::toJson() const
{
	Json output = Project::toJson();
	output.update({
		{"plantLocation", m_plantLocation},
		{"fluidType", m_fluidType},
		{"volume", m_volume}
	});
	return output;
}