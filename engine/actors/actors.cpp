#include "actors.h"
#include "../animalSpecies.h"
#include "../area.h"
#include "../config.h"
#include "../deserializationMemo.h"
#include "../eventSchedule.hpp"
#include "../items/items.h"
#include "../materialType.h"
#include "../simulation.h"
#include "../simulation/hasActors.h"
#include "../types.h"
#include "../util.h"
#include "../objectives/wait.h"
#include "../itemType.h"
#include "../sleep.h"
#include "../drink.h"
#include "../eat.h"
#include "../actors/grow.h"
#include "attributes.h"
#include "objective.h"
#include "temperature.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
Percent ActorParamaters::getPercentGrown(Simulation& simulation)
{
	if(percentGrown == nullPercent)
	{
		Percent percentLifeTime = simulation.m_random.getInRange(0, 100);
		// Don't generate actors in the last 15% of their possible life span, they don't get out much.
		Step adjustedMax = util::scaleByPercent(species.deathAgeSteps[0], 85);
		// Using util::scaleByPercent and util::fractionToPercent give the wrong result here for some reason.
		Step ageSteps = ((float)adjustedMax / (float)percentLifeTime) * 100.f;
		percentGrown = std::min(100, (Percent)(((float)ageSteps / (float)species.stepsTillFullyGrown) * 100.f));
		birthStep = simulation.m_step - ageSteps;
	}
	return percentGrown;
}
std::wstring ActorParamaters::getName(Simulation& simulation)
{
	if(name.empty())
		name = util::stringToWideString(species.name) + std::to_wstring(getId(simulation));
	return name;
}
Step ActorParamaters::getBirthStep(Simulation& simulation)
{
	if(!birthStep)
	{
		if(percentGrown == nullPercent)
			getPercentGrown(simulation);
		else
		{
			// Pick an age for the given percent grown.
			Step age =  percentGrown < 95 ?
				util::scaleByPercent(species.stepsTillFullyGrown, percentGrown) :
				simulation.m_random.getInRange(species.stepsTillFullyGrown, species.deathAgeSteps[1]);
			birthStep = simulation.m_step - age;
		}
	}
	return birthStep;
}
ActorId ActorParamaters::getId(Simulation& simulation)
{
	if(!id)
		id = simulation.m_actors.getNextId();
	return id;
}
Percent ActorParamaters::getPercentThirst(Simulation& simulation)
{
	if(percentThirst == nullPercent)
	{
		percentThirst = simulation.m_random.getInRange(0, 100);
		needsDrink = simulation.m_random.percentChance(10);
	}
	return percentThirst;
}
Percent ActorParamaters::getPercentHunger(Simulation& simulation)
{
	if(percentHunger == nullPercent)
	{
		percentHunger = simulation.m_random.getInRange(0, 100);
		needsEat = simulation.m_random.percentChance(10);
	}
	return percentHunger;
}
Percent ActorParamaters::getPercentTired(Simulation& simulation)
{
	if(percentTired == nullPercent)
	{
		percentTired = simulation.m_random.getInRange(0, 100);
		needsSleep = simulation.m_random.percentChance(10);
	}
	return percentTired;
}
void ActorParamaters::generateEquipment(Area& area, ActorIndex actor)
{
	static const MaterialType& leather = MaterialType::byName("leather");
	static const MaterialType& cotton = MaterialType::byName("cotton");
	static const MaterialType& iron = MaterialType::byName("iron");
	static const MaterialType& bronze = MaterialType::byName("bronze");
	static const MaterialType& poplarWoodType = MaterialType::byName("poplar wood");
	Actors& actors = area.getActors();
	if(!actors.isSentient(actor))
		return;
	auto& random = area.m_simulation.m_random;
	auto generate = [&](const ItemType& itemType, const MaterialType& materialType){
		uint quality = random.getInRange(10, 50);
		Percent wear = random.getInRange(10, 60);
		ItemIndex item = area.getItems().create({
			.itemType=itemType,
			.materialType=materialType,
			.quality=quality,
			.percentWear=wear
		});
		actors.equipment_add(actor, item);
	};
	auto generateMetal = [&](const ItemType& itemType) {
		const MaterialType* materialType = random.chance(0.8) ? &iron : &bronze;
		generate(itemType, *materialType);
	};
	if(hasCloths)
	{
		//TODO: Cultural differences.
		static const ItemType& pantsType = ItemType::byName("pants");
		generate(pantsType, cotton);
		static const ItemType& shirtType = ItemType::byName("shirt");
		generate(shirtType, cotton);
		static const ItemType& jacketType = ItemType::byName("jacket");
		generate(jacketType, cotton);
		static const ItemType& shoesType = ItemType::byName("shoes");
		generate(shoesType, cotton);
		bool hasBelt = random.chance(0.6);
		if(hasBelt)
		{
			static const ItemType& beltType = ItemType::byName("belt");
			generate(beltType, leather);
		}
	}
	static const ItemType& halfHelmType = ItemType::byName("half helm");
	static const ItemType& breastPlateType = ItemType::byName("breast plate");
	if(hasLightArmor && !hasHeavyArmor)
	{
		if(random.chance(0.8))
		{
			generateMetal(halfHelmType);
		}
		else if(random.chance(0.5))
		{
			static const ItemType& hoodType = ItemType::byName("hood");
			generate(hoodType, leather);
		}
		if(random.chance(0.4))
		{
			generateMetal(breastPlateType);
		}
		else if(random.chance(0.6))
		{
			static const ItemType& chainMailShirtType = ItemType::byName("chain mail shirt");
			generateMetal(chainMailShirtType);
		}
	}
	if(hasHeavyArmor)
	{
		if(random.chance(0.75))
		{
			static const ItemType& fullHelmType = ItemType::byName("full helm");
			generateMetal(fullHelmType);
		}
		else if(random.chance(0.9))
		{
			generateMetal(halfHelmType);
		}
		generateMetal(breastPlateType);
		static const ItemType& greavesType = ItemType::byName("greaves");
		generateMetal(greavesType);
		static const ItemType& vambracesType = ItemType::byName("vambraces");
		generateMetal(vambracesType);
	}
	static const ItemType& longSwordType = ItemType::byName("long sword");
	if(hasSidearm)
	{
		auto roll = random.getInRange(0, 100);
		if(roll > 95)
		{
			generateMetal(longSwordType);
		}
		if(roll > 70)
		{
			static const ItemType& shortSwordType = ItemType::byName("short sword");
			generateMetal(shortSwordType);
		}
		else
		{
			static const ItemType& daggerType = ItemType::byName("dagger");
			generateMetal(daggerType);
		}
	}
	if(hasLongarm)
	{
		auto roll = random.getInRange(0, 100);
		if(roll > 75)
		{
			static const ItemType& spearType = ItemType::byName("spear");
			generateMetal(spearType);
		}
		if(roll > 65)
		{
			static const ItemType& maceType = ItemType::byName("mace");
			generateMetal(maceType);
		}
		if(roll > 45)
		{
			static const ItemType& glaveType = ItemType::byName("glave");
			generateMetal(glaveType);
		}
		if(roll > 25)
		{
			static const ItemType& axeType = ItemType::byName("axe");
			generateMetal(axeType);
		}
		else
		{
			generateMetal(longSwordType);
			static const ItemType& shieldType = ItemType::byName("shield");
			generate(shieldType, poplarWoodType);
		}
	}
	if(hasRangedWeapon)
	{
		auto roll = random.getInRange(0, 100);
		if(roll > 75)
		{
			static const ItemType& crossbowType = ItemType::byName("crossbow");
			generate(crossbowType, poplarWoodType);
		}
		else
		{
			static const ItemType& shortbowType = ItemType::byName("shortbow");
			generate(shortbowType, poplarWoodType);
		}
	}
}
template<class T>
void to_json(const Json& data, std::unique_ptr<T>& t) { data = *t; }
Actors::Actors(Area& area) :
	Portables(area),
	m_coolDownEvent(area.m_eventSchedule),
	m_moveEvent(area.m_eventSchedule)
{ }
void Actors::load(const Json& data)
{
	Portables::load(data);
	data["id"].get_to(m_id);
	data["name"].get_to(m_name);
	data["species"].get_to(m_species);
	// No need to serialize m_project.
	data["birthStep"].get_to(m_birthStep);
	data["causeOfDeath"].get_to(m_causeOfDeath);
	data["unencomberedCarryMass"].get_to(m_unencomberedCarryMass);
	data["carrying"].get_to(m_carrying);
	data["stamina"].get_to(m_stamina);
	data["canSee"].get_to(m_canSee);
	data["visionRange"].get_to(m_visionRange);
	m_coolDownEvent.load(m_area.m_simulation, data["coolDownEvent"]);
	// No need to serialize path requests?
	data["meleeAttackTable"].get_to(m_meleeAttackTable);
	data["targetedBy"].get_to(m_targetedBy);
	data["target"].get_to(m_target);
	data["onMissCoolDownMelee"].get_to(m_onMissCoolDownMelee);
	data["maxMeleeRange"].get_to(m_maxMeleeRange);
	data["maxRange"].get_to(m_maxRange);
	data["coolDownDurationModifier"].get_to(m_coolDownDurationModifier);
	data["combatScore"].get_to(m_combatScore);
	m_moveEvent.load(m_area.m_simulation, data["moveEvent"]);
	data["path"].get_to(m_path);
	data["destination"].get_to(m_destination);
	data["speedIndividual"].get_to(m_speedIndividual);
	data["speedActual"].get_to(m_speedActual);
	data["moveRetries"].get_to(m_moveRetries);
	auto& deserializationMemo = m_area.m_simulation.getDeserializationMemo();
	m_attributes.resize(m_id.size());
	for(const Json& pair : data["attributes"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_attributes.at(index) = std::make_unique<Attributes>(pair[1], *m_species.at(index), getPercentGrown(index));
	}
	m_hasObjectives.resize(m_id.size());
	for(const Json& pair : data["hasObjectives"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_hasObjectives.at(index) = std::make_unique<HasObjectives>(index);
		m_hasObjectives.at(index)->load(pair[1], deserializationMemo);
	}
	m_skillSet.resize(m_id.size());
	for(const Json& pair : data["skillSet"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_skillSet.at(index) = std::make_unique<SkillSet>();
		m_skillSet.at(index)->load(pair[1]);
	}
	m_body.resize(m_id.size());
	for(const Json& pair : data["body"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_body.at(index) = std::make_unique<Body>(pair[1], deserializationMemo, index);
	}
	m_mustSleep.resize(m_id.size());
	for(const Json& pair : data["mustSleep"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_mustSleep.at(index) = std::make_unique<MustSleep>(m_area, pair[1], index, *m_species.at(index));
	}
	m_mustDrink.resize(m_id.size());
	for(const Json& pair : data["mustDrink"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_mustDrink.at(index) = std::make_unique<MustDrink>(m_area, pair[1], index, *m_species.at(index));
	}
	m_mustEat.resize(m_id.size());
	for(const Json& pair : data["mustEat"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_mustEat.at(index) = std::make_unique<MustEat>(m_area, pair[1], index, *m_species.at(index));
	}
	m_needsSafeTemperature.resize(m_id.size());
	for(const Json& pair : data["needsSafeTemperature"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_needsSafeTemperature.at(index) = std::make_unique<ActorNeedsSafeTemperature>(pair[1], index, m_area);
	}
	m_canGrow.resize(m_id.size());
	for(const Json& pair : data["canGrow"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_canGrow.at(index) = std::make_unique<CanGrow>(m_area, pair[1], index);
	}
	m_equipmentSet.resize(m_id.size());
	for(const Json& pair : data["equipmentSet"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_equipmentSet.at(index) = std::make_unique<EquipmentSet>(m_area, pair[1]);
	}
	m_canReserve.resize(m_id.size());
	for(const Json& pair : data["canReserve"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_canReserve.at(index) = std::make_unique<CanReserve>(m_faction.at(index));
		m_canReserve.at(index)->load(pair[1], deserializationMemo);
	}
	m_hasUniform.resize(m_id.size());
	for(const Json& pair : data["hasUniform"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_hasUniform.at(index) = std::make_unique<ActorHasUniform>();
		m_hasUniform.at(index)->load(m_area, pair[1]);
	}
	m_pathIter.resize(m_id.size());
	for(const Json& pair : data["pathIter"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_pathIter.at(index) = m_path.at(index).begin() + pair[1].get<int>();
	}
}
void to_json(Json& data, const std::unique_ptr<CanReserve>& canReserve) { data = canReserve->toJson(); }
void to_json(Json& data, const std::unique_ptr<ActorHasUniform>& uniform) { data = *uniform; }
void to_json(Json& data, const std::unique_ptr<EquipmentSet>& equipmentSet) { data = *equipmentSet; }
void to_json(Json& data, const std::unique_ptr<Attributes>& attributes) { data = attributes->toJson(); }
void to_json(Json& data, const std::unique_ptr<HasObjectives>& hasObjectives) { data = hasObjectives->toJson(); }
void to_json(Json& data, const std::unique_ptr<Body>& body) { data = body->toJson(); }
void to_json(Json& data, const std::unique_ptr<MustSleep>& mustSleep) { data = mustSleep->toJson(); }
void to_json(Json& data, const std::unique_ptr<MustEat>& mustEat) { data = mustEat->toJson(); }
void to_json(Json& data, const std::unique_ptr<MustDrink>& mustDrink) { data = mustDrink->toJson(); }
void to_json(Json& data, const std::unique_ptr<ActorNeedsSafeTemperature>& actorNeedsSafeTemperature) { data = actorNeedsSafeTemperature->toJson(); }
void to_json(Json& data, const std::unique_ptr<CanGrow>& canGrow) { data = canGrow->toJson(); }
void to_json(Json& data, const std::unique_ptr<SkillSet>& skillSet) { data = skillSet->toJson(); }
Json Actors::toJson() const 
{
	Json output{
		{"canReserve", m_canReserve},
		{"hasUniform", m_hasUniform},
		{"equipmentSet", m_equipmentSet},
		{"id", m_id},
		{"name", m_name},
		{"species", m_species},
		{"birthStep", m_birthStep},
		{"causeOfDeath", m_causeOfDeath},
		{"unencomberedCarryMass", m_unencomberedCarryMass},
		{"attributes", m_attributes},
		{"hasObjectives", m_hasObjectives},
		{"body", m_body},
		{"mustSleep", m_mustSleep},
		{"mustEat", m_mustSleep},
		{"mustDrink", m_mustDrink},
		{"needsSafeTemperature", m_needsSafeTemperature},
		{"canGrow", m_canGrow},
		{"skillSet", m_skillSet},
		{"carrying", m_carrying},
		{"stamina", m_stamina},
		{"canSee", m_canSee},
		{"visionRange", m_visionRange},
		{"coolDownEvent", m_coolDownEvent},
		{"targetedBy", m_targetedBy},
		{"target", m_target},
		{"onMissCoolDownMelee", m_onMissCoolDownMelee},
		{"maxMeleeRange", m_maxMeleeRange},
		{"maxRange", m_maxRange},
		{"coolDownDurationModifier", m_coolDownDurationModifier},
		{"combatScore", m_combatScore},
		{"moveEvent", m_moveEvent},
		{"path", m_path},
		{"pathIter", Json::array()},
		{"destination", m_destination},
		{"speedIndividual", m_speedIndividual},
		{"speedActual", m_speedActual},
		{"moveRetries", m_moveRetries}
	};
	for(ActorIndex index : getAll())
		output["pathIter"].push_back(m_pathIter.at(index) - m_path.at(index).begin());
	return output;
}
void Actors::resize(HasShapeIndex newSize)
{
	m_referenceTarget.resize(newSize);
	m_canReserve.resize(newSize);
	m_hasUniform.resize(newSize);
	m_equipmentSet.resize(newSize);
	m_id.resize(newSize);
	m_name.resize(newSize);
	m_species.resize(newSize);
	m_project.resize(newSize);
	m_birthStep.resize(newSize);
	m_causeOfDeath.resize(newSize);
	m_unencomberedCarryMass.resize(newSize);
	m_attributes.resize(newSize);
	m_hasObjectives.resize(newSize);
	m_body.resize(newSize);
	m_mustSleep.resize(newSize);
	m_mustDrink.resize(newSize);
	m_canGrow.resize(newSize);
	m_needsSafeTemperature.resize(newSize);
	m_skillSet.resize(newSize);
	m_carrying.resize(newSize);
	m_stamina.resize(newSize);
	m_canSee.resize(newSize);
	m_visionRange.resize(newSize);
	m_hasVisionFacade.resize(newSize);
	m_coolDownEvent.resize(newSize);
	m_getIntoAttackPositionPathRequest.resize(newSize);
	m_meleeAttackTable.resize(newSize);
	m_targetedBy.resize(newSize);
	m_target.resize(newSize);
	m_onMissCoolDownMelee.resize(newSize);
	m_maxMeleeRange.resize(newSize);
	m_maxRange.resize(newSize);
	m_coolDownDurationModifier.resize(newSize);
	m_combatScore.resize(newSize);
	m_moveEvent.resize(newSize);
	m_pathRequest.resize(newSize);
	m_path.resize(newSize);
	m_pathIter.resize(newSize);
	m_destination.resize(newSize);
	m_speedIndividual.resize(newSize);
	m_speedActual.resize(newSize);
	m_moveRetries.resize(newSize);
}
void Actors::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	m_referenceTarget.at(newIndex) = std::move(m_referenceTarget.at(oldIndex));
	m_canReserve.at(newIndex) = std::move(m_canReserve.at(oldIndex));
	m_hasUniform.at(newIndex) = std::move(m_hasUniform.at(oldIndex));
	m_equipmentSet.at(newIndex) = std::move(m_equipmentSet.at(oldIndex));
	m_id.at(newIndex) = m_id.at(oldIndex);
	m_name.at(newIndex) = m_name.at(oldIndex);
	m_species.at(newIndex) = m_species.at(oldIndex);
	m_project.at(newIndex) = m_project.at(oldIndex);
	m_birthStep.at(newIndex) = m_birthStep.at(oldIndex);
	m_causeOfDeath.at(newIndex) = m_causeOfDeath.at(oldIndex);
	m_unencomberedCarryMass.at(newIndex) = m_unencomberedCarryMass.at(oldIndex);
	m_attributes.at(newIndex) = std::move(m_attributes.at(oldIndex));
	m_hasObjectives.at(newIndex) = std::move(m_hasObjectives.at(oldIndex));
	m_body.at(newIndex) = std::move(m_body.at(oldIndex));
	m_mustSleep.at(newIndex) = std::move(m_mustSleep.at(oldIndex));
	m_mustDrink.at(newIndex) = std::move(m_mustDrink.at(oldIndex));
	m_canGrow.at(newIndex) = std::move(m_canGrow.at(oldIndex));
	m_needsSafeTemperature.at(newIndex) = std::move(m_needsSafeTemperature.at(oldIndex));
	m_skillSet.at(newIndex) = std::move(m_skillSet.at(oldIndex));
	m_carrying.at(newIndex) = m_carrying.at(oldIndex);
	m_stamina.at(newIndex) = m_stamina.at(oldIndex);
	m_canSee.at(newIndex) = m_canSee.at(oldIndex);
	m_visionRange.at(newIndex) = m_visionRange.at(oldIndex);
	m_hasVisionFacade.at(newIndex) = m_hasVisionFacade.at(oldIndex);
	m_coolDownEvent.moveIndex(oldIndex, newIndex);
	m_getIntoAttackPositionPathRequest.at(newIndex) = std::move(m_getIntoAttackPositionPathRequest.at(oldIndex));
	m_meleeAttackTable.at(newIndex) = m_meleeAttackTable.at(oldIndex);
	m_targetedBy.at(newIndex) = m_targetedBy.at(oldIndex);
	m_target.at(newIndex) = m_target.at(oldIndex);
	m_onMissCoolDownMelee.at(newIndex) = m_onMissCoolDownMelee.at(oldIndex);
	m_maxMeleeRange.at(newIndex) = m_maxMeleeRange.at(oldIndex);
	m_maxRange.at(newIndex) = m_maxRange.at(oldIndex);
	m_coolDownDurationModifier.at(newIndex) = m_coolDownDurationModifier.at(oldIndex);
	m_combatScore.at(newIndex) = m_combatScore.at(oldIndex);
	m_moveEvent.moveIndex(oldIndex, newIndex);
	m_pathRequest.at(newIndex) = std::move(m_pathRequest.at(oldIndex));
	m_path.at(newIndex) = m_path.at(oldIndex);
	m_pathIter.at(newIndex) = m_pathIter.at(oldIndex);
	m_destination.at(newIndex) = m_destination.at(oldIndex);
	m_speedIndividual.at(newIndex) = m_speedIndividual.at(oldIndex);
	m_speedActual.at(newIndex) = m_speedActual.at(oldIndex);
	m_moveRetries.at(newIndex) = m_moveRetries.at(oldIndex);
	// Update references.
	m_referenceTarget.at(newIndex)->index = newIndex;
	//TODO: EquipmentSet should only exist when it is holding something.
	m_equipmentSet.at(newIndex)->updateCarrierIndexForContents(m_area, newIndex);
	if(m_carrying.at(newIndex).exists())
	{
		ActorOrItemIndex carrying = m_carrying.at(newIndex);
		if(carrying.isActor())
			m_area.getActors().updateCarrierIndex(carrying.get(), newIndex);
		else
			m_area.getItems().updateCarrierIndex(carrying.get(), newIndex);
	}
	m_hasObjectives.at(newIndex)->updateActorIndex(newIndex);
	if(m_pathRequest.at(newIndex) != nullptr)
		m_pathRequest.at(newIndex)->updateActorIndex(newIndex);
	if(m_getIntoAttackPositionPathRequest.at(newIndex) != nullptr)
		m_getIntoAttackPositionPathRequest.at(newIndex)->updateActorIndex(newIndex);
	for(ActorIndex actor : m_targetedBy.at(newIndex))
		m_target.at(actor) = newIndex;
	if(!m_hasVisionFacade.at(newIndex).empty())
		m_hasVisionFacade.at(newIndex).updateActorIndex(newIndex);
}
void Actors::onChangeAmbiantSurfaceTemperature()
{
	for(ActorIndex index : m_onSurface)
		m_needsSafeTemperature.at(index)->onChange(m_area);
}
ActorIndex Actors::create(ActorParamaters params)
{
	ActorIndex index = getNextIndex();
	bool isStatic = false;
	Portables::create(index, params.species.moveType, params.species.shapeForPercentGrown(params.getPercentGrown(m_area.m_simulation)), params.location, params.facing, isStatic);
	Simulation& s = m_area.m_simulation;
	m_referenceTarget.at(index)->index = index;
	m_id.at(index) = params.getId(s);
	m_name.at(index) = params.getName(s);
	m_species.at(index) = &params.species;
	m_project.at(index) = nullptr;
	m_birthStep.at(index) = params.getBirthStep(s);
	m_causeOfDeath.at(index) = CauseOfDeath::none;
	m_unencomberedCarryMass.at(index) = 0;
	m_attributes.at(index) = std::make_unique<Attributes>(params.species, params.getPercentGrown(s));
	m_hasObjectives.at(index) = std::make_unique<HasObjectives>(index);
	m_body.at(index) = std::make_unique<Body>(m_area, index);
	m_mustSleep.at(index) = std::make_unique<MustSleep>(m_area, index);
	m_mustDrink.at(index) = std::make_unique<MustDrink>(m_area, index);
	m_canGrow.at(index) = std::make_unique<CanGrow>(m_area, index, params.getPercentGrown(s));
	m_needsSafeTemperature.at(index) = std::make_unique<ActorNeedsSafeTemperature>(m_area, index);
	m_skillSet.at(index) = std::make_unique<SkillSet>();
	// CanPickUp.
	m_carrying.at(index).clear();
	// Stamina.
	m_stamina.at(index) = 0;
	// Vision.
	assert(m_canSee.at(index).empty());
	m_visionRange.at(index) = params.species.visionRange;
	assert(m_hasVisionFacade.at(index).empty());
	// Combat.
	assert(!m_coolDownEvent.exists(index));
	assert(m_meleeAttackTable.at(index).empty());
	assert(m_targetedBy.at(index).empty());
	m_target.at(index) = ACTOR_INDEX_MAX;
	m_onMissCoolDownMelee.at(index) = 0;
	m_maxMeleeRange.at(index) = 0;
	m_maxRange.at(index) = 0;
	m_coolDownDurationModifier.at(index) = 0;
	m_combatScore.at(index) = 0;
	// Move.
	assert(!m_moveEvent.exists(index));
	assert(m_pathRequest.at(index) == nullptr);
	assert(m_path.at(index).empty());
	m_destination.at(index) = BLOCK_INDEX_MAX;
	m_speedIndividual.at(index) = 0;
	m_speedActual.at(index) = 0;
	m_moveRetries.at(index) = 0;
	s.m_actors.registerActor(m_id.at(index), *this, index);
	sharedConstructor(index);
	scheduleNeeds(index);
	if(isSentient(index))
		params.generateEquipment(m_area, index);
	if(params.location != BLOCK_INDEX_MAX)
		setLocationAndFacing(index, params.location, params.facing);
	return index;
}
void Actors::sharedConstructor(ActorIndex index)
{
	combat_update(index);
	move_updateIndividualSpeed(index);
	m_body.at(index)->initalize(m_area);
	m_mustDrink.at(index)->setFluidType(m_species.at(index)->fluidType);
}
void Actors::scheduleNeeds(ActorIndex index)
{
	m_mustSleep.at(index)->scheduleTiredEvent(m_area);
	m_mustDrink.at(index)->scheduleDrinkEvent(m_area);
	m_mustEat.at(index)->scheduleHungerEvent(m_area);
	m_canGrow.at(index)->updateGrowingStatus(m_area);
}
void Actors::setLocation(ActorIndex index, BlockIndex block)
{
	assert(m_location.at(index) != BLOCK_INDEX_MAX);
	Facing facing = m_area.getBlocks().facingToSetWhenEnteringFrom(block, m_location.at(index));
	setLocationAndFacing(index, block, facing);
}
void Actors::setLocationAndFacing(ActorIndex index, BlockIndex block, Facing facing)
{
	if(m_location.at(index) != BLOCK_INDEX_MAX)
		exit(index);
	Blocks& blocks = m_area.getBlocks();
	for(auto [x, y, z, v] : m_shape.at(index)->makeOccupiedPositionsWithFacing(facing))
	{
		BlockIndex occupied = blocks.offset(block, x, y, z);
		blocks.actor_record(occupied, index, v);
	}
}
void Actors::exit(ActorIndex index)
{
	assert(m_location.at(index) != BLOCK_INDEX_MAX);
	BlockIndex location = m_location.at(index);
	auto& blocks = m_area.getBlocks();
	for(auto [x, y, z, v] : m_shape.at(index)->makeOccupiedPositionsWithFacing(m_facing.at(index)))
	{
		BlockIndex occupied = blocks.offset(location, x, y, z);
		blocks.actor_erase(occupied, index);
	}
	m_location.at(index) = BLOCK_INDEX_MAX;
}
void Actors::resetNeeds(ActorIndex index)
{
	m_mustSleep.at(index)->notTired(m_area);
	m_mustDrink.at(index)->notThirsty(m_area);
	m_mustEat.at(index)->notHungry(m_area);
}
void Actors::removeMassFromCorpse(ActorIndex index, Mass mass)
{
	assert(!isAlive(index));
	assert(mass <= m_attributes.at(index)->getMass());
	m_attributes.at(index)->removeMass(mass);
}
bool Actors::isEnemy(ActorIndex index, ActorIndex other) const { return getFaction(index)->enemies.contains(m_faction.at(other)); }
bool Actors::isAlly(ActorIndex index, ActorIndex other) const { return getFaction(index)->allies.contains(m_faction.at(other)); }
void Actors::die(ActorIndex index, CauseOfDeath causeOfDeath)
{
	m_causeOfDeath.at(index) = causeOfDeath;
	combat_onDeath(index);
	move_onDeath(index);
	m_mustDrink.at(index)->onDeath();
	m_mustEat.at(index)->onDeath();
	m_mustSleep.at(index)->onDeath();
	if(m_project.at(index) != nullptr)
		m_project.at(index)->removeWorker(index);
	if(m_location.at(index) != BLOCK_INDEX_MAX)
		setStatic(index, true);
	m_area.m_visionFacadeBuckets.remove(index);
}
void Actors::passout(ActorIndex, Step)
{
	//TODO
}
void Actors::leaveArea(ActorIndex index)
{
	combat_onLeaveArea(index);
	move_onLeaveArea(index);
	exit(index);
}
void Actors::wait(ActorIndex index, Step duration)
{
	m_hasObjectives.at(index)->addTaskToStart(m_area, std::make_unique<WaitObjective>(m_area, duration));
}
void Actors::takeHit(ActorIndex index, Hit& hit, BodyPart& bodyPart)
{
	m_equipmentSet.at(index)->modifyImpact(m_area, hit, bodyPart.bodyPartType);
	m_body.at(index)->getHitDepth(hit, bodyPart);
	if(hit.depth != 0)
		m_body.at(index)->addWound(m_area, bodyPart, hit);
}
void Actors::setFaction(ActorIndex index, FactionId faction)
{
	m_faction.at(index) = faction;
	if(faction == FACTION_ID_MAX)
		m_canReserve.at(index) = nullptr;
	else
		m_canReserve.at(index)->setFaction(faction);
}
Mass Actors::getMass(ActorIndex index) const
{
	return m_attributes.at(index)->getMass() + m_equipmentSet.at(index)->getMass() + canPickUp_getMass(index);
}
Volume Actors::getVolume(ActorIndex index) const
{
	return m_body.at(index)->getVolume(m_area);
}
Quantity Actors::getAgeInYears(ActorIndex index) const
{
	DateTime now(m_area.m_simulation.m_step);
	DateTime birthDate(m_birthStep.at(index));
	Quantity differenceYears = now.year - birthDate.year;
	if(now.day > birthDate.day)
		++differenceYears;
	return differenceYears;
}
Step Actors::getAge(ActorIndex index) const
{
	return m_area.m_simulation.m_step - m_birthStep.at(index);
}
std::wstring Actors::getActionDescription(ActorIndex index) const
{
	if(m_hasObjectives.at(index)->hasCurrent())
		return util::stringToWideString(const_cast<HasObjectives&>(*m_hasObjectives.at(index).get()).getCurrent().name());
	return L"no action";
}
void Actors::reserveAllBlocksAtLocationAndFacing(ActorIndex index, const BlockIndex location, Facing facing)
{
	if(m_faction.at(index) == FACTION_ID_MAX)
		return;
	for(BlockIndex occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		m_area.getBlocks().reserve(occupied, *m_canReserve.at(index));
}
void Actors::unreserveAllBlocksAtLocationAndFacing(ActorIndex index, const BlockIndex location, Facing facing)
{
	if(m_faction.at(index) == FACTION_ID_MAX)
		return;
	for(BlockIndex occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		m_area.getBlocks().unreserve(occupied, *m_canReserve.at(index));
}
void Actors::setBirthStep(ActorIndex index, Step step)
{
	m_birthStep.at(index) = step;
	m_canGrow.at(index)->updateGrowingStatus(m_area);
}
Percent Actors::getPercentGrown(ActorIndex index) const { return m_canGrow.at(index)->growthPercent(); }
void Actors::log(ActorIndex index) const
{
	std::wcout << m_name.at(index);
	std::cout << "(" << m_species.at(index)->name << ")";
	Portables::log(index);
	std::cout << std::endl;
}
void Actors::satisfyNeeds(ActorIndex index)
{
	// Wake up if asleep.
	if(!m_mustSleep.at(index)->isAwake())
		m_mustSleep.at(index)->wakeUp(m_area);
	else
		m_mustSleep.at(index)->notTired(m_area);
	// Discard drink objective if exists.
	if(m_mustDrink.at(index)->getVolumeFluidRequested() != 0)
		m_mustDrink.at(index)->drink(m_area, m_mustDrink.at(index)->getVolumeFluidRequested());
	// Discard eat objective if exists.
	if(m_mustEat.at(index)->getMassFoodRequested() != 0)
		m_mustEat.at(index)->eat(m_area, m_mustEat.at(index)->getMassFoodRequested());
}
// Sleep.
void Actors::sleep_do(ActorIndex index) { m_mustSleep.at(index)->sleep(m_area); }
void Actors::sleep_wakeUp(ActorIndex index){ m_mustSleep.at(index)->wakeUp(m_area); }
void Actors::sleep_wakeUpEarly(ActorIndex index){ m_mustSleep.at(index)->wakeUpEarly(m_area); }
void Actors::sleep_setSpot(ActorIndex index, BlockIndex location) { m_mustSleep.at(index)->setLocation(location); }
void Actors::sleep_makeTired(ActorIndex index) { m_mustSleep.at(index)->tired(m_area); }
bool Actors::sleep_isAwake(ActorIndex index) const { return m_mustSleep.at(index)->isAwake(); }
Percent Actors::sleep_getPercentDoneSleeping(ActorIndex index) const { return m_mustSleep.at(index)->getSleepPercent(); }
Percent Actors::sleep_getPercentTired(ActorIndex index) const { return m_mustSleep.at(index)->getTiredPercent(); }
BlockIndex Actors::sleep_getSpot(ActorIndex index) const { return m_mustSleep.at(index)->getLocation(); }
bool Actors::sleep_hasTiredEvent(ActorIndex index) const { return m_mustSleep.at(index)->hasTiredEvent(); }
// Attributes.
uint32_t Actors::getStrength(ActorIndex index) const { return m_attributes.at(index)->getStrength(); }
uint32_t Actors::getDextarity(ActorIndex index) const { return m_attributes.at(index)->getDextarity(); }
uint32_t Actors::getAgility(ActorIndex index) const { return m_attributes.at(index)->getAgility(); }
// Skills.
uint32_t Actors::skill_getLevel(ActorIndex index, const SkillType& skillType) const { return m_skillSet.at(index)->get(skillType); }
// CoolDownEvent.
void AttackCoolDownEvent::execute(Simulation&, Area* area) { area->getActors().combat_coolDownCompleted(m_actor); }
void AttackCoolDownEvent::clearReferences(Simulation&, Area* area) { area->getActors().m_coolDownEvent.clearPointer(m_actor); }
