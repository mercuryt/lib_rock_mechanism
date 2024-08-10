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
#include "eventSchedule.h"
#include "index.h"
#include "objective.h"
#include "temperature.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
Percent ActorParamaters::getPercentGrown(Simulation& simulation)
{
	if(percentGrown.empty())
	{
		Percent percentLifeTime = Percent::create(simulation.m_random.getInRange(0, 100));
		// Don't generate actors in the last 15% of their possible life span, they don't get out much.
		Step adjustedMax = Step::create(util::scaleByPercent(AnimalSpecies::getDeathAgeSteps(species)[0].get(), Percent::create(85)));
		// Using util::scaleByPercent and util::fractionToPercent give the wrong result here for some reason.
		Step ageSteps = Step::create(((float)adjustedMax.get() / (float)percentLifeTime.get()) * 100.f);
		percentGrown = Percent::create(std::min(100u, (uint)(((float)ageSteps.get() / (float)AnimalSpecies::getStepsTillFullyGrown(species).get()) * 100.f)));
		birthStep = simulation.m_step - ageSteps;
	}
	return percentGrown;
}
std::wstring ActorParamaters::getName(Simulation& simulation)
{
	if(name.empty())
		name = util::stringToWideString(AnimalSpecies::getName(species)) + std::to_wstring(getId(simulation).get());
	return name;
}
Step ActorParamaters::getBirthStep(Simulation& simulation)
{
	if(birthStep.empty())
	{
		if(percentGrown.empty())
			getPercentGrown(simulation);
		else
		{
			// Pick an age for the given percent grown.
			Step grown = AnimalSpecies::getStepsTillFullyGrown(species);
			Step age = Step::create(percentGrown < 95 ?
				util::scaleByPercent(grown.get(), percentGrown) :
				simulation.m_random.getInRange(grown.get(), AnimalSpecies::getDeathAgeSteps(species)[1].get())
			);
			birthStep = simulation.m_step - age;
		}
	}
	return birthStep;
}
ActorId ActorParamaters::getId(Simulation& simulation)
{
	if(id.empty())
		id = simulation.m_actors.getNextId();
	return id;
}
Percent ActorParamaters::getPercentThirst(Simulation& simulation)
{
	if(percentThirst.empty())
	{
		percentThirst = Percent::create(simulation.m_random.getInRange(0, 100));
		needsDrink = simulation.m_random.percentChance(Percent::create(10));
	}
	return percentThirst;
}
Percent ActorParamaters::getPercentHunger(Simulation& simulation)
{
	if(percentHunger.empty())
	{
		percentHunger = Percent::create(simulation.m_random.getInRange(0, 100));
		needsEat = simulation.m_random.percentChance(Percent::create(10));
	}
	return percentHunger;
}
Percent ActorParamaters::getPercentTired(Simulation& simulation)
{
	if(percentTired.empty())
	{
		percentTired = Percent::create(simulation.m_random.getInRange(0, 100));
		needsSleep = simulation.m_random.percentChance(Percent::create(10));
	}
	return percentTired;
}
void ActorParamaters::generateEquipment(Area& area, ActorIndex actor)
{
	static MaterialTypeId leather = MaterialType::byName("leather");
	static MaterialTypeId cotton = MaterialType::byName("cotton");
	static MaterialTypeId iron = MaterialType::byName("iron");
	static MaterialTypeId bronze = MaterialType::byName("bronze");
	static MaterialTypeId poplarWoodType = MaterialType::byName("poplar wood");
	Actors& actors = area.getActors();
	if(!actors.isSentient(actor))
		return;
	auto& random = area.m_simulation.m_random;
	auto generate = [&](ItemTypeId itemType, MaterialTypeId materialType){
		Quality quality = Quality::create(random.getInRange(10, 50));
		Percent wear = Percent::create(random.getInRange(10, 60));
		ItemIndex item = area.getItems().create({
			.itemType=itemType,
			.materialType=materialType,
			.quality=quality,
			.percentWear=wear
		});
		actors.equipment_add(actor, item);
	};
	auto generateMetal = [&](ItemTypeId itemType) {
		MaterialTypeId materialType = random.chance(0.8) ? iron : bronze;
		generate(itemType, materialType);
	};
	if(hasCloths)
	{
		//TODO: Cultural differences.
		static ItemTypeId pantsType = ItemType::byName("pants");
		generate(pantsType, cotton);
		static ItemTypeId shirtType = ItemType::byName("shirt");
		generate(shirtType, cotton);
		static ItemTypeId jacketType = ItemType::byName("jacket");
		generate(jacketType, cotton);
		static ItemTypeId shoesType = ItemType::byName("shoes");
		generate(shoesType, cotton);
		bool hasBelt = random.chance(0.6);
		if(hasBelt)
		{
			static ItemTypeId beltType = ItemType::byName("belt");
			generate(beltType, leather);
		}
	}
	static ItemTypeId halfHelmType = ItemType::byName("half helm");
	static ItemTypeId breastPlateType = ItemType::byName("breast plate");
	if(hasLightArmor && !hasHeavyArmor)
	{
		if(random.chance(0.8))
		{
			generateMetal(halfHelmType);
		}
		else if(random.chance(0.5))
		{
			static ItemTypeId hoodType = ItemType::byName("hood");
			generate(hoodType, leather);
		}
		if(random.chance(0.4))
		{
			generateMetal(breastPlateType);
		}
		else if(random.chance(0.6))
		{
			static ItemTypeId chainMailShirtType = ItemType::byName("chain mail shirt");
			generateMetal(chainMailShirtType);
		}
	}
	if(hasHeavyArmor)
	{
		if(random.chance(0.75))
		{
			static ItemTypeId fullHelmType = ItemType::byName("full helm");
			generateMetal(fullHelmType);
		}
		else if(random.chance(0.9))
		{
			generateMetal(halfHelmType);
		}
		generateMetal(breastPlateType);
		static ItemTypeId greavesType = ItemType::byName("greaves");
		generateMetal(greavesType);
		static ItemTypeId vambracesType = ItemType::byName("vambraces");
		generateMetal(vambracesType);
	}
	static ItemTypeId longSwordType = ItemType::byName("long sword");
	if(hasSidearm)
	{
		auto roll = random.getInRange(0, 100);
		if(roll > 95)
		{
			generateMetal(longSwordType);
		}
		if(roll > 70)
		{
			static ItemTypeId shortSwordType = ItemType::byName("short sword");
			generateMetal(shortSwordType);
		}
		else
		{
			static ItemTypeId daggerType = ItemType::byName("dagger");
			generateMetal(daggerType);
		}
	}
	if(hasLongarm)
	{
		auto roll = random.getInRange(0, 100);
		if(roll > 75)
		{
			static ItemTypeId spearType = ItemType::byName("spear");
			generateMetal(spearType);
		}
		if(roll > 65)
		{
			static ItemTypeId maceType = ItemType::byName("mace");
			generateMetal(maceType);
		}
		if(roll > 45)
		{
			static ItemTypeId glaveType = ItemType::byName("glave");
			generateMetal(glaveType);
		}
		if(roll > 25)
		{
			static ItemTypeId axeType = ItemType::byName("axe");
			generateMetal(axeType);
		}
		else
		{
			generateMetal(longSwordType);
			static ItemTypeId shieldType = ItemType::byName("shield");
			generate(shieldType, poplarWoodType);
		}
	}
	if(hasRangedWeapon)
	{
		auto roll = random.getInRange(0, 100);
		if(roll > 75)
		{
			static ItemTypeId crossbowType = ItemType::byName("crossbow");
			generate(crossbowType, poplarWoodType);
		}
		else
		{
			static ItemTypeId shortbowType = ItemType::byName("shortbow");
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
	data["strength"].get_to(m_strength);
	data["strengthBonusOrPenalty"].get_to(m_strengthBonusOrPenalty);
	data["strengthModifier"].get_to(m_strengthModifier);
	data["agility"].get_to(m_agility);
	data["agilityBonusOrPenalty"].get_to(m_agilityBonusOrPenalty);
	data["agilityModifier"].get_to(m_agilityModifier);
	data["dextarity"].get_to(m_dextarity);
	data["dextarityBonusOrPenalty"].get_to(m_dextarityBonusOrPenalty);
	data["dextarityModifier"].get_to(m_dextarityModifier);
	data["mass"].get_to(m_mass);
	data["massBonusOrPenalty"].get_to(m_massBonusOrPenalty);
	data["massModifier"].get_to(m_massModifier);
	auto& deserializationMemo = m_area.m_simulation.getDeserializationMemo();
	m_hasObjectives.resize(m_id.size());
	for(const Json& pair : data["hasObjectives"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_hasObjectives[index] = std::make_unique<HasObjectives>(index);
		m_hasObjectives[index]->load(pair[1], deserializationMemo, m_area, index);
	}
	m_skillSet.resize(m_id.size());
	for(const Json& pair : data["skillSet"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_skillSet[index] = std::make_unique<SkillSet>();
		m_skillSet[index]->load(pair[1]);
	}
	m_body.resize(m_id.size());
	for(const Json& pair : data["body"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_body[index] = std::make_unique<Body>(pair[1], deserializationMemo, index);
	}
	m_mustSleep.resize(m_id.size());
	for(const Json& pair : data["mustSleep"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_mustSleep[index] = std::make_unique<MustSleep>(m_area, pair[1], index);
	}
	m_mustDrink.resize(m_id.size());
	for(const Json& pair : data["mustDrink"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_mustDrink[index] = std::make_unique<MustDrink>(m_area, pair[1], index, m_species[index]);
	}
	m_mustEat.resize(m_id.size());
	for(const Json& pair : data["mustEat"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_mustEat[index] = std::make_unique<MustEat>(m_area, pair[1], index, m_species[index]);
	}
	m_needsSafeTemperature.resize(m_id.size());
	for(const Json& pair : data["needsSafeTemperature"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_needsSafeTemperature[index] = std::make_unique<ActorNeedsSafeTemperature>(pair[1], index, m_area);
	}
	m_canGrow.resize(m_id.size());
	for(const Json& pair : data["canGrow"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_canGrow[index] = std::make_unique<CanGrow>(m_area, pair[1], index);
	}
	m_equipmentSet.resize(m_id.size());
	for(const Json& pair : data["equipmentSet"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_equipmentSet[index] = std::make_unique<EquipmentSet>(m_area, pair[1]);
	}
	m_canReserve.resize(m_id.size());
	for(const Json& pair : data["canReserve"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_canReserve[index] = std::make_unique<CanReserve>(m_faction[index]);
		m_canReserve[index]->load(pair[1], deserializationMemo, m_area);
	}
	m_hasUniform.resize(m_id.size());
	for(const Json& pair : data["hasUniform"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_hasUniform[index] = std::make_unique<ActorHasUniform>();
		m_hasUniform[index]->load(m_area, pair[1]);
	}
	m_pathIter.resize(m_id.size());
	for(const Json& pair : data["pathIter"])
	{
		ActorIndex index = pair[0].get<ActorIndex>();
		m_pathIter[index] = m_path[index].begin() + pair[1].get<int>();
	}
}
void to_json(Json& data, const std::unique_ptr<CanReserve>& canReserve) { data = canReserve->toJson(); }
void to_json(Json& data, const std::unique_ptr<ActorHasUniform>& uniform) { data = *uniform; }
void to_json(Json& data, const std::unique_ptr<EquipmentSet>& equipmentSet) { data = *equipmentSet; }
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
		{"strength", m_strength},
		{"strengthBonusOrPenalty", m_strengthBonusOrPenalty},
		{"strengthModifier", m_strengthModifier},
		{"agility", m_agility},
		{"agilityBonusOrPenalty", m_agilityBonusOrPenalty},
		{"agilityModifier", m_agilityModifier},
		{"dextarity", m_dextarity},
		{"dextarityBonusOrPenalty", m_dextarityBonusOrPenalty},
		{"dextarityModifier", m_dextarityModifier},
		{"mass", m_mass},
		{"massBonusOrPenalty", m_massBonusOrPenalty},
		{"massModifier", m_massModifier},
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
	for(auto index : getAll())
		output["pathIter"].push_back(m_pathIter[index] - m_path[index].begin());
	return output;
}
void Actors::resize(HasShapeIndex newSize)
{
	ActorIndex actor = newSize.toActor();
	m_referenceTarget.resize(actor);
	m_canReserve.resize(actor);
	m_hasUniform.resize(actor);
	m_equipmentSet.resize(actor);
	m_id.resize(actor);
	m_name.resize(actor);
	m_species.resize(actor);
	m_project.resize(actor);
	m_birthStep.resize(actor);
	m_causeOfDeath.resize(actor);
	m_unencomberedCarryMass.resize(actor);
	m_strength.resize(actor);
	m_strengthBonusOrPenalty.resize(actor);
	m_strengthModifier.resize(actor);
	m_agility.resize(actor);
	m_agilityBonusOrPenalty.resize(actor);
	m_agilityModifier.resize(actor);
	m_dextarity.resize(actor);
	m_dextarityBonusOrPenalty.resize(actor);
	m_dextarityModifier.resize(actor);
	m_mass.resize(actor);
	m_massBonusOrPenalty.resize(actor);
	m_massModifier.resize(actor);
	m_hasObjectives.resize(actor);
	m_body.resize(actor);
	m_mustSleep.resize(actor);
	m_mustDrink.resize(actor);
	m_canGrow.resize(actor);
	m_needsSafeTemperature.resize(actor);
	m_skillSet.resize(actor);
	m_carrying.resize(actor);
	m_stamina.resize(actor);
	m_canSee.resize(actor);
	m_visionRange.resize(actor);
	m_hasVisionFacade.resize(actor);
	m_coolDownEvent.resize(actor);
	m_getIntoAttackPositionPathRequest.resize(actor);
	m_meleeAttackTable.resize(actor);
	m_targetedBy.resize(actor);
	m_target.resize(actor);
	m_onMissCoolDownMelee.resize(actor);
	m_maxMeleeRange.resize(actor);
	m_maxRange.resize(actor);
	m_coolDownDurationModifier.resize(actor);
	m_combatScore.resize(actor);
	m_moveEvent.resize(actor);
	m_pathRequest.resize(actor);
	m_path.resize(actor);
	m_pathIter.resize(actor);
	m_destination.resize(actor);
	m_speedIndividual.resize(actor);
	m_speedActual.resize(actor);
	m_moveRetries.resize(actor);
}
void Actors::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	ActorIndex newActorIndex = newIndex.toActor();
	ActorIndex oldActorIndex = oldIndex.toActor();
	m_referenceTarget[newActorIndex] = std::move(m_referenceTarget[oldActorIndex]);
	m_canReserve[newActorIndex] = std::move(m_canReserve[oldActorIndex]);
	m_hasUniform[newActorIndex] = std::move(m_hasUniform[oldActorIndex]);
	m_equipmentSet[newActorIndex] = std::move(m_equipmentSet[oldActorIndex]);
	m_id[newActorIndex] = m_id[oldActorIndex];
	m_name[newActorIndex] = m_name[oldActorIndex];
	m_species[newActorIndex] = m_species[oldActorIndex];
	m_project[newActorIndex] = m_project[oldActorIndex];
	m_birthStep[newActorIndex] = m_birthStep[oldActorIndex];
	m_causeOfDeath[newActorIndex] = m_causeOfDeath[oldActorIndex];
	m_unencomberedCarryMass[newActorIndex] = m_unencomberedCarryMass[oldActorIndex];
	m_strength[newActorIndex] = m_strength[oldActorIndex];
	m_strengthBonusOrPenalty[newActorIndex] = m_strengthBonusOrPenalty[oldActorIndex];
	m_strengthModifier[newActorIndex] = m_strengthModifier[oldActorIndex];
	m_agility[newActorIndex] = m_agility[oldActorIndex];
	m_agilityBonusOrPenalty[newActorIndex] = m_agilityBonusOrPenalty[oldActorIndex];
	m_agilityModifier[newActorIndex] = m_agilityModifier[oldActorIndex];
	m_dextarity[newActorIndex] = m_dextarity[oldActorIndex];
	m_dextarityBonusOrPenalty[newActorIndex] = m_dextarityBonusOrPenalty[oldActorIndex];
	m_dextarityModifier[newActorIndex] = m_dextarityModifier[oldActorIndex];
	m_mass[newActorIndex] = m_mass[oldActorIndex];
	m_massBonusOrPenalty[newActorIndex] = m_massBonusOrPenalty[oldActorIndex];
	m_massModifier[newActorIndex] = m_massModifier[oldActorIndex];
	m_hasObjectives[newActorIndex] = std::move(m_hasObjectives[oldActorIndex]);
	m_body[newActorIndex] = std::move(m_body[oldActorIndex]);
	m_mustSleep[newActorIndex] = std::move(m_mustSleep[oldActorIndex]);
	m_mustDrink[newActorIndex] = std::move(m_mustDrink[oldActorIndex]);
	m_canGrow[newActorIndex] = std::move(m_canGrow[oldActorIndex]);
	m_needsSafeTemperature[newActorIndex] = std::move(m_needsSafeTemperature[oldActorIndex]);
	m_skillSet[newActorIndex] = std::move(m_skillSet[oldActorIndex]);
	m_carrying[newActorIndex] = m_carrying[oldActorIndex];
	m_stamina[newActorIndex] = m_stamina[oldActorIndex];
	m_canSee[newActorIndex] = m_canSee[oldActorIndex];
	m_visionRange[newActorIndex] = m_visionRange[oldActorIndex];
	m_hasVisionFacade[newActorIndex] = m_hasVisionFacade[oldActorIndex];
	m_coolDownEvent.moveIndex(oldActorIndex, newActorIndex);
	m_getIntoAttackPositionPathRequest[newActorIndex] = std::move(m_getIntoAttackPositionPathRequest[oldActorIndex]);
	m_meleeAttackTable[newActorIndex] = m_meleeAttackTable[oldActorIndex];
	m_targetedBy[newActorIndex] = m_targetedBy[oldActorIndex];
	m_target[newActorIndex] = m_target[oldActorIndex];
	m_onMissCoolDownMelee[newActorIndex] = m_onMissCoolDownMelee[oldActorIndex];
	m_maxMeleeRange[newActorIndex] = m_maxMeleeRange[oldActorIndex];
	m_maxRange[newActorIndex] = m_maxRange[oldActorIndex];
	m_coolDownDurationModifier[newActorIndex] = m_coolDownDurationModifier[oldActorIndex];
	m_combatScore[newActorIndex] = m_combatScore[oldActorIndex];
	m_moveEvent.moveIndex(oldActorIndex, newActorIndex);
	m_pathRequest[newActorIndex] = std::move(m_pathRequest[oldActorIndex]);
	m_path[newActorIndex] = m_path[oldActorIndex];
	m_pathIter[newActorIndex] = m_pathIter[oldActorIndex];
	m_destination[newActorIndex] = m_destination[oldActorIndex];
	m_speedIndividual[newActorIndex] = m_speedIndividual[oldActorIndex];
	m_speedActual[newActorIndex] = m_speedActual[oldActorIndex];
	m_moveRetries[newActorIndex] = m_moveRetries[oldActorIndex];
	// Update references.
	m_referenceTarget[newActorIndex]->index = newActorIndex;
	if(m_carrying[newActorIndex].exists())
	{
		ActorOrItemIndex carrying = m_carrying[newActorIndex];
		if(carrying.isActor())
			m_area.getActors().updateCarrierIndex(carrying.get(), newIndex);
		else
			m_area.getItems().updateCarrierIndex(carrying.get(), newIndex);
	}
	m_hasObjectives[newActorIndex]->updateActorIndex(newActorIndex);
	if(m_pathRequest[newActorIndex] != nullptr)
		m_pathRequest[newActorIndex]->updateActorIndex(newActorIndex);
	if(m_getIntoAttackPositionPathRequest[newActorIndex] != nullptr)
		m_getIntoAttackPositionPathRequest[newActorIndex]->updateActorIndex(newActorIndex);
	for(ActorIndex actor : m_targetedBy[newActorIndex])
		m_target[actor] = newActorIndex;
	if(!m_hasVisionFacade[newActorIndex].empty())
		m_hasVisionFacade[newActorIndex].updateActorIndex(newActorIndex);
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : m_blocks[newActorIndex])
		blocks.actor_updateIndex(block, oldActorIndex, newActorIndex);
}
ActorIndices Actors::getAll() const
{
	// TODO: Replace with std::iota?
	ActorIndices output;
	output.reserve(m_shape.size());
	for(auto i = ActorIndex::create(0); i < size(); ++i)
		output.add(i);
	return output;
}
void Actors::onChangeAmbiantSurfaceTemperature()
{
	for(auto index : getOnSurface())
		m_needsSafeTemperature[index.toActor()]->onChange(m_area);
}
ActorIndex Actors::create(ActorParamaters params)
{
	ActorIndex index = getNextIndex().toActor();
	bool isStatic = false;
	MoveTypeId moveType = AnimalSpecies::getMoveType(params.species);
	ShapeId shape = AnimalSpecies::shapeForPercentGrown(params.species, params.getPercentGrown(m_area.m_simulation));
	Portables::create(index, moveType, shape, params.location, params.facing, isStatic);
	Simulation& s = m_area.m_simulation;
	m_referenceTarget[index]->index = index;
	m_id[index] = params.getId(s);
	m_name[index] = params.getName(s);
	m_species[index] = params.species;
	m_project[index] = nullptr;
	m_birthStep[index] = params.getBirthStep(s);
	m_causeOfDeath[index] = CauseOfDeath::none;
	m_unencomberedCarryMass[index] = Mass::null();
	m_strengthBonusOrPenalty[index] = AttributeLevelBonusOrPenalty::create(0);
	m_strengthModifier[index] = 0.f;
	m_agilityBonusOrPenalty[index] = AttributeLevelBonusOrPenalty::create(0);
	m_agilityModifier[index] = 0.f;
	m_dextarityBonusOrPenalty[index] = AttributeLevelBonusOrPenalty::create(0);
	m_dextarityModifier[index] = 0.f;
	m_massBonusOrPenalty[index] = 0;
	m_massModifier[index] = 0.f;
	m_hasObjectives[index] = std::make_unique<HasObjectives>(index);
	m_body[index] = std::make_unique<Body>(m_area, index);
	m_mustSleep[index] = std::make_unique<MustSleep>(m_area, index);
	m_mustDrink[index] = std::make_unique<MustDrink>(m_area, index);
	m_canGrow[index] = std::make_unique<CanGrow>(m_area, index, params.getPercentGrown(s));
	m_needsSafeTemperature[index] = std::make_unique<ActorNeedsSafeTemperature>(m_area, index);
	m_skillSet[index] = std::make_unique<SkillSet>();
	// CanPickUp.
	m_carrying[index].clear();
	// Stamina.
	m_stamina[index] = Stamina::null();
	// Vision.
	assert(m_canSee[index].empty());
	m_visionRange[index] = AnimalSpecies::getVisionRange(params.species);
	assert(m_hasVisionFacade[index].empty());
	// Combat.
	assert(!m_coolDownEvent.exists(index));
	assert(m_meleeAttackTable[index].empty());
	assert(m_targetedBy[index].empty());
	m_target[index].clear();
	m_onMissCoolDownMelee[index] = Step::null();
	m_maxMeleeRange[index] = DistanceInBlocksFractional::null();
	m_maxRange[index] = DistanceInBlocksFractional::null();
	m_coolDownDurationModifier[index] = 0;
	m_combatScore[index] = CombatScore::null();
	// Move.
	assert(!m_moveEvent.exists(index));
	assert(m_pathRequest[index] == nullptr);
	assert(m_path[index].empty());
	m_destination[index].clear();
	m_speedIndividual[index] = Speed::create(0);
	m_speedActual[index] = Speed::create(0);
	m_moveRetries[index] = 0;
	s.m_actors.registerActor(m_id[index], *this, index);
	attributes_onUpdateGrowthPercent(index);
	sharedConstructor(index);
	scheduleNeeds(index);
	if(isSentient(index))
		params.generateEquipment(m_area, index);
	if(params.location.exists())
		setLocationAndFacing(index, params.location, params.facing);
	return index;
}
void Actors::sharedConstructor(ActorIndex index)
{
	combat_update(index);
	move_updateIndividualSpeed(index);
	m_body[index]->initalize(m_area);
	m_mustDrink[index]->setFluidType(AnimalSpecies::getFluidType(m_species[index]));
}
void Actors::scheduleNeeds(ActorIndex index)
{
	m_mustSleep[index]->scheduleTiredEvent(m_area);
	m_mustDrink[index]->scheduleDrinkEvent(m_area);
	m_mustEat[index]->scheduleHungerEvent(m_area);
	m_canGrow[index]->updateGrowingStatus(m_area);
}
void Actors::setShape(ActorIndex index, ShapeId shape)
{
	BlockIndex location = m_location[index];
	if(location.exists())
		exit(index);
	m_shape[index] = shape;
	if(location.exists())
		setLocation(index, location);
}
void Actors::setLocation(ActorIndex index, BlockIndex block)
{
	assert(m_location[index].exists());
	Facing facing = m_area.getBlocks().facingToSetWhenEnteringFrom(block, m_location[index]);
	setLocationAndFacing(index, block, facing);
}
void Actors::setLocationAndFacing(ActorIndex index, BlockIndex block, Facing facing)
{
	if(m_location[index].exists())
		exit(index);
	Blocks& blocks = m_area.getBlocks();
	for(auto [x, y, z, v] : Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing))
	{
		BlockIndex occupied = blocks.offset(block, x, y, z);
		blocks.actor_record(occupied, index, CollisionVolume::create(v));
	}
	if(blocks.isOnSurface(block))
		m_onSurface.add(index);
	else
		m_onSurface.remove(index);
}
void Actors::exit(ActorIndex index)
{
	assert(m_location[index].exists());
	BlockIndex location = m_location[index];
	auto& blocks = m_area.getBlocks();
	for(auto [x, y, z, v] : Shape::makeOccupiedPositionsWithFacing(m_shape[index], m_facing[index]))
	{
		BlockIndex occupied = blocks.offset(location, x, y, z);
		blocks.actor_erase(occupied, index);
	}
	m_location[index].clear();
	if(blocks.isOnSurface(location))
		m_onSurface.remove(index);
}
void Actors::resetNeeds(ActorIndex index)
{
	m_mustSleep[index]->notTired(m_area);
	m_mustDrink[index]->notThirsty(m_area);
	m_mustEat[index]->notHungry(m_area);
}
void Actors::removeMassFromCorpse(ActorIndex index, Mass mass)
{
	assert(!isAlive(index));
	assert(mass <= m_mass[index]);
	m_mass[index] -= mass;
	if(m_mass[index] == 0)
		leaveArea(index);
}
bool Actors::isEnemy(ActorIndex index, ActorIndex other) const
{
	return m_area.m_simulation.m_hasFactions.isEnemy(getFaction(index), getFaction(other));
}
bool Actors::isAlly(ActorIndex index, ActorIndex other) const
{
	return m_area.m_simulation.m_hasFactions.isAlly(getFaction(index), getFaction(other));
}
//TODO: Zombies are not sentient.
bool Actors::isSentient(ActorIndex index) const { return AnimalSpecies::getSentient(m_species[index]); }
void Actors::die(ActorIndex index, CauseOfDeath causeOfDeath)
{
	m_causeOfDeath[index] = causeOfDeath;
	combat_onDeath(index);
	move_onDeath(index);
	m_mustDrink[index]->onDeath();
	m_mustEat[index]->onDeath();
	m_mustSleep[index]->onDeath();
	if(m_project[index] != nullptr)
		m_project[index]->removeWorker(index);
	if(m_location[index].exists())
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
	m_hasObjectives[index]->addTaskToStart(m_area, std::make_unique<WaitObjective>(m_area, duration, index));
}
void Actors::takeHit(ActorIndex index, Hit& hit, BodyPart& bodyPart)
{
	m_equipmentSet[index]->modifyImpact(m_area, hit, bodyPart.bodyPartType);
	m_body[index]->getHitDepth(hit, bodyPart);
	if(hit.depth != 0)
		m_body[index]->addWound(m_area, bodyPart, hit);
}
void Actors::setFaction(ActorIndex index, FactionId faction)
{
	m_faction[index] = faction;
	if(faction.empty())
		m_canReserve[index] = nullptr;
	else
		m_canReserve[index]->setFaction(faction);
}
Mass Actors::getMass(ActorIndex index) const
{
	return m_mass[index] + m_equipmentSet[index]->getMass() + canPickUp_getMass(index);
}
Volume Actors::getVolume(ActorIndex index) const
{
	return m_body[index]->getVolume(m_area);
}
Quantity Actors::getAgeInYears(ActorIndex index) const
{
	DateTime now(m_area.m_simulation.m_step);
	DateTime birthDate(m_birthStep[index]);
	Quantity differenceYears = Quantity::create(now.year - birthDate.year);
	if(now.day > birthDate.day)
		++differenceYears;
	return differenceYears;
}
Step Actors::getAge(ActorIndex index) const
{
	return m_area.m_simulation.m_step - m_birthStep[index];
}
std::wstring Actors::getActionDescription(ActorIndex index) const
{
	if(m_hasObjectives[index]->hasCurrent())
		return util::stringToWideString(const_cast<HasObjectives&>(*m_hasObjectives[index].get()).getCurrent().name());
	return L"no action";
}
void Actors::reserveAllBlocksAtLocationAndFacing(ActorIndex index, const BlockIndex location, Facing facing)
{
	if(m_faction[index].empty())
		return;
	for(BlockIndex occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		m_area.getBlocks().reserve(occupied, *m_canReserve[index]);
}
void Actors::unreserveAllBlocksAtLocationAndFacing(ActorIndex index, const BlockIndex location, Facing facing)
{
	if(m_faction[index].empty())
		return;
	for(BlockIndex occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		m_area.getBlocks().unreserve(occupied, *m_canReserve[index]);
}
void Actors::setBirthStep(ActorIndex index, Step step)
{
	m_birthStep[index] = step;
	m_canGrow[index]->updateGrowingStatus(m_area);
}
Percent Actors::getPercentGrown(ActorIndex index) const { return m_canGrow[index]->growthPercent(); }
void Actors::log(ActorIndex index) const
{
	std::wcout << m_name[index];
	std::cout << "(" << AnimalSpecies::getName(m_species[index]) << ")";
	Portables::log(index);
	std::cout << std::endl;
}
void Actors::satisfyNeeds(ActorIndex index)
{
	// Wake up if asleep.
	if(!m_mustSleep[index]->isAwake())
		m_mustSleep[index]->wakeUp(m_area);
	else
		m_mustSleep[index]->notTired(m_area);
	// Discard drink objective if exists.
	if(m_mustDrink[index]->getVolumeFluidRequested() != 0)
		m_mustDrink[index]->drink(m_area, m_mustDrink[index]->getVolumeFluidRequested());
	// Discard eat objective if exists.
	if(m_mustEat[index]->getMassFoodRequested() != 0)
		m_mustEat[index]->eat(m_area, m_mustEat[index]->getMassFoodRequested());
}
// Sleep.
void Actors::sleep_do(ActorIndex index) { m_mustSleep[index]->sleep(m_area); }
void Actors::sleep_wakeUp(ActorIndex index){ m_mustSleep[index]->wakeUp(m_area); }
void Actors::sleep_wakeUpEarly(ActorIndex index){ m_mustSleep[index]->wakeUpEarly(m_area); }
void Actors::sleep_setSpot(ActorIndex index, BlockIndex location) { m_mustSleep[index]->setLocation(location); }
void Actors::sleep_makeTired(ActorIndex index) { m_mustSleep[index]->tired(m_area); }
bool Actors::sleep_isAwake(ActorIndex index) const { return m_mustSleep[index]->isAwake(); }
Percent Actors::sleep_getPercentDoneSleeping(ActorIndex index) const { return m_mustSleep[index]->getSleepPercent(); }
Percent Actors::sleep_getPercentTired(ActorIndex index) const { return m_mustSleep[index]->getTiredPercent(); }
BlockIndex Actors::sleep_getSpot(ActorIndex index) const { return m_mustSleep[index]->getLocation(); }
bool Actors::sleep_hasTiredEvent(ActorIndex index) const { return m_mustSleep[index]->hasTiredEvent(); }
// Skills.
SkillLevel Actors::skill_getLevel(ActorIndex index, SkillTypeId skillType) const { return m_skillSet[index]->get(skillType); }
// CoolDownEvent.
AttackCoolDownEvent::AttackCoolDownEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data["delay"].get<Step>(), data["start"].get<Step>())
{
	nlohmann::from_json(data, *static_cast<ScheduledEvent*>(this));
	data["actor"].get_to(m_actor);
}
void AttackCoolDownEvent::execute(Simulation&, Area* area) { area->getActors().combat_coolDownCompleted(m_actor); }
void AttackCoolDownEvent::clearReferences(Simulation&, Area* area) { area->getActors().m_coolDownEvent.clearPointer(m_actor); }
Json AttackCoolDownEvent::toJson() const
{
	Json output;
	nlohmann::to_json(output, *static_cast<const ScheduledEvent*>(this));
	output["actor"] = m_actor;
	return output;
}
