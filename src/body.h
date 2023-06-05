struct WoundType
{
	std::string name;
	uint32_t getStepsTillHealed(Hit& hit, BodyPartType& bodyPartType) = 0;
	uint32_t getBleedVolumeRate(Hit& hit, BodyPartType& bodyPartType) = 0;
	uint32_t getPercentTemporaryImpairment(Hit& hit, BodyPartType& bodyPartType) = 0;
	uint32_t getPercentPermanentImpairment(Hit& hit, BodyPartType& bodyPartType) = 0;
};
struct Hit
{
	uint32_t force;
	uint32_t area;
	uint32_t depth;
	Hit(uint32_t f, uint32_t a) : force(f), area(a)
};
template<class Wound>
class WoundHealEvent : public ScheduledEventWithPercent
{
	Wound& m_wound;
	Body& m_body;
	WoundHealEvent(Wound& w, Body& b) : m_wound(w), m_body(b) {}
	void execute() { m_body.healWound(m_wound); }
};
template<class WoundType, class BodyPart>
class Wound
{
	const WoundType& m_woundType;
	BodyPart& m_bodyPart;
	uint32_t m_percentHealed;
	ScheduledEventWithPercent* m_healEvent;
	uint32_t m_size;
	Wound(const WoundType& wt, BodyPart& bp, uint32_t m_size, uint32_t ph = 0) : m_woundType(wt), m_bodyPart(bp), m_percentHealed(ph), m_healEvent(nullptr) {}
	uint32_t getPercentHealed() const
	{
		uint32_t output = m_percentHealed;
		if(m_healEvent != nullptr)
			output += util::scaleByPercent(100u - m_percentHealed, m_healEvent.percentComplete());
		return output;
	}
	~Wound()
	{
		if(m_healEvent != nullptr)
			m_healEvent.cancel();
	}
};
template<class BodyPartType, class MaterialType, class Wound>
class BodyPart
{
public:
	const BodyPartType& bodyPartType;
	const MaterialType& materialType;
	std::list<Wound> m_wounds;
	BodyPart(const BodyPartType& bpt, const MaterialType mt) : m_bodyPartType(bpt), m_materialType(mt) {}
};
template<class Body>
class BleedEvent : public ScheduledEventWithPercent
{
	const Body& m_body;
	BleedEvent(const Body& b) : m_body(b) {}
	void execute() { m_body.bleed(); }
};
template<class Body>
class WoundsCloseEvent : public ScheduledEventWithPercent
{
	const Body& m_body;
	WoundsCloseEvent(const Body& b) : m_body(b) {}
	void execute() { m_body.woundsClose(); }
};
template<class BodyType, class BodyPart, class Actor>
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
	Body(const BodyType& bodyType) : m_totalVolume(0), m_impairMovePercent(0), m_impairManipulationPercent(0)
	{
		for(auto& [bodyPartName, bodyPartType, materialType] : bodyType.bodyParts)
		{
			m_bodyParts.emplace_back(bodyPartType, materialType);
			totalVolume += m_bodyParts.volume;
		}
		m_volumeOfBlood = healthyBloodVolume();
	}
	BodyPart& pickABodyPartByVolume()
	{
		uint32_t roll = randomUtil::getInRange(0, m_totalVolume);
		for(BodyPart& bodyPart : m_bodyParts)
		{
			if(bodyPart.volume >= roll)
				return bodyPart;
			else
				roll -= bodyPart.volume
		}
		assert(false);
	}
	// Armor has already been applied, calculate hit depth.
	void getHitDepth(Hit& hit, const BodyPart& bodyPart, const MaterialType& materialType)
	{
		assert(hit.depth == 0);
		if(piercesSkin(hit))
		{
			++hit.depth;
			hit.m_force -= materialType.hardness * hit.m_area * Config::skinPierceForceCost;
			if(piercesFat(hit))
			{
				++hit.depth;
				hit.m_force -= materialType.hardness * hit.m_area * Config::fatPierceForceCost;
				if(piercesMuscle(hit))
				{
					++hit.depth;
					hit.m_force -= materialType.hardness * hit.m_area * Config::musclePierceForceCost;
					if(piercesBone(hit))
						++hit.depth;
				}
			}
		}
	}
	Wound& addWound(const WoundType& woundType, const BodyPart& bodyPart, const Hit& hit)
	{
		Wound& wound = bodyPart.m_wounds.emplace_back(woundType, bodyPart, hit);
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<WoundHealEvent>(wound, *this);
		wound.m_healEvent = event.get();
		m_actor.m_location->m_area->m_eventSchedule.schedule(event);
		recalculateBleedAndImpairment();
		return wound;
	}
	void healWound(Wound& wound, BodyPart& bodyPart)
	{
		bodyPart.m_wounds.remove(wound);
		recalculateBleedAndImpairment();
	}
	void doctorWound(Wound& wound, uint32_t healSpeedPercentageChange)
	{
		if(wound.bleedVolumeRate != 0)
		{
			wound.bleedVolumeRate = 0;
			recalculateBleedAndImpairment();
		}
		assert(wound.m_healEvent != nullptr;
		uint32_t remaningSteps = wound.m_healEvent.remaningSteps();
		remaningSteps = util.scaleByPercent(remaningSteps, healSpeedPercentageChange);
		wound.m_healEvent.cancel();
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<WoundHealEvent>(s_step + remaningSteps, wound);
		wound.m_healEvent = event.get();
		m_actor.m_location->m_area->m_eventSchedule.scedule(event);
	}
	void woundsClose()
	{
		for(BodyPart& bodyPart : m_bodyParts)
			for(Wound& wound : bodyPart.wounds)
				wound.bleedVolumeRate = 0;
		recalculateBleedAndImpairment();
	}
	void bleed()
	{
		m_volumeOfBlood -= m_bleedVolumeRate;
		float ratio = m_bleedVolumeRate / healthyBloodVolume();
		if(ratio <= Config::bleedToDeathRatio)
			m_actor.die();
		else if (ratio < Config::bleedToUnconciousessRatio)
			m_actor.passOut(Config::bleedPassOutDuration);
	}
	void recalculateBleedAndImpairment()
	{
		uint32_t bleedVolumeRate = 0;
		m_impairManipulationPercent = 0;
		m_impairMoveSpeedPercent = 0;
		for(BodyPart& bodyPart : m_bodyParts)
		{
			for(Wound& wound : bodyPart.wounds)
			{
				bleedVolumeRate += wound.bleedVolumeRate; // when wound closes set to 0.
				if(bodyPart.bodyPartType.doesLocamotion)
					m_impairMovePercent += wound.impairPercent;
				if(bodyPart.bodyPartType.doesManipulation)
					m_impairManipulationPercent += wound.impairPercent;
			}
			m_impairManipulationPercent = std::min(100, m_impairManipulationPercent);
			m_impairMovePercent = std::min(100, m_impairMovePercent);
		}
		if(bleedVolumeRate > 0 && m_bleedEvent == nullptr)
		{
			std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<BleedEvent>(s_step + Config::bleedEventFrequency, *this);
			m_bleedEvent = event.get();
			m_actor.m_location->m_area->m_eventSchedule.schedule(std::move(event));
		}
		else if(bleedVolumeRate == 0 && m_woundsCloseEvent != nullptr)
			m_bleedEvent.cancel();
		if(bleedVolumeRate > 0 && m_woundsCloseEvent == nullptr)
			uint32_t delay = bleedVolumeRate * Config::ratioWoundsCloseDelayToBleedVolume;
			std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<WoundsCloseEvent>(s_step + delay, *this);
			m_woundsCloseEvent = event.get();
			m_actor.m_location->m_area->m_eventSchedule.schedule(std::move(event));
		}
		else if(bleedVolumeRate == 0 && m_woundsCloseEvent != nullptr)
			m_woundsCloseEvent.cancel();
		m_actor.statsNoLongerValid();
	}
	bool piercesSkin(Hit hit)
	{
		uint32_t pierceScore = (force / area) * materialType.hardness * Config::pierceSkinModifier;
		uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.m_bodyPartType.area * bodyPart.m_materialType.hardness;
		return pierceScore > defenseScore;
	}
	bool piercesFat(Hit hit)
	{
		uint32_t pierceScore = (force / area) * materialType.hardness * Config::pierceFatModifier;
		uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.m_bodyPartType.area * bodyPart.m_materialType.hardness;
		return pierceScore > defenseScore;
	}
	bool piercesMuscle(Hit hit)
	{
		uint32_t pierceScore = (force / area) * materialType.hardness * Config::pierceMuscelModifier;
		uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.m_bodyPartType.area * bodyPart.m_materialType.hardness;
		return pierceScore > defenseScore;
	}
	bool piercesBone(Hit hit)
	{
		uint32_t pierceScore = (force / area) * materialType.hardness * Config::pierceBoneModifier;
		uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.m_bodyPartType.area * bodyPart.m_materialType.hardness;
		return pierceScore > defenseScore;
	}
	uint32_t healthyBloodVolume() const
	{
		return m_totalVolume * Config::ratioOfTotalBodyVolumeWhichIsBlood;
	}
};
