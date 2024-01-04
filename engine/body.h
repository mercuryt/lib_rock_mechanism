#pragma once

#include "attackType.h"
#include "deserializationMemo.h"
#include "woundType.h"
#include "hit.h"
#include "eventSchedule.hpp"
#include "fight.h"

#include <vector>
#include <utility>

struct BodyPart;
class WoundHealEvent;
class BleedEvent;
class WoundsCloseEvent;
struct MaterialType;
class Actor;

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
	inline static std::vector<BodyPartType> data;
	static const BodyPartType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &BodyPartType::name);
		assert(found != data.end());
		return *found;
	}
};
// For example biped, quadraped, bird, etc.
struct BodyType final
{
	const std::string name;
	std::vector<const BodyPartType*> bodyPartTypes;
	// Infastructure.
	bool operator==(const BodyType& bodyType){ return this == &bodyType; }
	inline static std::vector<BodyType> data;
	static const BodyType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &BodyType::name);
		assert(found != data.end());
		return *found;
	}
	bool hasBodyPart(const BodyPartType& bodyPartType) const
	{
		return std::ranges::find(bodyPartTypes, &bodyPartType) != bodyPartTypes.end();
	}
};
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
	Wound(Actor& a, const WoundType wt, BodyPart& bp, Hit h, uint32_t bvr, Percent ph = 0);
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
	Actor& m_actor;
	const MaterialType* m_materialType;
	uint32_t m_totalVolume;
	Percent m_impairMovePercent;
	Percent m_impairManipulationPercent;
	Volume m_volumeOfBlood;
	bool m_isBleeding;
	HasScheduledEvent<BleedEvent> m_bleedEvent;
	HasScheduledEvent<WoundsCloseEvent> m_woundsCloseEvent;
public:
	std::list<BodyPart> m_bodyParts;
	Body(Actor& a);
	Body(const Json& data, DeserializationMemo& deserializationMemo, Actor& a);
	BodyPart& pickABodyPartByVolume();
	BodyPart& pickABodyPartByType(const BodyPartType& bodyPartType);
	// Armor has already been applied, calculate hit depth.
	void getHitDepth(Hit& hit, const BodyPart& bodyPart);
	Wound& addWound(BodyPart& bodyPart, Hit& hit);
	void healWound(Wound& wound);
	void doctorWound(Wound& wound, Percent healSpeedPercentageChange);
	void woundsClose();
	void bleed();
	void sever(BodyPart& bodyPart, Wound& wound);
	// TODO: periodicly update impairment as wounds heal.
	void recalculateBleedAndImpairment();
	Wound& getWoundWhichIsBleedingTheMost();
	[[nodiscard]] bool piercesSkin(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] bool piercesFat(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] bool piercesMuscle(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] bool piercesBone(Hit hit, const BodyPart& bodyPart) const;
	[[nodiscard]] Volume healthyBloodVolume() const;
	[[nodiscard]] std::vector<Attack> getMeleeAttacks() const;
	[[nodiscard]] Volume getVolume() const;
	[[nodiscard]] bool isInjured() const;
	[[nodiscard]] Step getStepsTillBleedToDeath() const;
	[[nodiscard]] bool hasBodyPart(const BodyPartType& bodyPartType) const;
	[[nodiscard]] Step getStepsTillWoundsClose() const { return m_woundsCloseEvent.remainingSteps(); }
	[[nodiscard]] Percent getImpairMovePercent() { return m_impairMovePercent; }
	[[nodiscard]] Percent getImpairManipulationPercent() { return m_impairManipulationPercent; }
	[[nodiscard]] Json toJson() const;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasBleedEvent() const { return m_bleedEvent.exists(); }
	[[maybe_unused, nodiscard]] bool hasBodyPart() const;
	[[maybe_unused, nodiscard]] uint32_t getImpairPercentFor(const BodyPartType& bodyPartType) const;
	friend class WoundHealEvent;
	friend class BleedEvent;
	friend class WoundsCloseEvent;
};
class WoundHealEvent : public ScheduledEventWithPercent
{
	Wound& m_wound;
	Body& m_body;
public:
	WoundHealEvent(const Step delay, Wound& w, Body& b, const Step start = 0);
	void execute() { m_body.healWound(m_wound); }
	void clearReferences() { m_wound.healEvent.clearPointer(); }
};
class BleedEvent : public ScheduledEventWithPercent
{
	Body& m_body;
public:
	BleedEvent(const Step delay, Body& b, const Step start = 0);
	void execute() { m_body.bleed(); }
	void clearReferences() { m_body.m_bleedEvent.clearPointer(); }
};
class WoundsCloseEvent : public ScheduledEventWithPercent
{
	Body& m_body;
public:
	WoundsCloseEvent(const Step delay, Body& b, const Step start = 0);
	void execute() { m_body.woundsClose(); }
	void clearReferences() { m_body.m_woundsCloseEvent.clearPointer(); }
};
