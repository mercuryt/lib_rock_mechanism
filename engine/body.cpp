#include "body.h"
#include "definitions/bodyType.h"
#include "definitions/animalSpecies.h"
#include "config/config.h"
#include "deserializationMemo.h"
#include "random.h"
#include "reference.h"
#include "reservable.h"
#include "numericTypes/types.h"
#include "util.h"
#include "simulation/simulation.h"
#include "definitions/woundType.h"
#include "area/area.h"
// For Attack declaration.
#include "actors/actors.h"
#include <cstdint>
// Static method.
Wound::Wound(Area& area, const ActorIndex& a, const WoundType wt, BodyPart& bp, Hit h, int32_t bvr, Percent ph) :
	woundType(wt), bodyPart(bp), hit(h), bleedVolumeRate(bvr), percentHealed(ph), healEvent(area.m_eventSchedule)
{
	AnimalSpeciesId species = area.getActors().getSpecies(a);
	auto scale = AnimalSpecies::getBodyScale(species);
	maxPercentTemporaryImpairment = WoundCalculations::getPercentTemporaryImpairment(hit, bodyPart.bodyPartType, scale);
	maxPercentPermanantImpairment = WoundCalculations::getPercentPermanentImpairment(hit, bodyPart.bodyPartType, scale);
}
Wound::Wound(const Json& data, DeserializationMemo& deserializationMemo, BodyPart& bp) :
	woundType(woundTypeByName(data["woundType"].get<std::string>())),
	bodyPart(bp), hit(data["hit"]), bleedVolumeRate(data["bleedVolumeRate"].get<int32_t>()),
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
	data["bodyPartType"] = bodyPartType;
	data["materialType"] = materialType;
	data["severed"] = severed;
	data["wounds"] = Json::array();
	for(const Wound& wound : wounds)
		data["wounds"].push_back(wound.toJson());
	return data;
}
Body::Body(Area& area, const ActorIndex& a) : m_bleedEvent(area.m_eventSchedule), m_woundsCloseEvent(area.m_eventSchedule), m_actor(a) { }
void Body::initalize(Area& area)
{
	AnimalSpeciesId species = area.getActors().getSpecies(m_actor);
	setMaterialType(AnimalSpecies::getMaterialType(species));
	for(BodyPartTypeId bodyPartType : BodyType::getBodyPartTypes(AnimalSpecies::getBodyType(species)))
	{
		m_bodyParts.emplace_back(bodyPartType, m_solid);
		m_totalVolume += BodyPartType::getVolume(bodyPartType);
	}
	m_volumeOfBlood = healthyBloodVolume();
}
Body::Body(const Json& data, DeserializationMemo& deserializationMemo, const ActorIndex& a) :
	m_bleedEvent(deserializationMemo.m_simulation.m_eventSchedule),
	m_woundsCloseEvent(deserializationMemo.m_simulation.m_eventSchedule),
	m_actor(a),
	m_solid(MaterialType::byName(data["materialType"].get<std::string>())),
	m_totalVolume(data["totalVolume"].get<FullDisplacement>()),
	m_impairMovePercent(data.contains("impairMovePercent") ? data["impairMovePercent"].get<Percent>() : Percent::create(0)),
	m_impairManipulationPercent(data.contains("impairManipulationPercent") ? data["impairManipulationPercent"].get<Percent>() : Percent::create(0)),
	m_volumeOfBlood(data["volumeOfBlood"].get<FullDisplacement>()),
	m_pain(data["pain"].get<PsycologyWeight>()),
	m_isBleeding(data["isBleeding"].get<bool>())
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
		{"materialType", m_solid},
		{"totalVolume", m_totalVolume},
		{"volumeOfBlood", m_volumeOfBlood},
		{"isBleeding", m_isBleeding},
		{"pain", m_pain}
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
	int32_t roll = random.getInRange(0, m_totalVolume.get());
	for(BodyPart& bodyPart : m_bodyParts)
	{
		FullDisplacement volume = BodyPartType::getVolume(bodyPart.bodyPartType);
		if(volume >= roll)
			return bodyPart;
		else
			roll -= volume.get();
	}
	std::unreachable();
	return m_bodyParts.front();
}
BodyPart& Body::pickABodyPartByType(const BodyPartTypeId& bodyPartType)
{
	for(BodyPart& bodyPart : m_bodyParts)
		if(bodyPart.bodyPartType == bodyPartType)
				return bodyPart;
	std::unreachable();
	return m_bodyParts.front();
}
// Armor has already been applied, calculate hit depth.
void Body::getHitDepth(Hit& hit, const BodyPart& bodyPart)
{
	assert(hit.depth == 0);
	if(piercesSkin(hit, bodyPart))
	{
		++hit.depth;
		int32_t hardnessDelta = std::max(1, (int32_t)MaterialType::getHardness(bodyPart.materialType) - (int32_t)MaterialType::getHardness(hit.materialType));
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
	// Run Body::getHitDepth on hit before calling this method.
	assert(hit.depth != 0);
	Actors& actors = area.getActors();
	int32_t scale = AnimalSpecies::getBodyScale(actors.getSpecies(m_actor));
	int32_t bleedVolumeRate = WoundCalculations::getBleedVolumeRate(hit, bodyPart.bodyPartType, scale);
	Wound& wound = bodyPart.wounds.emplace_back(area, m_actor, hit.woundType, bodyPart, hit, bleedVolumeRate);
	float ratio = (float)hit.area / (BodyPartType::getVolume(bodyPart.bodyPartType).get() * scale);
	if(BodyPartType::getVital(bodyPart.bodyPartType) && hit.depth == 4 && ratio >= Config::hitAreaToBodyPartVolumeRatioForFatalStrikeToVitalArea)
	{
		actors.die(m_actor, CauseOfDeath::wound);
		return wound;
	}
	else if(hit.woundType == WoundType::Cut && hit.depth == 4 && ((float)hit.area / (float)BodyPartType::getVolume(bodyPart.bodyPartType).get()) > Config::ratioOfHitAreaToBodyPartVolumeForSever)
	{
		sever(bodyPart, wound);
		if(BodyPartType::getVital(bodyPart.bodyPartType))
		{
			actors.die(m_actor, CauseOfDeath::wound);
			return wound;
		}
		else
			m_pain += Config::painWhenALimbIsSevered;
	}
	else
	{
		m_pain += hit.pain();
		Step delayTillHeal = WoundCalculations::getStepsTillHealed(hit, bodyPart.bodyPartType, scale);
		wound.healEvent.schedule(area.m_simulation, delayTillHeal, wound, *this);
	}
	recalculateBleedAndImpairment(area);
	return wound;
}
void Body::healWound(Area& area, Wound& wound)
{
	wound.bodyPart.wounds.remove(wound);
	// TODO: reduce pain gradually in stages.
	m_pain -= wound.hit.pain();
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
	int32_t bleedVolumeRate = 0;
	m_impairManipulationPercent = Percent::create(0);
	m_impairMovePercent = Percent::create(0);
	for(BodyPart& bodyPart : m_bodyParts)
	{
		for(Wound& wound : bodyPart.wounds)
		{
			bleedVolumeRate += wound.bleedVolumeRate; // when wound closes set to 0.
			if(BodyPartType::getDoesLocamotion(bodyPart.bodyPartType))
				m_impairMovePercent += wound.impairPercent();
			if(BodyPartType::getDoesManipulation(bodyPart.bodyPartType))
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
		Step baseWoundsCloseDelay = Step::create(Config::ratioWoundsCloseDelayToBleedVolume * bleedVolumeRate);
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
	int32_t outputBleedRate = 0;
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
	FullDisplacement bodyPartVolume = BodyPartType::getVolume(bodyPart.bodyPartType);
	int32_t pierceScore = (int32_t)((float)hit.force.get() / (float)hit.area) * MaterialType::getHardness(hit.materialType) * Config::pierceSkinModifier;
	int32_t defenseScore = Config::bodyHardnessModifier * bodyPartVolume.get() * MaterialType::getHardness(bodyPart.materialType);
	return pierceScore > defenseScore;
}
bool Body::piercesFat(Hit hit, const BodyPart& bodyPart) const
{
	FullDisplacement bodyPartVolume = BodyPartType::getVolume(bodyPart.bodyPartType);
	int32_t pierceScore = (int32_t)((float)hit.force.get() / (float)hit.area) * MaterialType::getHardness(hit.materialType) * Config::pierceFatModifier;
	int32_t defenseScore = Config::bodyHardnessModifier * bodyPartVolume.get() * MaterialType::getHardness(bodyPart.materialType);
	return pierceScore > defenseScore;
}
bool Body::piercesMuscle(Hit hit, const BodyPart& bodyPart) const
{
	FullDisplacement bodyPartVolume = BodyPartType::getVolume(bodyPart.bodyPartType);
	int32_t pierceScore = (int32_t)((float)hit.force.get() / (float)hit.area) * MaterialType::getHardness(hit.materialType) * Config::pierceMuscelModifier;
	int32_t defenseScore = Config::bodyHardnessModifier * bodyPartVolume.get() * MaterialType::getHardness(bodyPart.materialType);
	return pierceScore > defenseScore;
}
bool Body::piercesBone(Hit hit, const BodyPart& bodyPart) const
{
	FullDisplacement bodyPartVolume = BodyPartType::getVolume(bodyPart.bodyPartType);
	int32_t pierceScore = (int32_t)((float)hit.force.get() / (float)hit.area) * MaterialType::getHardness(hit.materialType) * Config::pierceBoneModifier;
	int32_t defenseScore = Config::bodyHardnessModifier * bodyPartVolume.get() * MaterialType::getHardness(bodyPart.materialType);
	return pierceScore > defenseScore;
}
FullDisplacement Body::healthyBloodVolume() const
{
	return m_totalVolume * Config::ratioOfTotalBodyVolumeWhichIsBlood;
}
std::vector<Attack> Body::getMeleeAttacks() const
{
	std::vector<Attack> output;
	for(const BodyPart& bodyPart : m_bodyParts)
		for(auto& [attackType, materialType] : BodyPartType::getAttackTypesAndMaterials(bodyPart.bodyPartType))
			output.emplace_back(attackType, materialType, ItemIndex::null());
	return output;
}
FullDisplacement Body::getVolume(Area& area) const
{
	return FullDisplacement::create(util::scaleByPercent(m_totalVolume.get(), area.getActors().getPercentGrown(m_actor)));
}
bool Body::isInjured() const
{
	for(const BodyPart& bodyPart : m_bodyParts)
		if(!bodyPart.wounds.empty())
			return true;
	return false;
}
bool Body::isSeriouslyInjured() const
{
	if(m_isBleeding)
	{
		float ratio = (float)m_volumeOfBlood.get() / (float)healthyBloodVolume().get();
		if(ratio < Config::maximumRatioOfBloodRemainingForSeriousInjury)
			return true;
	}
	for(const BodyPart& bodyPart : m_bodyParts)
		if(getImpairPercentFor(bodyPart) > Config::minimumImpairPercentForSeriousInjury)
			return true;
	return false;
}
bool Body::hasBodyPart(const BodyPartTypeId& bodyPartType) const
{
	for(const BodyPart& bodyPart : m_bodyParts)
		if(bodyPart.bodyPartType == bodyPartType)
			return true;
	return false;
}
Percent Body::getImpairPercentFor(const BodyPartTypeId& bodyPartType) const
{
	Percent output = Percent::create(0);
	for(const BodyPart& bodyPart : m_bodyParts)
		if(bodyPart.bodyPartType == bodyPartType)
			output += getImpairPercentFor(bodyPart);
	return output;
}
Percent Body::getImpairPercentFor(const BodyPart& bodyPart) const
{
	Percent output = Percent::create(0);
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
WoundHealEvent::WoundHealEvent(Simulation& simulation, const Step& delay, Wound& w, Body& b, const Step start) :
	ScheduledEvent(simulation, delay, start), m_wound(w), m_body(b) {}
BleedEvent::BleedEvent(Simulation& simulation, const Step& delay, Body& b, const Step start) :
	ScheduledEvent(simulation, delay, start), m_body(b) {}
WoundsCloseEvent::WoundsCloseEvent(Simulation& simulation, const Step& delay, Body& b, const Step start) :
	ScheduledEvent(simulation, delay, start), m_body(b) {}
