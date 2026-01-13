#include "relationship.h"
#include "psycologyData.h"
#include "../actors/actors.h"
#include "../area/area.h"
RelationshipBetweenActors& ActorHasRelationships::create(const ActorId& actor)
{
	m_actors.emplace_back(actor);
	// No duplicates.
	assert(std::ranges::find_if(m_actors.begin(), m_actors.end() - 1, [&](const auto& other){ return other == m_actors.back(); }) == m_actors.end() - 1);
	return m_actors.back();
}
RelationshipWithItem& ActorHasRelationships::create(const ItemId& item)
{
	m_items.emplace_back(item);
	// No duplicates.
	assert(std::ranges::find_if(m_items.begin(), m_items.end() - 1, [&](const auto& other){ return other == m_items.back(); }) == m_items.end() - 1);
	return m_items.back();
}
RelationshipWithPlant& ActorHasRelationships::create(const Point3D& location, const PlantSpeciesId& species, const AreaId& area)
{
	m_plants.emplace_back(location, species, area);
	// No duplicates.
	assert(std::ranges::find_if(m_plants.begin(), m_plants.end() - 1, [&](const auto& other){ return other == m_plants.back(); }) == m_plants.end() - 1);
	return m_plants.back();
}
RelationshipBetweenActors& ActorHasRelationships::getOrCreate(const ActorId& actor)
{
	RelationshipBetweenActors* relationship = maybeGetForActor(actor);
	if(relationship == nullptr)
		return create(actor);
	return *relationship;
}
RelationshipWithItem& ActorHasRelationships::getOrCreate(const ItemId& item)
{

	RelationshipWithItem* relationship = maybeGetForItem(item);
	if(relationship == nullptr)
		return create(item);
	return *relationship;
}
RelationshipWithPlant& ActorHasRelationships::getOrCreate(const Point3D& location, const PlantSpeciesId& species, const AreaId& area)
{
	RelationshipWithPlant* relationship = maybeGetForPlant(location, species, area);
	if(relationship == nullptr)
		return create(location, species, area);
	return *relationship;
}
void ActorHasRelationships::destroy(const RelationshipBetweenActors& relationship) { m_actors.erase(std::ranges::find_if(m_actors, [&](const auto& other){ return other == relationship; })); }
void ActorHasRelationships::destroy(const RelationshipWithItem& relationship) { m_items.erase(std::ranges::find_if(m_items, [&](const auto& other){ return other == relationship; })); }
void ActorHasRelationships::destroy(const RelationshipWithPlant& relationship) { m_plants.erase(std::ranges::find_if(m_plants, [&](const auto& other){ return other == relationship; })); }
RelationshipBetweenActors* ActorHasRelationships::maybeGetForActor(const ActorId& actor)
{
	return maybeGetForActorWithCondition([&](const RelationshipBetweenActors& relationship) { return relationship.actor == actor; });
}
RelationshipWithItem* ActorHasRelationships::maybeGetForItem(const ItemId& item)
{
	return maybeGetForItemWithCondition([&](const RelationshipWithItem& relationship) { return relationship.item == item; });
}
RelationshipWithPlant* ActorHasRelationships::maybeGetForPlant(const Point3D& location, const PlantSpeciesId& species, const AreaId& area)
{
	return maybeGetForPlantWithCondition([&](const RelationshipWithPlant& relationship) { return relationship.plantLocation == location && relationship.area == area && relationship.species == species; });
}
const RelationshipBetweenActors* ActorHasRelationships::maybeGetForActor(const ActorId& actor) const
{
	return maybeGetForActorWithCondition([&](const RelationshipBetweenActors& relationship) { return relationship.actor == actor; });
}
const RelationshipWithItem* ActorHasRelationships::maybeGetForItem(const ItemId& item) const
{
	return maybeGetForItemWithCondition([&](const RelationshipWithItem& relationship) { return relationship.item == item; });
}
const RelationshipWithPlant* ActorHasRelationships::maybeGetForPlant(const Point3D& location, const PlantSpeciesId& species, const AreaId& area) const
{
	return maybeGetForPlantWithCondition([&](const RelationshipWithPlant& relationship) { return relationship.plantLocation == location && relationship.area == area && relationship.species == species; });
}
void ActorHasRelationships::addToRelationship(const ActorId& actor, const PsycologyAttribute& attribute, const PsycologyWeight& weight)
{
	RelationshipBetweenActors& relationship = getOrCreate(actor);
	switch(attribute)
	{
		case PsycologyAttribute::Positivity:
			relationship.positivity += weight;
			break;
		case PsycologyAttribute::Respect:
			relationship.respect += weight;
			break;
		default:
			std::unreachable();
	}
}