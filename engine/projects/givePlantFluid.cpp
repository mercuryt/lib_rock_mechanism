#include "givePlantFluid.h"
#include "../plants.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../actors/actors.h"
#include "../definitions/plantSpecies.h"

GivePlantFluidProject::GivePlantFluidProject(const Point3D& location, Area& area, const FactionId& faction) :
	Project(faction, area, location, Quantity::create(1)),
	m_plantLocation(location)
{
	setFluidTypeAndVolume();
	m_area.m_hasFarmFields.getForFaction(m_faction).removeGivePlantFluidDesignation(area, m_plantLocation);
}
GivePlantFluidProject::GivePlantFluidProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	Project(data, deserializationMemo, area),
	m_plantLocation(data["plantLocation"].get<Point3D>()),
	m_volume(data["volume"].get<CollisionVolume>()),
	m_fluidType(data["fluidType"].get<FluidTypeId>())
{ }
void GivePlantFluidProject::setFluidTypeAndVolume()
{
	const Plants& plants = m_area.getPlants();
	const Space& space = m_area.getSpace();
	const PlantIndex& plant = space.plant_get(m_location);
	assert(plant.exists());
	const PlantSpeciesId& species = plants.getSpecies(plant);
	m_fluidType = PlantSpecies::getFluidType(species);
	m_volume = plants.getVolumeFluidRequested(plant);
}
void GivePlantFluidProject::onComplete()
{
	Plants& plants = m_area.getPlants();
	Actors& actors = m_area.getActors();
	const Space& space = m_area.getSpace();
	const PlantIndex& plant = space.plant_get(m_location);
	if(plant.exists())
		plants.addFluid(plant, m_volume, m_fluidType);
	for(auto& [actor, projectWorker] : m_workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
void GivePlantFluidProject::onCancel()
{
	Space& space = m_area.getSpace();
	Plants& plants = m_area.getPlants();
	PlantIndex plant = space.plant_get(m_plantLocation);
	if(
		plant.exists() &&
		m_area.getPlants().getVolumeFluidRequested(plant) != 0 &&
		space.farm_contains(m_plantLocation, m_faction) &&
		space.farm_get(m_plantLocation, m_faction)->m_plantSpecies == plants.getSpecies(plant)
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