#pragma once
#include "dataStructures/smallMap.h"
#include "reference.h"

using SquadIdWidth = uint32_t;
class SquadId : public StrongInteger<SquadId, SquadIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const Force& index) const { return index.get(); } };
};
inline void to_json(Json& data, const SquadId& index) { data = index.get(); }
inline void from_json(const Json& data, SquadId& index) { index = SquadId::create(data.get<SquadIdWidth>()); }

class SquadFormation
{
	// Offset is relative to the center of the squad.
	// No Facing4 is stored: All squad members have the same facing.
	SmallMap<ActorReference, Offset3D> m_locations;
	std::wstring m_name;
	ShapeId m_shapeId;
	void buildShape();
public:
	[[nodiscard]] const std::wstring& getName() const;
	[[nodiscard]] const ShapeId& getShapeId() const;
	[[nodiscard]] const Offset3D getOffsetFor(const ActorReference& reference) const;
	void setName(const std::wstring name);
	// 'setOffset' and 'remove' both call buildShape.
	void setOffset(const ActorReference& actor, const Offset3D& offset);
	void remove(const ActorReference& actor);
};
static SquadId nextSquadId = {0};
class Squad
{
	SmallSet<ActorReference> m_actors;
	// Each squad has it's own formations specifing individual members.
	// TODO: create a way to copy formations between squads, turing the original into a generic form which selects positions via weapons equiped, falling back on substituting weapons with the same range.
	SmallSet<SquadFormation> m_formations;
	std::wstring m_name;
	// A pointer into the m_formations set.
	// When m_currentFormation is null or an actor doesn't have an offset in the set formation set their destination to be the commander's destination.
	SquadFormation* m_currentFormation;
	// When a formation is used the commander's position is the center around which it offsets.
	// The commander is used for pathing and his m_compoundShape is updated with the formation's shape.
	ActorReference m_commander;
	SquadId m_id;
public:
	void setCommander(const ActorReference& commander);
	void addActor(const ActorReference& actor);
	// Remove from all formations.
	void removeActor(const ActorReference& actor);
	void setOffsetOfActorInCurrentFormation(const ActorReference& actor);
	void renameFormation(const SquadFormation& formation, std::wstring name);
	void setName(const std::wstring name);
	[[nodiscard]] const std::wstring& getName() const;
	[[nodiscard]] ActorReference& getCommander();
	[[nodiscard]] const ActorReference& getCommander() const;
	[[nodiscard]] const SmallSet<ActorReference>& getAll() const;
	[[nodiscard]] SquadFormation& getFormation();
	[[nodiscard]] const SquadFormation& getFormation() const;
	[[nodiscard]] const SmallSet<SquadFormation>& getFormations() const;
	// Creates a squad with actor as commander and sole member.
	// m_formations is left empty and current formation is set to null.
	// m_name is set to the name of the actor with "'s squad" appended to the end.
	// m_id is the value of nextSquadId, which is then incremented.
	[[nodiscard]] static Squad create(const ActorReference& actor);
};