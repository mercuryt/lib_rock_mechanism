#pragma once

#include "eventSchedule.h"
#include "eventSchedule.hpp"
#include "findsPath.h"
#include "objective.h"
#include "project.h"
#include "threadedTask.h"

class Actor;
struct Wound;
class MedicalProject;
class MedicalObjective;
class MedicalPatientRelistEvent;

class MedicalThreadedTask final : public ThreadedTask
{
	MedicalObjective& m_objective;
	FindsPath m_findsPath;
public:
	MedicalThreadedTask(MedicalObjective& o);
	void readStep();
	void writeStep();
	void clearReferences();
};
class MedicalObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Medical; }
};
class MedicalObjective final : public Objective
{
	Actor& m_actor;
	MedicalProject* m_project;
	HasThreadedTask<MedicalThreadedTask> m_threadedTask;
public:
	MedicalObjective(Actor& a) : Objective(Config::medicalPriority), m_actor(a), m_threadedTask(a.getThreadedTaskEngine()) { } 
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset() { cancel(); }
	void setLocation(BlockIndex& block);
	bool isNeed() const { return false; }
	std::string name() const { return "medical"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Medical; }
	bool blockContainsPatientForThisWorker(const BlockIndex& block) const;
	MedicalProject* getProjectForActorAtLocation(BlockIndex& block);
	friend class MedicalThreadedTask;
};
struct MedicalProjectType final
{
	std::string name;
	uint32_t baseStepsDuration;
	std::vector<std::pair<ItemQuery, uint32_t>> consumedItems;
	std::vector<std::pair<ItemQuery, uint32_t>> unconsumedItems;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> byproductItems;
	// Infastructure.
	bool operator==(const MedicalProjectType& medicalProjectType) const { return this == &medicalProjectType; }
	inline static std::vector<MedicalProjectType> data;
	static const MedicalProjectType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &MedicalProjectType::name);
		assert(found != data.end());
		return *found;
	}
};
class MedicalProject final : public Project
{
	Actor& m_patient;
	Actor& m_doctor;
	Wound& m_wound;
	MedicalProjectType& m_medicalProjectType;
public:
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	void onComplete();
	void onCancel();
	// TODO: geometric progresson of disable duration.
	void onDelay();
	void offDelay() { assert(false); }
	Step getDuration() const;
	uint32_t getItemScaleFactor() const;
	MedicalProject(Actor& p, BlockIndex& location, Actor& doctor, Wound& wound, MedicalProjectType& medicalProjectType) : Project(doctor.getFaction(), location, 0), m_patient(p), m_doctor(doctor), m_wound(wound), m_medicalProjectType(medicalProjectType) { }
	friend class AreaHasMedicalPatientsForFaction;
};
struct SortPatientsByPriority
{
	bool operator()(Actor* const& a, Actor* const& b) { return a->m_body.getStepsTillBleedToDeath() < b->m_body.getStepsTillBleedToDeath(); }
};
struct SortDoctorsBySkill
{
	bool operator()(Actor* const& a, Actor* const& b) 
	{ 
		static const SkillType& medicalSkill = SkillType::byName("medical");
		return a->m_skillSet.get(medicalSkill) < b->m_skillSet.get(medicalSkill); 
	}
};
class AreaHasMedicalPatientsForFaction final
{
	Faction& m_faction;
	std::set<Actor*, SortPatientsByPriority> m_waitingPatients;
	std::set<Actor*, SortDoctorsBySkill> m_waitingDoctors;
	std::unordered_set<BlockIndex*> m_medicalLocations;
	std::unordered_map<Actor*, MedicalProject> m_medicalProjects;
	std::unordered_map<Actor*, HasScheduledEvent<MedicalPatientRelistEvent>> m_relistEvents;
public:
	AreaHasMedicalPatientsForFaction(Faction& f) : m_faction(f) { }
	void addPatient(Actor& patient);
	void removePatient(Actor& patient);
	void removePatientTemporarily(Actor& patient);
	void addDoctor(Actor& doctor);
	void removeDoctor(Actor& doctor);
	void addLocation(BlockIndex& block);
	void removeLocation(BlockIndex& block);
	void createProject(Actor& patient, BlockIndex& location, Actor& doctor, Wound& wound, const MedicalProjectType& medicalProjectType);
	void cancelProject(MedicalProject& project);
	void destroyProject(MedicalProject& project);
	void triage();
	MedicalProject* getProjectForPatient(const Actor& actor);
	const MedicalProjectType& getMedicalProjectTypeForWound(const Wound& wound) const;
	bool hasPatients() const;
	friend class MedicalPatientRelistEvent;
};
class MedicalPatientRelistEvent final : public ScheduledEvent
{
	Actor& m_patient;
public:
	MedicalPatientRelistEvent(Actor& p, const Step start = 0) : ScheduledEvent(p.getSimulation(), Config::medicalProjectDelaySteps, start), m_patient(p) { }
	void execute();
	void clearReferences();
};
class AreaHasMedicalPatients final
{
	std::unordered_map<Faction*, AreaHasMedicalPatientsForFaction> m_data;
public:
	void addFaction(Faction& faction) { assert(!m_data.contains(&faction)); m_data.emplace(&faction, faction); }
	void removeFaction(Faction& faction) { assert(m_data.contains(&faction)); m_data.erase(&faction); }
	AreaHasMedicalPatientsForFaction& at(Faction& faction) { assert(m_data.contains(&faction)); return m_data.at(&faction); }
};
