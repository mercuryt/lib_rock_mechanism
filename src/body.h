struct Hit
{
	uint32_t force;
	uint32_t area;
	uint32_t depth;
	Hit(uint32_t f, uint32_t a) : force(f), area(a)
};
struct BodyPartType
{
	const std::string name;
	const uint32_t area;
	const bool doesLocamotion;
	const bool doesManipulation;
	const std::vector<AttackType> attackTypes;
};
struct WoundType
{
	const std::string name;
	virtual uint32_t getStepsTillHealed(Hit& hit, BodyPartType& bodyPartType) = 0;
	virtual uint32_t getBleedVolumeRate(Hit& hit, BodyPartType& bodyPartType) = 0;
	virtual uint32_t getPercentTemporaryImpairment(Hit& hit, BodyPartType& bodyPartType) = 0;
	virtual uint32_t getPercentPermanentImpairment(Hit& hit, BodyPartType& bodyPartType) = 0;
};
class Wound
{
	const WoundType& m_woundType;
	BodyPart& m_bodyPart;
	uint32_t m_percentHealed;
	HasScheduledEvent<WoundHealEvent>* m_healEvent;
	uint32_t m_size;
	Wound(const WoundType& wt, BodyPart& bp, uint32_t m_size, uint32_t ph = 0) : m_woundType(wt), m_bodyPart(bp), m_percentHealed(ph), { }
	uint32_t getPercentHealed() const;
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
	Body(const BodyType& bodyType) : m_totalVolume(0), m_impairMovePercent(0), m_impairManipulationPercent(0);
	BodyPart& pickABodyPartByVolume();
	// Armor has already been applied, calculate hit depth.
	void getHitDepth(Hit& hit, const BodyPart& bodyPart, const MaterialType& materialType);
	Wound& addWound(const WoundType& woundType, const BodyPart& bodyPart, const Hit& hit);
	void healWound(Wound& wound, BodyPart& bodyPart);
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
	WoundHealEvent(Wound& w, Body& b) : m_wound(w), m_body(b) {}
	void execute() { m_body.healWound(m_wound); }
};
class BleedEvent : public ScheduledEventWithPercent
{
	const Body& m_body;
	BleedEvent(const Body& b) : m_body(b) {}
	void execute() { m_body.bleed(); }
};
class WoundsCloseEvent : public ScheduledEventWithPercent
{
	const Body& m_body;
	WoundsCloseEvent(const Body& b) : m_body(b) {}
	void execute() { m_body.woundsClose(); }
};
