#pragma once
#include "../reference.h"
#include "psycologyData.h"

// These are virtual base classes to be extended by various specific relationship types.
// Multiple relationships may exist between pairs of participants, but they must be unique relationship types.
enum class RelationshipBetweenActorsType
{
	// Family relationships.
	Child,
	Mother,
	Father,
	Sibling,
	GrandParent,
	ChildOfSibling,
	SiblingOfParent,
	Cousine,
	ChildOfCousin,
	ExtendedFamily, // For more complex relationships like second cousine once removed.
	Friend,
	Lover,
	Spouse,
	SquadMate,
	SquadCommander,
	Coworker,
	Supervisor,
	Mayor,
	Null,
};
struct RelationshipBetweenActors final
{
	ActorId actor;
	PsycologyWeight positivity;
	PsycologyWeight respect;
	RelationshipBetweenActorsType type;
	bool operator==(const RelationshipBetweenActors& other) const { return other.actor == actor && other.type == type; };
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RelationshipBetweenActors, actor, positivity, respect, type)
};
enum class RelationshipWithItemType
{
	Creator,
	Owner,
	Gift,
};
struct RelationshipWithItem final
{
	ItemId item;
	PsycologyWeight positivity;
	PsycologyWeight respect;
	RelationshipWithItemType type;
	bool operator==(const RelationshipWithItem& other) const { return other.item == item && other.type == type; };
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RelationshipWithItem, item, positivity, respect, type);
};
enum class RelationshipWithPlantType
{
	Sower,
	Caretaker,
};
struct RelationshipWithPlant final
{
	Point3D plantLocation;
	PlantSpeciesId species;
	AreaId area;
	PsycologyWeight positivity;
	PsycologyWeight respect;
	RelationshipWithPlantType type;
	bool operator==(const RelationshipWithPlant& other) const { return other.plantLocation == plantLocation && other.species == species && other.type == type; };
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RelationshipWithPlant, plantLocation, species, positivity, respect, type);
};
class ActorHasRelationships
{
	std::vector<RelationshipBetweenActors> m_actors;
	std::vector<RelationshipWithItem> m_items;
	std::vector<RelationshipWithPlant> m_plants;
public:
	RelationshipBetweenActors& create(const ActorId& actor);
	RelationshipWithItem& create(const ItemId& item);
	RelationshipWithPlant& create(const Point3D& location, const PlantSpeciesId& species, const AreaId& area);
	RelationshipBetweenActors& getOrCreate(const ActorId& actor);
	RelationshipWithItem&  getOrCreate(const ItemId& item);
	RelationshipWithPlant& getOrCreate(const Point3D& location, const PlantSpeciesId& species, const AreaId& area);
	void destroy(const RelationshipBetweenActors& relationship);
	void destroy(const RelationshipWithItem& relationship);
	void destroy(const RelationshipWithPlant& relationship);
	void addToRelationship(const ActorId& actor, const PsycologyAttribute& attribute, const PsycologyWeight& weight);
	void addToRelationship(const ItemId& actor, const PsycologyAttribute& attribute, const PsycologyWeight& weight);
	void addToRelationship(const PlantIndex& plant, Area& area, const PsycologyAttribute& attribute, const PsycologyWeight& weight);
	[[nodiscard]] RelationshipBetweenActors* maybeGetForActor(const ActorId& actor);
	[[nodiscard]] RelationshipWithItem* maybeGetForItem(const ItemId& item);
	[[nodiscard]] RelationshipWithPlant* maybeGetForPlant(const Point3D& location, const PlantSpeciesId& species, const AreaId& area);
	[[nodiscard]] const RelationshipBetweenActors* maybeGetForActor(const ActorId& actor) const;
	[[nodiscard]] const RelationshipWithItem* maybeGetForItem(const ItemId& item) const;
	[[nodiscard]] const RelationshipWithPlant* maybeGetForPlant(const Point3D& location, const PlantSpeciesId& species, const AreaId& area) const;
	[[nodiscard]] RelationshipBetweenActors* maybeGetForActorWithCondition(auto&& condition)
	{
		const auto found = std::ranges::find_if(m_actors, condition);
		if(found == m_actors.end())
			return nullptr;
		// Return address that iterator refers to.
		return &(*found);
	}
	[[nodiscard]] RelationshipWithItem* maybeGetForItemWithCondition(auto&& condition)
	{
		const auto found = std::ranges::find_if(m_items, condition);
		if(found == m_items.end())
			return nullptr;
		// Return address that iterator refers to.
		return &(*found);
	}
	[[nodiscard]] RelationshipWithPlant* maybeGetForPlantWithCondition(auto&& condition)
	{
		const auto found = std::ranges::find_if(m_plants, condition);
		if(found == m_plants.end())
			return nullptr;
		// Return address that iterator refers to.
		return &(*found);
	}
	[[nodiscard]] const RelationshipBetweenActors* maybeGetForActorWithCondition(auto&& condition) const { return const_cast<ActorHasRelationships*>(this)->maybeGetForActorWithCondition(condition); }
	[[nodiscard]] const RelationshipWithItem* maybeGetForItemWithCondition(auto&& condition) const { return const_cast<ActorHasRelationships*>(this)->maybeGetForItemWithCondition(condition); }
	[[nodiscard]] const RelationshipWithPlant* maybeGetForPlantWithCondition(auto&& condition) const { return const_cast<ActorHasRelationships*>(this)->maybeGetForPlantWithCondition(condition); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ActorHasRelationships, m_actors, m_items, m_plants);
};