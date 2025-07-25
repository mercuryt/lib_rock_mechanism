#pragma once
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../numericTypes/types.h"
#include "../dataStructures/smallSet.h"
#include "deserializationMemo.h"
struct CraftJob;
struct SkillType;
class CraftObjectiveType final : public ObjectiveType
{
	SkillTypeId m_skillType; // Multiple skills are represented by CraftObjective and CraftObjectiveType, such as Leatherworking, Woodworking, Metalworking, etc.
public:
	CraftObjectiveType(SkillTypeId skillType) : m_skillType(skillType) { }
	CraftObjectiveType(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::string name() const override;
};
class CraftObjective final : public Objective
{
	SkillTypeId m_skillType;
	CraftJob* m_craftJob = nullptr;
	SmallSet<CraftJob*> m_failedJobs;
public:
	CraftObjective(SkillTypeId st);
	CraftObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor)override;
	void cancel(Area& area, const ActorIndex& actor)override;
	void delay(Area& area, const ActorIndex& actor) override{ cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor) override;
	void recordFailedJob(CraftJob& craftJob) { assert(!m_failedJobs.contains(&craftJob)); m_failedJobs.insert(&craftJob); }
	[[nodiscard]] Json toJson() const override;
	[[nodiscard]] SmallSet<CraftJob*>& getFailedJobs() { return m_failedJobs; }
	[[nodiscard]] std::string name() const override;
	[[nodiscard]] bool canBeAddedToPrioritySet() { return true; }
	friend class CraftPathRequest;
	friend class HasCraftingLocationsAndJobsForFaction;
	// For testing.
	[[maybe_unused, nodiscard]] CraftJob* getCraftJob() { return m_craftJob; }

};
class CraftPathRequest final : public PathRequestBreadthFirst
{
	CraftObjective& m_craftObjective;
	CraftJob* m_craftJob = nullptr;
	Point3D m_location;
public:
	CraftPathRequest(Area& area, CraftObjective& co, const ActorIndex& actor);
	CraftPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() { return m_craftObjective.name(); }
};
