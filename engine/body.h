#pragma once

#include "attackType.h"
#include "types.h"
#include "woundType.h"
#include "hit.h"
#include "eventSchedule.hpp"

#include <vector>
#include <utility>

struct BodyPart;
class WoundHealEvent;
class BleedEvent;
class WoundsCloseEvent;
struct MaterialType;
class Actor;
struct DeserializationMemo;
struct Attack;
class Simulation;

// For example: 'left arm', 'head', etc.
struct BodyPartType final
{
	const std::string name;
	const Volume volume;
	const bool doesLocamotion;
	const bool doesManipulation;
	const bool vital;
	std::vector<std::pair<const AttackType, const MaterialType*>> attackTypesAndMaterials;
	// Infastructure.
	bool operator==(const BodyPartType& bodyPartType) const { return this == &bodyPartType; }
	static const BodyPartType& byName(const std::string name);
};
inline std::vector<BodyPartType> bodyPartTypeDataStore;
// For example biped, quadraped, bird, etc.
struct BodyType final
{
	const std::string name;
	std::vector<const BodyPartType*> bodyPartTypes;
	bool hasBodyPart(const BodyPartType& bodyPartType) const
	{
		return std::ranges::find(bodyPartTypes, &bodyPartType) != bodyPartTypes.end();
	}
	// Infastructure.
	bool operator==(const BodyType& bodyType){ return this == &bodyType; }
	static const BodyType& byName(const std::string name);
};
inline std::vector<BodyType> bodyTypeDataStore;
struct Wound final
{
	const WoundType woundType;
	BodyPart& bodyPart;
	Hit hit;
	uint32_t bleedVolumeRate;
	Percent percentHealed;
	Percent maxPercentTemporaryImpairment;
	Percent maxPercentPermanantImpairment;
	HasScheduledEvent<WoundHealEvent> healEvent;
	Wound(Area& area, ActorIndex a, const WoundType wt, BodyPart& bp, Hit h, uint32_t bvr, Percent ph = 0);
	Wound(const Json& data, DeserializationMemo& deserializationMemo, BodyPart& bp);
	bool operator==(const Wound& other) const { return &other == this; }
	Percent getPercentHealed() const;
	Percent impairPercent() const;
	[[nodiscard]] Json toJson() const;
};
struct BodyPart final
{
	const BodyPartType& bodyPartType;
	const MaterialType& materialType;
	std::list<Wound> wounds;
	bool severed;
	BodyPart(const BodyPartType& bpt, const MaterialType& mt) : bodyPartType(bpt), materialType(mt), severed(false) {}
	BodyPart(const Json data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
};
/*
 * Body handles reciving wounds, bleeding, healing, temporary and permanant disability.
 * Bleeding happens at a rate of 1 unit of volume per bleed event, with the interval determined by the sum of Wound::bleedVolumeRate.
 */
class Body final
{
	HasScheduledEvent<BleedEvent> m_bleedEvent;
	HasScheduledEvent<WoundsCloseEvent> m_woundsCloseEvent;
	ActorIndex m_actor;
	const MaterialType* m_materialType = nullptr;
	uint32_t m_totalVolume = 0;
	Percent m_impairMovePercent = 0;
	Percent m_impairManipulationPercent = 0;
	Volume m_volumeOfBlood = 0;
	bool m_isBleeding = false;
public:
	std::list<BodyPart> m_bodyParts;
	Body(Area& area, ActorIndex a);
	Body(const Json& data, DeserializationMemo& deserializationMemo, ActorIndex a);
	void initalize(Area& area);
	BodyPart& pickABodyPartByVolume(Simulation& simulation);
	BodyPart& pickABodyPartByType(const BodyPartType& bodyPartType);
	// Armor has already been applied, calculate hit depth.
	void getHitDepth(Hit& hit, const BodyPart& bodyPart);
	Wound& addWound(Area& area, BodyPart& bodyPart, Hit& hit);
	void healWound(Area& area, Wound& wound);
	void doctorWound(Area& area, Wound& wound, Percent healSpeedPercentageChange);
	void woundsClose(Area& area);
	void bleed(Area& area);
	void sever(BodyPart& bodyPart, Wound& wound);
	// TODO: periodicly update impairment as wounds heal.
	void recalculateBleedAndImpairment(Area& area);
	Wound& getWoundWhichIsBleedingTheMost();
	void setMaterialType(const MaterialType& materialType) { m_materialType = &materialType; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool piercesSkin(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] bool piercesFat(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] bool piercesMuscle(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] bool piercesBone(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] Volume healthyBloodVolume() const;
	[[nodiscard]] std::vector<Attack> getMeleeAttacks() const;
	[[nodiscard]] Volume getVolume(Area& area) const;
	[[nodiscard]] bool isInjured() const;
	[[nodiscard]] Step getStepsTillBleedToDeath() const;
	[[nodiscard]] bool hasBodyPart(const BodyPartType& bodyPartType) const;
	[[nodiscard]] Step getStepsTillWoundsClose() const { return m_woundsCloseEvent.remainingSteps(); }
	[[nodiscard]] Percent getImpairMovePercent() const { return m_impairMovePercent; }
	[[nodiscard]] Percent getImpairManipulationPercent() const { return m_impairManipulationPercent; }
	[[nodiscard]] std::vector<Wound*> getAllWounds();
	// For testing.
	[[maybe_unused, nodiscard]] bool hasBleedEvent() const { return m_bleedEvent.exists(); }
	[[maybe_unused, nodiscard]] bool hasBodyPart() const;
	[[maybe_unused, nodiscard]] uint32_t getImpairPercentFor(const BodyPartType& bodyPartType) const;
	friend class WoundHealEvent;
	friend class BleedEvent;
	friend class WoundsCloseEvent;
};
class WoundHealEvent : public ScheduledEvent
{
	Wound& m_wound;
	Body& m_body;
public:
	WoundHealEvent(Simulation& simulation, const Step delay, Wound& w, Body& b, const Step start = 0);
	void execute(Simulation&, Area* area) { m_body.healWound(*area, m_wound); }
	void clearReferences(Simulation&, Area*) { m_wound.healEvent.clearPointer(); }
};
class BleedEvent : public ScheduledEvent
{
	Body& m_body;
public:
	BleedEvent(Simulation& simulation, const Step delay, Body& b, const Step start = 0);
	void execute(Simulation&, Area* area) { m_body.bleed(*area); }
	void clearReferences(Simulation&, Area*) { m_body.m_bleedEvent.clearPointer(); }
};
class WoundsCloseEvent : public ScheduledEvent
{
	Body& m_body;
public:
	WoundsCloseEvent(Simulation& simulation, const Step delay, Body& b, const Step start = 0);
	void execute(Simulation&, Area* area) { m_body.woundsClose(*area); }
	void clearReferences(Simulation&, Area*) { m_body.m_woundsCloseEvent.clearPointer(); }
};
