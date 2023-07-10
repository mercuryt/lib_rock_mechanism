#pragma once

#include "materialType.h"
#include "woundType.h"
#include "attackType.h"
#include "fight.h"
#include "hit.h"
#include "util.h"

#include <vector>
#include <utility>

struct BodyPartType
{
	const std::string name;
	const uint32_t area;
	const bool doesLocamotion;
	const bool doesManipulation;
	std::vector<AttackType> attackTypes;
	// Infastructure.
	bool operator==(const BodyPartType& bodyPartType){ return this == &bodyPartType; }
	static std::vector<BodyPartType> data;
	static const BodyPartType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &BodyPartType::name);
		assert(found != data.end());
		return *found;
	}
};
struct BodyType
{
	std::vector<std::pair<const BodyPartType*, const MaterialType*>> bodyParts;
};
class BodyPart;
class WoundHealEvent;
class Wound
{
	const WoundType& m_woundType;
	BodyPart& m_bodyPart;
	uint32_t m_percentHealed;
	HasScheduledEvent<WoundHealEvent>* m_healEvent;
	uint32_t m_size;
public:
	Wound(const WoundType& wt, BodyPart& bp, uint32_t s, uint32_t ph = 0) : m_woundType(wt), m_bodyPart(bp), m_percentHealed(ph), m_size(s) { }
	const uint32_t& getPercentHealed() const;
	~Wound();
};
class BodyPart
{
public:
	const BodyPartType& bodyPartType;
	const MaterialType& materialType;
	std::list<Wound> m_wounds;
	BodyPart(const BodyPartType& bpt, const MaterialType& mt);
};
class Body
{
	Actor& m_actor;
	std::list<BodyPart> m_bodyParts;
	uint32_t m_totalVolume;
	uint32_t m_impairMovePercent;
	uint32_t m_impairManipulationPercent;
	uint32_t m_volumeOfBlood;
	uint32_t m_bleedVolumeRate;
	ScheduledEventWithPercent* m_bleedEvent;
public:
	Body(Actor& a, const BodyType& bt);
	BodyPart& pickABodyPartByVolume();
	// Armor has already been applied, calculate hit depth.
	void getHitDepth(Hit& hit, const BodyPart& bodyPart, const MaterialType& materialType);
	Wound& addWound(const WoundType& woundType, const BodyPart& bodyPart, const Hit& hit);
	void healWound(Wound& wound);
	void doctorWound(Wound& wound, uint32_t healSpeedPercentageChange);
	void woundsClose();
	void bleed();
	void recalculateBleedAndImpairment();
	bool piercesSkin(Hit hit);
	bool piercesFat(Hit hit);
	bool piercesMuscle(Hit hit);
	bool piercesBone(Hit hit);
	uint32_t healthyBloodVolume() const;
	std::vector<Attack> getAttacks();
};
class WoundHealEvent : public ScheduledEventWithPercent
{
	Wound& m_wound;
	Body& m_body;
	WoundHealEvent(uint32_t delay, Wound& w, Body& b) : ScheduledEventWithPercent(delay), m_wound(w), m_body(b) {}
	void execute() { m_body.healWound(m_wound); }
};
class BleedEvent : public ScheduledEventWithPercent
{
	Body& m_body;
	BleedEvent(uint32_t delay, Body& b) : ScheduledEventWithPercent(delay), m_body(b) {}
	void execute() { m_body.bleed(); }
};
class WoundsCloseEvent : public ScheduledEventWithPercent
{
	Body& m_body;
	WoundsCloseEvent(uint32_t delay, Body& b) : ScheduledEventWithPercent(delay), m_body(b) {}
	void execute() { m_body.woundsClose(); }
};
