#pragma once

#include "materialType.h"
#include "woundType.h"
#include "attackType.h"
#include "fight.h"
#include "hit.h"
#include "eventSchedule.h"
#include "eventSchedule.hpp"

#include <vector>
#include <utility>

struct BodyPartType
{
	const std::string name;
	const uint32_t volume;
	const bool doesLocamotion;
	const bool doesManipulation;
	std::vector<std::pair<const AttackType, const MaterialType*>> attackTypesAndMaterials;
	// Infastructure.
	bool operator==(const BodyPartType& bodyPartType){ return this == &bodyPartType; }
	inline static std::vector<BodyPartType> data;
	static const BodyPartType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &BodyPartType::name);
		assert(found != data.end());
		return *found;
	}
};
struct BodyTypeCategory
{
	const std::string name;
	// Infastructure.
	bool operator==(const BodyTypeCategory& bodyType){ return this == &bodyType; }
	inline static std::vector<BodyTypeCategory> data;
	static const BodyTypeCategory& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &BodyTypeCategory::name);
		assert(found != data.end());
		return *found;
	}
};
struct BodyType
{
	const std::string name;
	const BodyTypeCategory& category;
	uint32_t scale;
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
};
struct BodyPart;
class WoundHealEvent;
class BleedEvent;
class WoundsCloseEvent;
struct Wound
{
	const WoundType& woundType;
	BodyPart& bodyPart;
	Hit hit;
	uint32_t bleedVolumeRate;
	uint32_t percentHealed;
	uint32_t maxPercentTemporaryImpairment;
	uint32_t maxPercentPermanantImpairment;
	HasScheduledEvent<WoundHealEvent> healEvent;
	Wound(const WoundType& wt, BodyPart& bp, Hit h, uint32_t bvr, uint32_t ph = 0) : woundType(wt), bodyPart(bp), hit(h), bleedVolumeRate(bvr), percentHealed(ph) { }
	bool operator==(const Wound& other) const { return &other == this; }
	uint32_t getPercentHealed() const;
	uint32_t impairPercent() const;
};
struct BodyPart
{
	const BodyPartType& bodyPartType;
	const MaterialType& materialType;
	std::list<Wound> wounds;
	BodyPart(const BodyPartType& bpt, const MaterialType& mt) : bodyPartType(bpt), materialType(mt) {}
};
class Body
{
	Actor& m_actor;
	std::list<BodyPart> m_bodyParts;
	const MaterialType* m_materialType;
	uint32_t m_totalVolume;
	uint32_t m_impairMovePercent;
	uint32_t m_impairManipulationPercent;
	uint32_t m_volumeOfBlood;
	uint32_t m_bleedVolumeRate;
	HasScheduledEvent<BleedEvent> m_bleedEvent;
	HasScheduledEvent<WoundsCloseEvent> m_woundsCloseEvent;
public:
	Body(Actor& a);
	BodyPart& pickABodyPartByVolume();
	// Armor has already been applied, calculate hit depth.
	void getHitDepth(Hit& hit, const BodyPart& bodyPart);
	Wound& addWound(const WoundType& woundType, BodyPart& bodyPart, const Hit& hit);
	void healWound(Wound& wound);
	void doctorWound(Wound& wound, uint32_t healSpeedPercentageChange);
	void woundsClose();
	void bleed();
	// TODO: periodicly update impairment as wounds heal.
	void recalculateBleedAndImpairment();
	bool piercesSkin(Hit hit, const BodyPart& bodyPart) const;
	bool piercesFat(Hit hit, const BodyPart& bodyPart) const;
	bool piercesMuscle(Hit hit, const BodyPart& bodyPart) const;
	bool piercesBone(Hit hit, const BodyPart& bodyPart) const;
	uint32_t healthyBloodVolume() const;
	std::vector<Attack> getAttacks() const;
	uint32_t getVolume() const;
	bool isInjured() const;
};
class WoundHealEvent : public ScheduledEventWithPercent
{
	Wound& m_wound;
	Body& m_body;
public:
	WoundHealEvent(uint32_t delay, Wound& w, Body& b) : ScheduledEventWithPercent(delay), m_wound(w), m_body(b) {}
	void execute() { m_body.healWound(m_wound); }
};
class BleedEvent : public ScheduledEventWithPercent
{
	Body& m_body;
public:
	BleedEvent(uint32_t delay, Body& b) : ScheduledEventWithPercent(delay), m_body(b) {}
	void execute() { m_body.bleed(); }
};
class WoundsCloseEvent : public ScheduledEventWithPercent
{
	Body& m_body;
public:
	WoundsCloseEvent(uint32_t delay, Body& b) : ScheduledEventWithPercent(delay), m_body(b) {}
	void execute() { m_body.woundsClose(); }
};
