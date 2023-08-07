#include "medical.h"
#include "config.h"
#include "item.h"
#include "actor.h"
#include "block.h"
#include <vector>
#include <utility>
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
uint32_t MedicalProject::getDelay() const { return m_medicalProjectType.baseStepsDuration; }
uint32_t MedicalProject::getItemScaleFactor() const { return m_wound.hit.area * Config::unitsOfWoundAreaPerUnitItemScaleFactor; }
void AreaHasMedicalPatientsForFaction::addPatient(Actor& patient)
{
	assert(!m_waitingPatients.contains(&patient));
	m_waitingPatients.insert(&patient);
	triage();
}
void AreaHasMedicalPatientsForFaction::removePatient(Actor& patient)
{
	assert(m_waitingPatients.contains(&patient));
	m_waitingPatients.erase(&patient);
	if(m_medicalProjects.contains(&patient))
	{
		m_medicalProjects.at(&patient).cancel();
		m_medicalProjects.erase(&patient);
		triage();
	}
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
	assert(!location.m_reservable.isFullyReserved(*doctor.m_faction));
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
void AreaHasMedicalPatientsForFaction::triage()
{
	while(!m_waitingPatients.empty() && !m_waitingDoctors.empty())
	{
		Actor& patient = **m_waitingPatients.begin();
		Actor& doctor = **m_waitingDoctors.begin();
		// TODO: get location nearest to patient?
		auto* location = *m_medicalLocations.begin();
		Wound& wound = patient.m_body.getWoundWhichIsBleedingTheMost();
		std::vector<const MedicalProjectType*> projectTypes;
		createProject(patient, location, doctor, wound, medicalProjectTypes);
	}
}
bool AreaHasMedicalPatientsForFaction::hasPatients() const { return !m_waitingPatients.empty(); }
