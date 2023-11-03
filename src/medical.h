#pragma once

#include "project.h"

class Actor;

struct MedicalProjectType final
{
	std::string name;
	uint32_t baseStepsDuration;
	const std::vector<std::pair<ItemQuery, uint32_t>> consumedItems;
	const std::vector<std::pair<ItemQuery, uint32_t>> unconsumedItems;
	const std::vector<std::pair<ItemType, uint32_t>> consumedItemsOfSameTypeAsProduct;
	const std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> byproductItems;
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
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	void onComplete();
	void onDelay();
	void offDelay();
	Step getDuration() const;
	uint32_t getItemScaleFactor() const;
public:
	MedicalProject(Actor& p, Block& location, Actor& doctor, Wound& wound, MedicalProjectType& medicalProjectType) : Project(doctor.getFaction(), location, 0), m_patient(p), m_doctor(doctor), m_wound(wound), m_medicalProjectType(medicalProjectType) { }
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
	std::unordered_set<Block*> m_medicalLocations;
	std::unordered_map<Actor*, MedicalProject> m_medicalProjects;
public:
	AreaHasMedicalPatientsForFaction(Faction& f) : m_faction(f) { }
	void addPatient(Actor& patient);
	void removePatient(Actor& patient);
	void addDoctor(Actor& doctor);
	void removeDoctor(Actor& doctor);
	void addLocation(Block& block);
	void removeLocation(Block& block);
	void createProject(Actor& patient, Block& location, Actor& doctor, Wound& wound, const MedicalProjectType& medicalProjectType);
	void cancelProject(MedicalProject& project);
	void triage();
	bool hasPatients() const;
};
