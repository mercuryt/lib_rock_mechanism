#include "medical.h"
#include "area.h"
#include "config.h"
#include "item.h"
#include "actor.h"
#include "block.h"
#include "project.h"
#include <memory>
#include <vector>
#include <utility>
#include <functional>

// ThreadedTask
MedicalThreadedTask::MedicalThreadedTask(MedicalObjective& objective) : ThreadedTask(objective.m_actor.m_location->m_area->m_simulation.m_threadedTaskEngine), m_objective(objective), m_findsPath(objective.m_actor, objective.m_detour) { }
void MedicalThreadedTask::readStep()
{
	assert(!m_objective.m_project);
	std::function<bool(const Block&)> predicate = [&](const Block& block){ return m_objective.blockContainsPatientForThisWorker(block); };
	m_findsPath.pathToAdjacentToPredicate(predicate);
}
void MedicalThreadedTask::writeStep()
{
	assert(!m_objective.m_project);
	if(m_findsPath.found() || m_findsPath.m_useCurrentLocation)
	{
		Block& location = *m_findsPath.getBlockWhichPassedPredicate();
		if(!m_objective.blockContainsPatientForThisWorker(location))
			// Selected location is no longer valid.
			m_objective.m_threadedTask.create(m_objective);
		else
			m_objective.setLocation(location);
	}
	else 
		m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
}
void MedicalThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
// ObjectiveType
bool MedicalObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasMedicalPatients.at(*actor.getFaction()).hasPatients();
}
std::unique_ptr<Objective> MedicalObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<MedicalObjective>(actor);
}
// Objective
void MedicalObjective::execute()
{
	if(m_project == nullptr)
		m_threadedTask.create(*this);
	else
		m_project->commandWorker(m_actor);
}
void MedicalObjective::cancel()
{
	if(m_project != nullptr)
	{
		m_project->removeWorker(m_actor);
		m_project = nullptr;
		m_actor.m_project = nullptr;
	}
	else
		assert(m_actor.m_project == nullptr);
	m_threadedTask.maybeCancel();
	m_actor.m_canReserve.clearAll();
}
bool MedicalObjective::blockContainsPatientForThisWorker(const Block& block) const
{
	return const_cast<MedicalObjective*>(this)->getProjectForActorAtLocation(const_cast<Block&>(block)) != nullptr;
}
void MedicalObjective::setLocation(Block& block)
{
	MedicalProject* project = getProjectForActorAtLocation(block);
	assert(project != nullptr);
	m_actor.m_project = project;
	m_project = project;
	project->addWorkerCandidate(m_actor, *this);
}
MedicalProject* MedicalObjective::getProjectForActorAtLocation(Block& block)
{
	AreaHasMedicalPatientsForFaction& areaHasPatientsForFaction = block.m_area->m_hasMedicalPatients.at(*m_actor.getFaction());
	for(const Actor* actor : block.m_hasActors.getAllConst())
	{
		MedicalProject* project = areaHasPatientsForFaction.getProjectForPatient(*actor);
		if(project == nullptr || !project->canAddWorker(m_actor))
			continue;
		return project;
	}
	return nullptr;
}
// Project
std::vector<std::pair<ItemQuery, uint32_t>> MedicalProject::getConsumed() const
{
	auto output = m_medicalProjectType.consumedItems;
	for(auto& pair : output)
		pair.second *= getItemScaleFactor();
	return output;
}
std::vector<std::pair<ItemQuery, uint32_t>> MedicalProject::getUnconsumed() const
{
	auto output = m_medicalProjectType.unconsumedItems;
	for(auto& pair : output)
		pair.second *= getItemScaleFactor();
	return output;
}
std::vector<std::pair<ActorQuery, uint32_t>> MedicalProject::getActors() const
{
	return {{m_patient, 1}};
}
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> MedicalProject::getByproducts() const { return {}; }
void MedicalProject::onComplete()
{
	uint32_t healRateModifier = 1;//TODO: Apply doctor skill and room cleanliness.
	m_patient.m_body.doctorWound(m_wound, healRateModifier);
}
void MedicalProject::onCancel()
{
	std::vector<Actor*> actors = getWorkersAndCandidates();
	m_location.m_area->m_hasMedicalPatients.at(m_faction).destroyProject(*this);
	for(Actor* actor : actors)
	{
		static_cast<StockPileObjective&>(actor->m_hasObjectives.getCurrent()).m_project = nullptr;
		actor->m_project = nullptr;
		actor->m_hasObjectives.getCurrent().reset();
		actor->m_hasObjectives.cannotCompleteTask();
	}
}
void MedicalProject::onDelay()
{
	auto& hasProjects = m_location.m_area->m_hasMedicalPatients.at(m_faction);
	hasProjects.removePatientTemporarily(m_patient);
}
Step MedicalProject::getDuration() const { return m_medicalProjectType.baseStepsDuration; }
uint32_t MedicalProject::getItemScaleFactor() const { return m_wound.hit.area * Config::unitsOfWoundAreaPerUnitItemScaleFactor; }
// Index.
void AreaHasMedicalPatientsForFaction::addPatient(Actor& patient)
{
	assert(!m_waitingPatients.contains(&patient));
	m_waitingPatients.insert(&patient);
	triage();
}
void AreaHasMedicalPatientsForFaction::removePatient(Actor& patient)
{
	assert(m_waitingPatients.contains(&patient));
	m_relistEvents.erase(&patient);
	m_waitingPatients.erase(&patient);
	if(m_medicalProjects.contains(&patient))
	{
		m_medicalProjects.at(&patient).cancel();
		m_medicalProjects.erase(&patient);
		triage();
	}
}
void AreaHasMedicalPatientsForFaction::removePatientTemporarily(Actor& patient)
{
	removePatient(patient);
	m_relistEvents.emplace(&patient, patient);
}
void AreaHasMedicalPatientsForFaction::addDoctor(Actor& doctor)
{
	assert(!m_waitingDoctors.contains(&doctor));
	m_waitingDoctors.insert(&doctor);
	triage();
}
void AreaHasMedicalPatientsForFaction::removeDoctor(Actor& doctor)
{
	assert(m_waitingDoctors.contains(&doctor));
	m_waitingDoctors.erase(&doctor);
	//TODO: Erase any medical projects the doctor is assigned to.
	triage();
}
void AreaHasMedicalPatientsForFaction::addLocation(Block& block)
{
	assert(!m_medicalLocations.contains(&block));
	m_medicalLocations.insert(&block);
	triage();
}
void AreaHasMedicalPatientsForFaction::removeLocation(Block& block)
{
	assert(m_medicalLocations.contains(&block));
	m_medicalLocations.erase(&block);
	//TODO: Cancel any medical projects the location is assigned to.
	triage();
}
void AreaHasMedicalPatientsForFaction::createProject(Actor& patient, Block& location, Actor& doctor, Wound& wound, const MedicalProjectType& medicalProjectType)
{
	assert(m_waitingPatients.contains(&patient));
	assert(m_waitingDoctors.contains(&doctor));
	assert(!location.m_reservable.isFullyReserved(doctor.getFaction()));
	m_medicalProjects.try_emplace(&patient, patient, location, doctor, wound, medicalProjectType);
	m_waitingPatients.erase(&patient);
	m_waitingDoctors.erase(&doctor);
}
void AreaHasMedicalPatientsForFaction::cancelProject(MedicalProject& project)
{
	project.cancel();
	m_waitingDoctors.insert(&project.m_doctor);
	addPatient(project.m_patient);
	m_medicalProjects.erase(&project.m_patient);
}
void AreaHasMedicalPatientsForFaction::destroyProject(MedicalProject& project)
{
	m_medicalProjects.erase(&project.m_patient);
}
void AreaHasMedicalPatientsForFaction::triage()
{
	while(!m_waitingPatients.empty() && !m_waitingDoctors.empty())
	{
		Actor& patient = **m_waitingPatients.begin();
		Actor& doctor = **m_waitingDoctors.begin();
		// TODO: get location nearest to patient?
		auto* location = *m_medicalLocations.begin();
		Wound& wound = patient.m_body.getWoundWhichIsBleedingTheMost();
		const MedicalProjectType& medicalProjectType = getMedicalProjectTypeForWound(const_cast<const Wound&>(wound));
		createProject(patient, *location, doctor, wound, medicalProjectType);
	}
}
MedicalProject* AreaHasMedicalPatientsForFaction::getProjectForPatient(const Actor& actor)
{
	auto found = m_medicalProjects.find(const_cast<Actor*>(&actor));
	if(found == m_medicalProjects.end())
		return nullptr;
	return &found->second;
}
bool AreaHasMedicalPatientsForFaction::hasPatients() const { return !m_waitingPatients.empty(); }
const MedicalProjectType& AreaHasMedicalPatientsForFaction::getMedicalProjectTypeForWound([[maybe_unused]] const Wound& wound) const
{
	//TODO: Splint, stich, cauterize, amputate.
	return MedicalProjectType::byName("bandage");
}
void MedicalPatientRelistEvent::execute() { m_patient.m_location->m_area->m_hasMedicalPatients.at(*m_patient.getFaction()).addPatient(m_patient); }
void MedicalPatientRelistEvent::clearReferences() 
{ 
	auto& hasProjectsForFaction = m_patient.m_location->m_area->m_hasMedicalPatients.at(*m_patient.getFaction());
	hasProjectsForFaction.m_relistEvents.erase(&m_patient);
}
