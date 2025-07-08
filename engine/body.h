#pragma once

#include "definitions/attackType.h"
#include "numericTypes/types.h"
#include "definitions/woundType.h"
#include "hit.h"
#include "eventSchedule.hpp"

#include <vector>
#include <utility>

struct BodyPart;
class WoundHealEvent;
class BleedEvent;
class WoundsCloseEvent;
struct DeserializationMemo;
struct Attack;
class Simulation;

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
	Wound(Area& area, const ActorIndex& a, const WoundType wt, BodyPart& bp, Hit h, const uint32_t bvr, const Percent ph = Percent::create(0));
	Wound(const Json& data, DeserializationMemo& deserializationMemo, BodyPart& bp);
	bool operator==(const Wound& other) const { return &other == this; }
	Percent getPercentHealed() const;
	Percent impairPercent() const;
	[[nodiscard]] Json toJson() const;
};
struct BodyPart final
{
	BodyPartTypeId bodyPartType;
	MaterialTypeId materialType;
	std::list<Wound> wounds;
	bool severed;
	BodyPart(const BodyPartTypeId& bpt, const MaterialTypeId& mt) : bodyPartType(bpt), materialType(mt), severed(false) {}
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
	MaterialTypeId m_solid;
	FullDisplacement m_totalVolume = FullDisplacement::create(0);
	Percent m_impairMovePercent = Percent::create(0);
	Percent m_impairManipulationPercent = Percent::create(0);
	FullDisplacement m_volumeOfBlood = FullDisplacement::create(0);
	bool m_isBleeding = false;
public:
	std::list<BodyPart> m_bodyParts;
	Body(Area& area, const ActorIndex& a);
	Body(const Json& data, DeserializationMemo& deserializationMemo, const ActorIndex& a);
	void initalize(Area& area);
	BodyPart& pickABodyPartByVolume(Simulation& simulation);
	BodyPart& pickABodyPartByType(const BodyPartTypeId& bodyPartType);
	// Armor has already been applied, calculate hit depth.
	void getHitDepth(Hit& hit, const BodyPart& bodyPart);
	Wound& addWound(Area& area, BodyPart& bodyPart, Hit& hit);
	void healWound(Area& area, Wound& wound);
	void doctorWound(Area& area, Wound& wound, const Percent healSpeedPercentageChange);
	void woundsClose(Area& area);
	void bleed(Area& area);
	void sever(BodyPart& bodyPart, Wound& wound);
	// TODO: periodicly update impairment as wounds heal.
	void recalculateBleedAndImpairment(Area& area);
	Wound& getWoundWhichIsBleedingTheMost();
	void setMaterialType(const MaterialTypeId& materialType) { m_solid = materialType; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool piercesSkin(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] bool piercesFat(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] bool piercesMuscle(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] bool piercesBone(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] FullDisplacement healthyBloodVolume() const;
	[[nodiscard]] std::vector<Attack> getMeleeAttacks() const;
	[[nodiscard]] FullDisplacement getVolume(Area& area) const;
	[[nodiscard]] bool isInjured() const;
	[[nodiscard]] Step getStepsTillBleedToDeath() const;
	[[nodiscard]] bool hasBodyPart(const BodyPartTypeId& bodyPartType) const;
	[[nodiscard]] Step getStepsTillWoundsClose() const { return m_woundsCloseEvent.remainingSteps(); }
	[[nodiscard]] Percent getImpairMovePercent() const { return m_impairMovePercent; }
	[[nodiscard]] Percent getImpairManipulationPercent() const { return m_impairManipulationPercent; }
	[[nodiscard]] std::vector<Wound*> getAllWounds();
	// For testing.
	[[maybe_unused, nodiscard]] bool hasBleedEvent() const { return m_bleedEvent.exists(); }
	[[maybe_unused, nodiscard]] bool hasBodyPart() const;
	[[maybe_unused, nodiscard]] Percent getImpairPercentFor(const BodyPartTypeId& bodyPartType) const;
	friend class WoundHealEvent;
	friend class BleedEvent;
	friend class WoundsCloseEvent;
};
class WoundHealEvent : public ScheduledEvent
{
	Wound& m_wound;
	Body& m_body;
public:
	WoundHealEvent(Simulation& simulation, const Step& delay, Wound& w, Body& b, const Step start = Step::null());
	void execute(Simulation&, Area* area) { m_body.healWound(*area, m_wound); }
	void clearReferences(Simulation&, Area*) { m_wound.healEvent.clearPointer(); }
};
class BleedEvent : public ScheduledEvent
{
	Body& m_body;
public:
	BleedEvent(Simulation& simulation, const Step& delay, Body& b, const Step start = Step::null());
	void execute(Simulation&, Area* area) { m_body.bleed(*area); }
	void clearReferences(Simulation&, Area*) { m_body.m_bleedEvent.clearPointer(); }
};
class WoundsCloseEvent : public ScheduledEvent
{
	Body& m_body;
public:
	WoundsCloseEvent(Simulation& simulation, const Step& delay, Body& b, const Step start = Step::null());
	void execute(Simulation&, Area* area) { m_body.woundsClose(*area); }
	void clearReferences(Simulation&, Area*) { m_body.m_woundsCloseEvent.clearPointer(); }
};
