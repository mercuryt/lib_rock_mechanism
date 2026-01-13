#pragma once
#include "dataStructures/smallMap.h"
#include "reference.h"

class Area;
class DeserializationMemo;
struct SquadFormation
{
	// Offset is relative to the center of the squad.
	// No Facing4 is stored: All squad members have the same facing.
	SmallMap<ActorId, Offset3D> m_locations;
	std::string m_name;
	ShapeId m_shapeId;
	[[nodiscard]] const std::string& getName() const;
	[[nodiscard]] const ShapeId& getShapeId() const;
	[[nodiscard]] const Offset3D getOffsetFor(const ActorId& reference) const;
	void setName(const std::string& name);
	// 'setOffset' and 'remove' both call buildShape.
	void setOffset(const ActorId& actor, const Offset3D& offset);
	void remove(const ActorId& actor);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SquadFormation, m_locations, m_name, m_shapeId);
};
class Squad
{
	SmallSet<ActorId> m_actors;
	// Each squad has it's own formations specifing individual members.
	// TODO: create a way to copy formations between squads, turing the original into a generic form which selects positions via weapons equiped, falling back on substituting weapons with the same range.
	StrongVector<SquadFormation, SquadFormationIndex> m_formations;
	SmallMap<ActorIndex, Offset3D> m_currentFormationLocationsAsIndices;
	std::string m_name;
	// When a formation is used the commander's position is the center around which it offsets.
	// The commander is used for pathing and his m_compoundShape is updated with the formation's shape.
	ActorId m_commander;
	// A pointer into the m_formations set.
	// When m_currentFormation is empty or an actor doesn't have an offset in the set formation set their destination to be the commander's destination.
	SquadFormationIndex m_currentFormationIndex;
public:
	void setCommander(const ActorId& commander);
	void addActor(const ActorId& actor);
	// Remove from all formations.
	void removeActor(const ActorId& actor);
	void setOffsetOfActorInCurrentFormation(const ActorId& actor, const Offset3D& offset);
	void setName(const std::string& name);
	void createFormationWithCurrentPositions(const std::string& name, Area& area);
	void updateActorIndex(const ActorIndex& oldIndex, const ActorIndex& newIndex);
	void setCurrentArea(const Area& area);
	[[nodiscard]] const std::string& getName() const;
	[[nodiscard]] ActorId& getCommander();
	[[nodiscard]] const ActorId& getCommander() const;
	[[nodiscard]] const SmallSet<ActorId>& getAll() const;
	[[nodiscard]] SquadFormation* getFormation();
	[[nodiscard]] const SquadFormation* getFormation() const;
	// Used in deserialization.
	[[nodiscard]] SquadFormation& getFormationByName(std::string& name);
	[[nodiscard]] const StrongVector<SquadFormation, SquadFormationIndex>& getFormations() const;
	// Creates a squad with actor as commander and sole member.
	// m_formations is left empty and current formation is set to null.
	// m_name is set to the name of the actor with "'s squad" appended to the end.
	[[nodiscard]] static Squad create(const ActorId& commanderId, const std::string& name);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Squad, m_actors, m_formations, m_currentFormationLocationsAsIndices, m_name, m_commander, m_currentFormationIndex);
};