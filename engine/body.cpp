#include "body.h"
#include "animalSpecies.h"
#include "config.h"
#include "deserializationMemo.h"
#include "random.h"
#include "reservable.h"
#include "types.h"
#include "util.h"
#include "simulation.h"
#include "woundType.h"
#include "area.h"
// For Attack declaration.
#include "actors/actors.h"
#include <cstdint>
// Static method.
const BodyPartType& BodyPartType::byName(const std::string name)
{
	auto found = std::ranges::find(bodyPartTypeDataStore, name, &BodyPartType::name);
	assert(found != bodyPartTypeDataStore.end());
	return *found;
}
// Static method.
const BodyType& BodyType::byName(const std::string name)
{
	auto found = std::ranges::find(bodyTypeDataStore, name, &BodyType::name);
	assert(found != bodyTypeDataStore.end());
	return *found;
}
Wound::Wound(Area& area, ActorIndex a, const WoundType wt, BodyPart& bp, Hit h, uint32_t bvr, Percent ph) :
	woundType(wt), bodyPart(bp), hit(h), bleedVolumeRate(bvr), percentHealed(ph), healEvent(area.m_eventSchedule)
{
	const AnimalSpecies& species = area.getActors().getSpecies(a);
	maxPercentTemporaryImpairment = WoundCalculations::getPercentTemporaryImpairment(hit, bodyPart.bodyPartType, species.bodyScale);
	maxPercentPermanantImpairment = WoundCalculations::getPercentPermanentImpairment(hit, bodyPart.bodyPartType, species.bodyScale);
}
Wound::Wound(const Json& data, DeserializationMemo& deserializationMemo, BodyPart& bp) :
	woundType(woundTypeByName(data["woundType"].get<std::string>())),
	bodyPart(bp), hit(data["hit"]), bleedVolumeRate(data["bleedVolumeRate"].get<uint32_t>()),
	percentHealed(data["percentHealed"].get<Percent>()), healEvent(deserializationMemo.m_simulation.m_eventSchedule) { }
Json Wound::toJson() const
{
	Json data;
	data["woundType"] = getWoundTypeName(woundType);
	data["hit"] = hit.toJson();
	data["bleedVolumeRate"] = bleedVolumeRate;
	data["percentHealed"] = getPercentHealed();
	return data;
}
Percent Wound::getPercentHealed() const
{
	Percent output = percentHealed;
	if(healEvent.exists())
			output += healEvent.percentComplete();
	return output;
}
Percent Wound::impairPercent() const
{
	return Percent::create(util::scaleByInversePercent(maxPercentTemporaryImpairment.get(), getPercentHealed())) + maxPercentPermanantImpairment;
}
BodyPart::BodyPart(const Json data, DeserializationMemo& deserializationMemo) :
	bodyPartType(BodyPartType::byName(data["bodyPartType"].get<std::string>())),
	materialType(MaterialType::byName(data["materialType"].get<std::string>())),
	severed(data["severed"].get<bool>())
{
	for(const Json& wound : data["wounds"])
		wounds.emplace_back(wound, deserializationMemo, *this);
}
Json BodyPart::toJson() const
{
	Json data;
	data["bodyPartType"] = bodyPartType.name;
	data["materialType"] = materialType.name;
	data["severed"] = severed;
	data["wounds"] = Json::array();
	for(const Wound& wound : wounds)
		data["wounds"].push_back(wound.toJson());
	return data;
}
Body::Body(Area& area, ActorIndex a) :  m_bleedEvent(area.m_eventSchedule), m_woundsCloseEvent(area.m_eventSchedule), m_actor(a) { }
void Body::initalize(Area& area)
{
	const AnimalSpecies& species = area.getActors().getSpecies(m_actor);
	setMaterialType(species.materialType);
	for(const BodyPartType* bodyPartType : species.bodyType.bodyPartTypes)
	{
		m_bodyParts.emplace_back(*bodyPartType, *m_materialType);
		m_totalVolume += bodyPartType->volume;
	}
	m_volumeOfBlood = healthyBloodVolume();
}
Body::Body(const Json& data, DeserializationMemo& deserializationMemo, ActorIndex a) :
       	m_bleedEvent(deserializationMemo.m_simulation.m_eventSchedule),
	m_woundsCloseEvent(deserializationMemo.m_simulation.m_eventSchedule),
	m_actor(a),
	m_materialType(&MaterialType::byName(data["materialType"].get<std::string>())),
	m_totalVolume(data["totalVolume"].get<Volume>()),
	m_impairMovePercent(data.contains("impairMovePercent") ? data["impairMovePercent"].get<Percent>() : Percent::create(0)),
	m_impairManipulationPercent(data.contains("impairManipulationPercent") ? data["impairManipulationPercent"].get<Percent>() : Percent::create(0)),
	m_volumeOfBlood(data["volumeOfBlood"].get<Volume>()), m_isBleeding(data["isBleeding"].get<bool>())
{
	for(const Json& bodyPart : data["bodyParts"])
		m_bodyParts.emplace_back(bodyPart, deserializationMemo);
	if(data.contains("woundsCloseEventStart"))
		m_woundsCloseEvent.schedule(deserializationMemo.m_simulation, data["woundsCloseEventDuration"].get<Step>(), *this, data["woundsCloseEventStart"].get<Step>());
	if(data.contains("bleedEventStart"))
		m_bleedEvent.schedule(deserializationMemo.m_simulation, data["bleedEventDuration"].get<Step>(), *this, data["bleedEventStart"].get<Step>());
}
Json Body::toJson() const
{
	Json data{
		{"materialType", m_materialType->name},
		{"totalVolume", m_totalVolume},
		{"volumeOfBlood", m_volumeOfBlood},
		{"isBleeding", m_isBleeding},
	};
	if(m_impairMovePercent != Percent::create(0))
		data["impairMovePercent"] = m_impairMovePercent;
	if(m_impairManipulationPercent != Percent::create(0))
		data["impairManipulationPercent"] = m_impairManipulationPercent;
	if(m_bleedEvent.exists())
	{
		data["bleedEventStart"] = m_bleedEvent.getStartStep();
		data["bleedEventDuration"] = m_bleedEvent.duration();
	}
	if(m_woundsCloseEvent.exists())
	{
		data["woundsCloseEventStart"] = m_woundsCloseEvent.getStartStep();
		data["woundsCloseEventDuration"] = m_woundsCloseEvent.duration();
	}
	data["bodyParts"] = Json::array();
	for(const BodyPart& bodyPart : m_bodyParts)
		data["bodyParts"].push_back(bodyPart.toJson());
	return data;
}
BodyPart& Body::pickABodyPartByVolume(Simulation& simulation)
{
	Random& random = simulation.m_random;
	uint32_t roll = random.getInRange(0u, m_totalVolume.get());
	for(BodyPart& bodyPart : m_bodyParts)
	{
		if(bodyPart.bodyPartType.volume >= roll)
			return bodyPart;
		else
			roll -= bodyPart.bodyPartType.volume.get();
	}
	assert(false);
	return m_bodyParts.front();
}
BodyPart& Body::pickABodyPartByType(const BodyPartType& bodyPartType)
{
	for(BodyPart& bodyPart : m_bodyParts)
		if(bodyPart.bodyPartType == bodyPartType)
				return bodyPart;
	assert(false);
	return m_bodyParts.front();
}
// Armor has already been applied, calculate hit depth.
void Body::getHitDepth(Hit& hit, const BodyPart& bodyPart)
{
	assert(hit.depth == 0);
	if(piercesSkin(hit, bodyPart))
	{
		++hit.depth;
		uint32_t hardnessDelta = std::max(1, (int32_t)bodyPart.materialType.hardness - (int32_t)hit.materialType.hardness);
		Force forceDelta = Force::create(hardnessDelta * hit.area * Config::skinPierceForceCost);
		hit.force = hit.force < forceDelta ? Force::create(0) : hit.force - forceDelta;
		if(piercesFat(hit, bodyPart))
		{
			++hit.depth;
			forceDelta = Force::create(hardnessDelta * hit.area * Config::fatPierceForceCost);
			hit.force = hit.force < forceDelta ? Force::create(0) : hit.force - forceDelta;
			if(piercesMuscle(hit, bodyPart))
			{
				++hit.depth;
				forceDelta = Force::create(hardnessDelta * hit.area * Config::musclePierceForceCost);
				hit.force = hit.force < forceDelta ? Force::create(0) : hit.force - forceDelta;
				if(piercesBone(hit, bodyPart))
					++hit.depth;
			}
		}
	}
}
Wound& Body::addWound(Area& area, BodyPart& bodyPart, Hit& hit)
{
	assert(hit.depth != 0);
	Actors& actors = area.getActors();
	uint32_t scale = actors.getSpecies(m_actor).bodyScale;
	uint32_t bleedVolumeRate = WoundCalculations::getBleedVolumeRate(hit, bodyPart.bodyPartType, scale);
	Wound& wound = bodyPart.wounds.emplace_back(area, m_actor, hit.woundType, bodyPart, hit, bleedVolumeRate);
	float ratio = (float)hit.area / bodyPart.bodyPartType.volume.get() * scale;
	if(bodyPart.bodyPartType.vital && hit.depth == 4 && ratio >= Config::hitAreaToBodyPartVolumeRatioForFatalStrikeToVitalArea)
	{
		actors.die(m_actor, CauseOfDeath::wound);
		return wound;
	}
	else if(hit.woundType == WoundType::Cut && hit.depth == 4 && ((float)hit.area / (float)bodyPart.bodyPartType.volume.get()) > Config::ratioOfHitAreaToBodyPartVolumeForSever)
	{
		sever(bodyPart, wound);
		if(bodyPart.bodyPartType.vital)
		{
			actors.die(m_actor, CauseOfDeath::wound);
			return wound;
		}
	}
	else
	{
		Step delayTillHeal = WoundCalculations::getStepsTillHealed(hit, bodyPart.bodyPartType, scale);
		wound.healEvent.schedule(area.m_simulation, delayTillHeal, wound, *this);
	}
	recalculateBleedAndImpairment(area);
	return wound;
}
void Body::healWound(Area& area, Wound& wound)
{
	wound.bodyPart.wounds.remove(wound);
	recalculateBleedAndImpairment(area);
}
void Body::doctorWound(Area& area, Wound& wound, Percent healSpeedPercentageChange)
{
	assert(wound.healEvent.exists());
	if(wound.bleedVolumeRate != 0)
	{
		wound.bleedVolumeRate = 0;
		recalculateBleedAndImpairment(area);
	}
	Step remaningSteps = wound.healEvent.remainingSteps();
	remaningSteps = Step::create(util::scaleByPercent(remaningSteps.get(), healSpeedPercentageChange));
	wound.healEvent.unschedule();
	wound.healEvent.schedule(area.m_simulation, remaningSteps, wound, *this);
}
void Body::woundsClose(Area& area)
{
	for(BodyPart& bodyPart : m_bodyParts)
		for(Wound& wound : bodyPart.wounds)
			wound.bleedVolumeRate = 0;
	recalculateBleedAndImpairment(area);
}
void Body::bleed(Area& area)
{
	Actors& actors = area.getActors();
	--m_volumeOfBlood;
	float ratio = (float)m_volumeOfBlood.get() / (float)healthyBloodVolume().get();
	if(ratio <= Config::bleedToDeathRatio)
		actors.die(m_actor, CauseOfDeath::bloodLoss);
	else
	{
		if (actors.sleep_isAwake(m_actor) && ratio < Config::bleedToUnconciousessRatio)
			actors.passout(m_actor, Config::bleedPassOutDuration);
		recalculateBleedAndImpairment(area);
	}
}
void Body::sever(BodyPart& bodyPart, Wound& wound)
{
	wound.maxPercentPermanantImpairment = Percent::create(100);
	wound.maxPercentTemporaryImpairment = Percent::create(0);
	bodyPart.severed = true;
}
void Body::recalculateBleedAndImpairment(Area& area)
{
	uint32_t bleedVolumeRate = 0;
	m_impairManipulationPercent = Percent::create(0);
	m_impairMovePercent = Percent::create(0);
	for(BodyPart& bodyPart : m_bodyParts)
	{
		for(Wound& wound : bodyPart.wounds)
		{
			bleedVolumeRate += wound.bleedVolumeRate; // when wound closes set to 0.
			if(bodyPart.bodyPartType.doesLocamotion)
				m_impairMovePercent += wound.impairPercent();
			if(bodyPart.bodyPartType.doesManipulation)
				m_impairManipulationPercent += wound.impairPercent();
		}
	}
	if(m_impairManipulationPercent > 100)
		m_impairManipulationPercent = Percent::create(100);
	m_impairMovePercent = std::min(Percent::create(100), m_impairMovePercent);
	if(bleedVolumeRate > 0)
	{
		// Calculate bleed event frequency from bleed volume rate.
		Step baseFrequency = Config::bleedEventBaseFrequency / bleedVolumeRate;
		Step baseWoundsCloseDelay = Config::ratioWoundsCloseDelayToBleedVolume * bleedVolumeRate;
		// Apply already elapsed progress from existing event ( if any ).
		if(m_isBleeding)
		{
			// Already bleeding, reschedule bleed event and wounds close event.
			assert(m_woundsCloseEvent.exists());
			Step adjustedFrequency = m_bleedEvent.exists() ?
				Step::create(util::scaleByInversePercent(baseFrequency.get(), m_bleedEvent.percentComplete())) :
				baseFrequency;
			Step toScheduleStep = area.m_simulation.m_step + adjustedFrequency;
			if(!m_bleedEvent.exists() || toScheduleStep != m_bleedEvent.getStep())
			{
				m_bleedEvent.maybeUnschedule();
				m_bleedEvent.schedule(area.m_simulation, adjustedFrequency, *this);
			}
			Step adjustedWoundsCloseDelay = Step::create(util::scaleByInversePercent(baseWoundsCloseDelay.get(), m_woundsCloseEvent.percentComplete()));
			toScheduleStep = area.m_simulation.m_step + adjustedWoundsCloseDelay;
			if(!m_woundsCloseEvent.exists() || toScheduleStep != m_woundsCloseEvent.getStep())
			{
				m_woundsCloseEvent.maybeUnschedule();
				m_woundsCloseEvent.schedule(area.m_simulation, adjustedWoundsCloseDelay, *this);
			}
		}
		else
		{
			// Start bleeding.
			m_isBleeding = true;
			m_bleedEvent.schedule(area.m_simulation, baseFrequency, *this);
			m_woundsCloseEvent.schedule(area.m_simulation, baseWoundsCloseDelay, *this);
		}
	}
	else
	{
		// Stop bleeding if bleeding.
		m_isBleeding = false;
		m_woundsCloseEvent.maybeUnschedule();
		m_bleedEvent.maybeUnschedule();
	}
	// Update move speed for current move impairment level.
	area.getActors().move_updateIndividualSpeed(m_actor);
	// Update combat score for current manipulation impairment level.
	area.getActors().combat_update(m_actor);
}
Wound& Body::getWoundWhichIsBleedingTheMost()
{
	Wound* output = nullptr;
	uint32_t outputBleedRate = 0;
	for(BodyPart& bodyPart : m_bodyParts)
		for(Wound& wound : bodyPart.wounds)
			if(wound.bleedVolumeRate >= outputBleedRate)
			{
				outputBleedRate = wound.bleedVolumeRate;
				output = &wound;
			}
	assert(output != nullptr);
	return *output;
}
Step Body::getStepsTillBleedToDeath() const
{
	assert(m_bleedEvent.exists());
	Step output = m_bleedEvent.remainingSteps();
	if(m_volumeOfBlood > 1)
	{
		auto volume = m_volumeOfBlood - 1;
		while(true)
		{
			output += m_bleedEvent.duration();
			if(volume == 0 || (float)volume.get() / (float)healthyBloodVolume().get() <= Config::bleedToDeathRatio)
				return output;
			--volume;
		}
	}
	else
		return output;
}
bool Body::piercesSkin(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (uint32_t)((float)hit.force.get() / (float)hit.area) * hit.materialType.hardness * Config::pierceSkinModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume.get() * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesFat(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (uint32_t)((float)hit.force.get() / (float)hit.area) * hit.materialType.hardness * Config::pierceFatModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume.get() * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesMuscle(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (uint32_t)((float)hit.force.get() / (float)hit.area) * hit.materialType.hardness * Config::pierceMuscelModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume.get() * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesBone(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (uint32_t)((float)hit.force.get() / (float)hit.area) * hit.materialType.hardness * Config::pierceBoneModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume.get() * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
Volume Body::healthyBloodVolume() const
{
	return m_totalVolume * Config::ratioOfTotalBodyVolumeWhichIsBlood;
}
std::vector<Attack> Body::getMeleeAttacks() const
{
	std::vector<Attack> output;
	for(const BodyPart& bodyPart : m_bodyParts)
		for(auto& [attackType, materialType] : bodyPart.bodyPartType.attackTypesAndMaterials)
			output.emplace_back(&attackType, materialType, ItemIndex::null());
	return output;
}
Volume Body::getVolume(Area& area) const
{
	return Volume::create(util::scaleByPercent(m_totalVolume.get(), area.getActors().getPercentGrown(m_actor)));
}
bool Body::isInjured() const
{
	for(const BodyPart& bodyPart : m_bodyParts)
		if(!bodyPart.wounds.empty())
			return true;
	return false;
}
bool Body::hasBodyPart(const BodyPartType& bodyPartType) const
{
	for(const BodyPart& bodyPart : m_bodyParts)
		if(bodyPart.bodyPartType == bodyPartType)
			return true;
	return false;
}
Percent Body::getImpairPercentFor(const BodyPartType& bodyPartType) const
{
	Percent output = Percent::create(0);
	for(const BodyPart& bodyPart : m_bodyParts)
		if(bodyPart.bodyPartType == bodyPartType)
			for(const Wound& wound : bodyPart.wounds)
				output += wound.impairPercent();
	return output;
}
std::vector<Wound*> Body::getAllWounds()
{
	std::vector<Wound*> output;
	for(const BodyPart& bodyPart : m_bodyParts)
		for(const Wound& wound : bodyPart.wounds)
			output.push_back(&const_cast<Wound&>(wound));
	return output;
}
WoundHealEvent::WoundHealEvent(Simulation& simulation, const Step delay, Wound& w, Body& b, const Step start) :
	ScheduledEvent(simulation, delay, start), m_wound(w), m_body(b) {}
BleedEvent::BleedEvent(Simulation& simulation, const Step delay, Body& b, const Step start) :
	ScheduledEvent(simulation, delay, start), m_body(b) {}
WoundsCloseEvent::WoundsCloseEvent(Simulation& simulation, const Step delay, Body& b, const Step start) :
	ScheduledEvent(simulation, delay, start), m_body(b) {}
