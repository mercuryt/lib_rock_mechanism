#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
struct CraftJob;
struct SkillType;
class CraftObjectiveType final : public ObjectiveType
{
	const SkillType& m_skillType; // Multiple skills are represented by CraftObjective and CraftObjectiveType, such as Leatherworking, Woodworking, Metalworking, etc.
public:
	CraftObjectiveType(const SkillType& skillType) : ObjectiveType(), m_skillType(skillType) { }
	CraftObjectiveType(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canBeAssigned(Area& area, ActorIndex actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Craft; }
};
class CraftObjective final : public Objective
{
	const SkillType& m_skillType;
	CraftJob* m_craftJob = nullptr;
	std::unordered_set<CraftJob*> m_failedJobs;
public:
	CraftObjective(ActorIndex a, const SkillType& st);
	CraftObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	void recordFailedJob(CraftJob& craftJob) { assert(!m_failedJobs.contains(&craftJob)); m_failedJobs.insert(&craftJob); }
	[[nodiscard]] std::unordered_set<CraftJob*>& getFailedJobs() { return m_failedJobs; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Craft; }
	[[nodiscard]] std::string name() const { return "craft"; }
	friend class CraftPathRequest;
	friend class HasCraftingLocationsAndJobsForFaction;
	// For testing.
	[[maybe_unused, nodiscard]] CraftJob* getCraftJob() { return m_craftJob; }

};
class CraftPathRequest final : public PathRequest
{
	CraftObjective& m_craftObjective;
	CraftJob* m_craftJob = nullptr;
	BlockIndex m_location = BLOCK_INDEX_MAX;
public:
	CraftPathRequest(Area& area, CraftObjective& co);
	void callback(Area& area, FindPathResult result);
};