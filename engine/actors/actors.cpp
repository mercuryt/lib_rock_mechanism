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
#include "../portables.hpp"
#include "grow.h"
#include "equipment.h"
#include "eventSchedule.h"
#include "hasShapes.h"
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
void ActorParamaters::generateEquipment(Area& area, const ActorIndex& actor)
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
	auto generate = [&](ItemTypeId itemType, const MaterialTypeId& materialType){
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
	Portables<Actors, ActorIndex, ActorReferenceIndex>(area, true),
	m_coolDownEvent(area.m_eventSchedule),
	m_moveEvent(area.m_eventSchedule)
	{
		assert(!area.m_loaded);
	}
Actors::~Actors()
{
	/*
	m_mustSleep.clear();
	m_mustEat.clear();
	m_mustSleep.clear();
	m_needsSafeTemperature.clear();
	m_canGrow.clear();
	m_canReserve.clear();
	*/
	// TODO: Clear path request?
}
void Actors::load(const Json& data)
{
	Portables<Actors, ActorIndex, ActorReferenceIndex>::load(data);
	ActorIndex size = ActorIndex::create(m_location.size());
	data["id"].get_to(m_id);
	data["name"].get_to(m_name);
	data["species"].get_to(m_species);
	// No need to serialize m_project.
	data["birthStep"].get_to(m_birthStep);
	data["causeOfDeath"].get_to(m_causeOfDeath);
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
	data["unencomberedCarryMass"].get_to(m_unencomberedCarryMass);
	data["leadFollowPath"].get_to(m_leadFollowPath);
	data["carrying"].get_to(m_carrying);
	data["stamina"].get_to(m_stamina);
	data["visionRange"].get_to(m_visionRange);
	m_coolDownEvent.load(m_area.m_simulation, data["coolDownEvent"], size.toHasShape());
	data["targetedBy"].get_to(m_targetedBy);
	data["target"].get_to(m_target);
	data["onMissCoolDownMelee"].get_to(m_onMissCoolDownMelee);
	data["maxMeleeRange"].get_to(m_maxMeleeRange);
	data["maxRange"].get_to(m_maxRange);
	data["coolDownDurationModifier"].get_to(m_coolDownDurationModifier);
	data["combatScore"].get_to(m_combatScore);
	m_moveEvent.load(m_area.m_simulation, data["moveEvent"], size.toHasShape());
	data["path"].get_to(m_path);
	data["destination"].get_to(m_destination);
	data["speedIndividual"].get_to(m_speedIndividual);
	data["speedActual"].get_to(m_speedActual);
	data["moveRetries"].get_to(m_moveRetries);
	auto& deserializationMemo = m_area.m_simulation.getDeserializationMemo();
	m_skillSet.resize(size);
	const auto& skillData = data["skillSet"];
	for(auto iter = skillData.begin(); iter != skillData.end(); ++iter)
	{
		ActorIndex index = ActorIndex::create(std::stoi(iter.key()));
		m_skillSet[index] = std::make_unique<SkillSet>();
		m_skillSet[index]->load(iter.value());
	}
	m_body.resize(size);
	const auto& bodyData = data["body"]["data"];
	ActorIndex i = ActorIndex::create(0);
	for(const Json& data : bodyData)
	{
		m_body[i] = std::make_unique<Body>(data, deserializationMemo, i);
		++i;
	}
	m_mustSleep.resize(size);
	i = ActorIndex::create(0);
	const auto& sleepData = data["mustSleep"]["data"];
	for(const Json& data : sleepData)
	{
		m_mustSleep[i] = std::make_unique<MustSleep>(m_area, data, i);
		++i;
	}
	m_mustDrink.resize(size);
	const auto& drinkData = data["mustDrink"]["data"];
	i = ActorIndex::create(0);
	for(const Json& data : drinkData)
	{
		m_mustDrink[i] = std::make_unique<MustDrink>(m_area, data, i, m_species[i]);
		++i;
	}
	m_mustEat.resize(size);
	i = ActorIndex::create(0);
	const auto& eatData = data["mustEat"]["data"];
	for(const Json& data : eatData)
	{
		m_mustEat[i] = std::make_unique<MustEat>(m_area, data, i, m_species[i]);
		++i;
	}
	m_needsSafeTemperature.resize(size);
	i = ActorIndex::create(0);
	const auto& temperatureData = data["needsSafeTemperature"]["data"];
	for(const Json& data : temperatureData)
	{
		m_needsSafeTemperature[i] = std::make_unique<ActorNeedsSafeTemperature>(data, i, m_area);
		++i;
	}
	m_canGrow.resize(size);
	i = ActorIndex::create(0);
	const auto& growData = data["canGrow"]["data"];
	for(const Json& data : growData)
	{
		m_canGrow[i] = std::make_unique<CanGrow>(m_area, data, i);
		++i;
	}
	m_equipmentSet.resize(size);
	const auto& equipmentData = data["equipmentSet"];
	for(auto iter = equipmentData.begin(); iter != equipmentData.end(); ++iter)
	{
		ActorIndex index = ActorIndex::create(std::stoi(iter.key()));
		m_equipmentSet[index] = std::make_unique<EquipmentSet>(m_area, iter.value());
	}
	m_hasUniform.resize(size);
	const auto& uniformData = data["hasUniform"];
	for(auto iter = uniformData.begin(); iter != uniformData.end(); ++iter)
	{
		ActorIndex index = ActorIndex::create(std::stoi(iter.key()));
		m_hasUniform[index] = std::make_unique<ActorHasUniform>();
		m_hasUniform[index]->load(m_area, iter.value(), m_faction[index]);
	}
	m_pathIter.resize(size);
	const auto& pathIterData = data["pathIter"];
	i = ActorIndex::create(0);
	for(const Json& data : pathIterData)
	{
		if(!m_path[i].empty())
			m_pathIter[i] = m_path[i].begin() + data.get<uint>();
		++i;
	}
	m_canSee.resize(size);
	assert(data["canSee"].type() == Json::value_t::object);
	const auto& canSeeData = data["canSee"];
	for(auto iter = canSeeData.begin(); iter != canSeeData.end(); ++iter)
	{
		ActorIndex index = ActorIndex::create(std::stoi(iter.key()));
		ActorReference ref = getReference(index);
		for(const Json& data : iter.value()["m_data"])
		{
			ActorReference otherRef(data, m_referenceData);
			m_canSee[index].insert(otherRef);
			ActorIndex otherIndex = otherRef.getIndex(m_referenceData);
			m_canBeSeenBy[otherIndex].insert(ref);
		}
	}
	for(ActorIndex index : getAll())
	{
		m_area.m_simulation.m_actors.registerActor(getId(index), m_area.getActors(), index);
		Blocks &blocks = m_area.getBlocks();
		if(m_location[index].exists())
			for (auto [x, y, z, v] : Shape::makeOccupiedPositionsWithFacing(m_shape[index], m_facing[index]))
			{
				BlockIndex occupied = blocks.offset(m_location[index], x, y, z);
				blocks.actor_record(occupied, index, CollisionVolume::create(v));
			}
	}
	m_project.resize(size);
}
void Actors::loadObjectivesAndReservations(const Json& data)
{
	auto& deserializationMemo = m_area.m_simulation.getDeserializationMemo();
	m_hasObjectives.resize(m_id.size());
	const auto& objectiveData = data["hasObjectives"]["data"];
	ActorIndex index = ActorIndex::create(0);
	for(const Json& data : objectiveData)
	{
		m_hasObjectives[index] = std::make_unique<HasObjectives>(index);
		m_hasObjectives[index]->load(data, deserializationMemo, m_area, index);
		++index;
	}
	m_canReserve.resize(m_id.size());
	index = ActorIndex::create(0);
	const auto& reserveData = data["canReserve"]["data"];
	for(const Json& data : reserveData)
	{
		m_canReserve[index] = std::make_unique<CanReserve>(m_faction[index]);
		m_canReserve[index]->load(data, deserializationMemo, m_area);
		++index;
	}
	m_pathRequest.resize(m_id.size());
	const auto& pathRequestData = data["pathRequest"];
	for(auto iter = pathRequestData.begin(); iter != pathRequestData.end(); ++iter)
	{
		ActorIndex index = ActorIndex::create(std::stoi(iter.key()));
		m_pathRequest[index] = &PathRequest::load(iter.value(), deserializationMemo, m_area, m_moveType[index]);
	}
}
void to_json(Json& data, const std::unique_ptr<CanReserve>& canReserve) { data = canReserve->toJson(); }
void to_json(Json& data, const std::unique_ptr<ActorHasUniform>& hasUniform)
{
	if(hasUniform == nullptr)
		data = false;
	else
		data = *hasUniform;
}
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
		{"id", m_id},
		{"name", m_name},
		{"species", m_species},
		{"project", m_project},
		{"birthStep", m_birthStep},
		{"canSee", Json::object()},
		{"causeOfDeath", m_causeOfDeath},
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
		{"unencomberedCarryMass", m_unencomberedCarryMass},
		{"leadFollowPath", m_leadFollowPath},
		{"hasObjectives", m_hasObjectives},
		{"body", m_body},
		{"mustSleep", m_mustSleep},
		{"mustDrink", m_mustDrink},
		{"mustEat", m_mustEat},
		{"needsSafeTemperature", m_needsSafeTemperature},
		{"canGrow", m_canGrow},
		{"skillSet", Json::object()},
		{"canReserve", m_canReserve},
		{"hasUniform", Json::object()},
		{"equipmentSet", Json::object()},
		{"carrying", m_carrying},
		{"stamina", m_stamina},
		{"visionRange", m_visionRange},
		{"coolDownEvent", m_coolDownEvent},
		// TODO: These don't need to be serialized
		{"targetedBy", m_targetedBy},
		{"target", m_target},
		// TODO: These don't need to be serialized
		{"onMissCoolDownMelee", m_onMissCoolDownMelee},
		{"maxMeleeRange", m_maxMeleeRange},
		{"maxRange", m_maxRange},
		{"coolDownDurationModifier", m_coolDownDurationModifier},
		{"combatScore", m_combatScore},
		{"moveEvent", m_moveEvent},
		{"pathRequest", Json::object()},
		{"path", m_path},
		{"pathIter", Json::array()},
		{"destination", m_destination},
		{"speedIndividual", m_speedIndividual},
		{"speedActual", m_speedActual},
		{"moveRetries", m_moveRetries},
	};
	for(auto index : getAll())
	{
		std::string i = std::to_string(index.get());
		if(m_skillSet[index] != nullptr)
			output["skillSet"][i] = m_skillSet[index]->toJson();
		if(m_hasUniform[index] != nullptr)
			output["uniform"][i] = *m_hasUniform[index];
		if(m_equipmentSet[index] != nullptr)
			output["equipmentSet"][i] = *m_equipmentSet[index];
		output["pathIter"].push_back(m_pathIter[index] - m_path[index].begin());
		if(m_pathRequest[index] != nullptr)
			output["pathRequest"][i] = *m_pathRequest[index];
		if(!m_canSee[index].empty())
			output["canSee"][i] = m_canSee[index];
	}
	output.update(Portables<Actors, ActorIndex, ActorReferenceIndex>::toJson());
	return output;
}
void Actors::resize(const ActorIndex& newSize)
{
	Portables<Actors, ActorIndex, ActorReferenceIndex>::resize(newSize);
	m_id.resize(newSize);
	m_name.resize(newSize);
	m_species.resize(newSize);
	m_project.resize(newSize);
	m_birthStep.resize(newSize);
	m_causeOfDeath.resize(newSize);
	m_strength.resize(newSize);
	m_strengthBonusOrPenalty.resize(newSize);
	m_strengthModifier.resize(newSize);
	m_agility.resize(newSize);
	m_agilityBonusOrPenalty.resize(newSize);
	m_agilityModifier.resize(newSize);
	m_dextarity.resize(newSize);
	m_dextarityBonusOrPenalty.resize(newSize);
	m_dextarityModifier.resize(newSize);
	m_mass.resize(newSize);
	m_massBonusOrPenalty.resize(newSize);
	m_massModifier.resize(newSize);
	m_unencomberedCarryMass.resize(newSize);
	m_leadFollowPath.resize(newSize);
	m_hasObjectives.resize(newSize);
	m_body.resize(newSize);
	m_mustSleep.resize(newSize);
	m_mustDrink.resize(newSize);
	m_mustEat.resize(newSize);
	m_needsSafeTemperature.resize(newSize);
	m_canGrow.resize(newSize);
	m_skillSet.resize(newSize);
	m_canReserve.resize(newSize);
	m_hasUniform.resize(newSize);
	m_equipmentSet.resize(newSize);
	m_carrying.resize(newSize);
	m_stamina.resize(newSize);
	m_canSee.resize(newSize);
	m_canBeSeenBy.resize(newSize);
	m_visionRange.resize(newSize);
	m_coolDownEvent.resize(newSize);
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
void Actors::moveIndex(const ActorIndex& oldIndex, const ActorIndex& newIndex)
{
	Portables<Actors, ActorIndex, ActorReferenceIndex>::moveIndex(oldIndex, newIndex);
	m_id[newIndex] = m_id[oldIndex];
	m_name[newIndex] = m_name[oldIndex];
	m_species[newIndex] = m_species[oldIndex];
	m_project[newIndex] = m_project[oldIndex];
	m_birthStep[newIndex] = m_birthStep[oldIndex];
	m_causeOfDeath[newIndex] = m_causeOfDeath[oldIndex];
	m_strength[newIndex] = m_strength[oldIndex];
	m_strengthBonusOrPenalty[newIndex] = m_strengthBonusOrPenalty[oldIndex];
	m_strengthModifier[newIndex] = m_strengthModifier[oldIndex];
	m_agility[newIndex] = m_agility[oldIndex];
	m_agilityBonusOrPenalty[newIndex] = m_agilityBonusOrPenalty[oldIndex];
	m_agilityModifier[newIndex] = m_agilityModifier[oldIndex];
	m_dextarity[newIndex] = m_dextarity[oldIndex];
	m_dextarityBonusOrPenalty[newIndex] = m_dextarityBonusOrPenalty[oldIndex];
	m_dextarityModifier[newIndex] = m_dextarityModifier[oldIndex];
	m_mass[newIndex] = m_mass[oldIndex];
	m_massBonusOrPenalty[newIndex] = m_massBonusOrPenalty[oldIndex];
	m_massModifier[newIndex] = m_massModifier[oldIndex];
	m_leadFollowPath[newIndex] = m_leadFollowPath[oldIndex];
	m_unencomberedCarryMass[newIndex] = m_unencomberedCarryMass[oldIndex];
	m_hasObjectives[newIndex] = std::move(m_hasObjectives[oldIndex]);
	m_body[newIndex] = std::move(m_body[oldIndex]);
	m_mustSleep[newIndex] = std::move(m_mustSleep[oldIndex]);
	m_mustDrink[newIndex] = std::move(m_mustDrink[oldIndex]);
	m_mustEat[newIndex] = std::move(m_mustEat[oldIndex]);
	m_needsSafeTemperature[newIndex] = std::move(m_needsSafeTemperature[oldIndex]);
	m_canGrow[newIndex] = std::move(m_canGrow[oldIndex]);
	m_skillSet[newIndex] = std::move(m_skillSet[oldIndex]);
	m_canReserve[newIndex] = std::move(m_canReserve[oldIndex]);
	m_hasUniform[newIndex] = std::move(m_hasUniform[oldIndex]);
	m_equipmentSet[newIndex] = std::move(m_equipmentSet[oldIndex]);
	m_carrying[newIndex] = m_carrying[oldIndex];
	m_stamina[newIndex] = m_stamina[oldIndex];
	m_canSee[newIndex] = m_canSee[oldIndex];
	m_canBeSeenBy[newIndex] = m_canBeSeenBy[oldIndex];
	m_visionRange[newIndex] = m_visionRange[oldIndex];
	m_coolDownEvent.moveIndex(oldIndex, newIndex);
	m_meleeAttackTable[newIndex] = m_meleeAttackTable[oldIndex];
	m_targetedBy[newIndex] = m_targetedBy[oldIndex];
	m_target[newIndex] = m_target[oldIndex];
	m_onMissCoolDownMelee[newIndex] = m_onMissCoolDownMelee[oldIndex];
	m_maxMeleeRange[newIndex] = m_maxMeleeRange[oldIndex];
	m_maxRange[newIndex] = m_maxRange[oldIndex];
	m_coolDownDurationModifier[newIndex] = m_coolDownDurationModifier[oldIndex];
	m_combatScore[newIndex] = m_combatScore[oldIndex];
	m_moveEvent.moveIndex(oldIndex, newIndex);
	move_pathRequestMaybeCancel(newIndex);
	m_pathRequest[newIndex] = std::move(m_pathRequest[oldIndex]);
	m_path[newIndex] = m_path[oldIndex];
	m_pathIter[newIndex] = m_pathIter[oldIndex];
	m_destination[newIndex] = m_destination[oldIndex];
	m_speedIndividual[newIndex] = m_speedIndividual[oldIndex];
	m_speedActual[newIndex] = m_speedActual[oldIndex];
	m_moveRetries[newIndex] = m_moveRetries[oldIndex];
	// Update references.
	// Move event contains a ActorIndex which must be updated if MoveEvent exists.
	if(m_moveEvent.contains(newIndex))
		m_moveEvent[newIndex].updateIndex(oldIndex, newIndex);
	// If carrying anything update the carried thing's carrierIndex.
	if(m_carrying[newIndex].exists())
	{
		ActorOrItemIndex carrying = m_carrying[newIndex];
		if(carrying.isActor())
			m_area.getActors().updateCarrierIndex(carrying.getActor(), newIndex);
		else
			m_area.getItems().updateCarrierIndex(carrying.getItem(), newIndex);
	}
	// Update stored actor indices in objectives.
	m_hasObjectives[newIndex]->updateActorIndex(newIndex);
	// Update stored index for all actors who are targeting this one.
	for(ActorIndex actor : m_targetedBy[newIndex])
		m_target[actor] = newIndex;
	// Update ActorIndices stored in occupied block(s).
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : m_blocks[newIndex])
		blocks.actor_updateIndex(block, oldIndex, newIndex);
}
void Actors::destroy(const ActorIndex& index)
{
	// No need to explicitly unschedule events here, destorying the event holder will do it.
	if(hasLocation(index))
	{
		vision_clearRequestIfExists(index);
		exit(index);
	}
	const auto& s = ActorIndex::create(size() - 1);
	if(index != s)
		moveIndex(s, index);
	resize(s);
	// Will do the same move / resize logic internally, so stays in sync with moves from the DataVectors.
	m_referenceData.remove(index);
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
		m_needsSafeTemperature[index]->onChange(m_area);
}
ActorIndex Actors::create(ActorParamaters params)
{
	ActorIndex index = ActorIndex::create(m_id.size());
	resize(index + 1);
	bool isStatic = false;
	MoveTypeId moveType = AnimalSpecies::getMoveType(params.species);
	m_area.m_hasTerrainFacades.maybeRegisterMoveType(moveType);
	ShapeId shape = AnimalSpecies::shapeForPercentGrown(params.species, params.getPercentGrown(m_area.m_simulation));
	Portables<Actors, ActorIndex, ActorReferenceIndex>::create(index, moveType, shape, params.faction, isStatic, Quantity::create(1));
	Simulation& simulation = m_area.m_simulation;
	m_id[index] = params.getId(simulation);
	m_name[index] = params.getName(simulation);
	m_species[index] = params.species;
	m_project[index] = nullptr;
	m_birthStep[index] = params.getBirthStep(simulation);
	m_causeOfDeath[index] = CauseOfDeath::none;
	m_strengthBonusOrPenalty[index] = AttributeLevelBonusOrPenalty::create(0);
	m_strengthModifier[index] = 0.f;
	m_agilityBonusOrPenalty[index] = AttributeLevelBonusOrPenalty::create(0);
	m_agilityModifier[index] = 0.f;
	m_dextarityBonusOrPenalty[index] = AttributeLevelBonusOrPenalty::create(0);
	m_dextarityModifier[index] = 0.f;
	//TODO: Only allocate equipment set for actors which have equipment.
	m_equipmentSet[index] = std::make_unique<EquipmentSet>();
	m_massBonusOrPenalty[index] = 0;
	m_massModifier[index] = 0.f;
	m_unencomberedCarryMass[index] = Mass::null();
	assert(m_leadFollowPath[index].empty());
	m_hasObjectives[index] = std::make_unique<HasObjectives>(index);
	m_body[index] = std::make_unique<Body>(m_area, index);
	m_mustSleep[index] = std::make_unique<MustSleep>(m_area, index);
	m_mustDrink[index] = std::make_unique<MustDrink>(m_area, index);
	m_mustEat[index] = std::make_unique<MustEat>(m_area, index);
	m_needsSafeTemperature[index] = std::make_unique<ActorNeedsSafeTemperature>(m_area, index);
	m_canGrow[index] = std::make_unique<CanGrow>(m_area, index, params.getPercentGrown(simulation));
	m_skillSet[index] = std::make_unique<SkillSet>();
	// TODO: can reserve is not needed for non sentients or actors without factions.
	m_canReserve[index] = std::make_unique<CanReserve>(params.faction);
	assert(m_hasUniform[index] == nullptr);
	// CanPickUp.
	m_carrying[index].clear();
	// Stamina.
	m_stamina[index] = Stamina::null();
	// Vision.
	assert(m_canSee[index].empty());
	assert(m_canBeSeenBy[index].empty());
	m_visionRange[index] = AnimalSpecies::getVisionRange(params.species);
	// Combat.
	assert(!m_coolDownEvent.exists(index));
	assert(m_meleeAttackTable[index].empty());
	assert(m_targetedBy[index].empty());
	m_target[index].clear();
	m_onMissCoolDownMelee[index].clear();
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
	simulation.m_actors.registerActor(m_id[index], *this, index);
	attributes_onUpdateGrowthPercent(index);
	stamina_setFull(index);
	if(isSentient(index))
		params.generateEquipment(m_area, index);
	else if(params.faction.exists())
		m_area.m_hasHaulTools.registerYokeableActor(m_area, index);
	sharedConstructor(index);
	scheduleNeeds(index);
	if(params.location.exists())
		setLocationAndFacing(index, params.location, (params.facing.exists() ? params.facing : Facing::create(0)));
	return index;
}
void Actors::sharedConstructor(const ActorIndex& index)
{
	m_body[index]->initalize(m_area);
	combat_update(index);
	move_updateIndividualSpeed(index);
	m_mustDrink[index]->setFluidType(AnimalSpecies::getFluidType(m_species[index]));
}
void Actors::scheduleNeeds(const ActorIndex& index)
{
	assert(m_mustSleep[index] != nullptr);
	assert(m_mustDrink[index] != nullptr);
	assert(m_mustEat[index] != nullptr);
	assert(m_canGrow[index] != nullptr);
	m_mustSleep[index]->scheduleTiredEvent(m_area);
	m_mustDrink[index]->scheduleDrinkEvent(m_area);
	m_mustEat[index]->scheduleHungerEvent(m_area);
	// TODO: check for safe temperature, create a get to safe temperature objective?
	m_canGrow[index]->updateGrowingStatus(m_area);
}
void Actors::setShape(const ActorIndex& index, const ShapeId& shape)
{
	BlockIndex location = m_location[index];
	Facing facing = m_facing[index];
	if(location.exists())
		exit(index);
	m_shape[index] = shape;
	if(location.exists())
		setLocationAndFacing(index, location, facing);
}
void Actors::setLocation(const ActorIndex& index, const BlockIndex& block)
{
	assert(m_location[index].exists());
	Facing facing = m_area.getBlocks().facingToSetWhenEnteringFrom(block, m_location[index]);
	setLocationAndFacing(index, block, facing);
}
void Actors::setLocationAndFacing(const ActorIndex& index, const BlockIndex& block, const Facing& facing)
{
	assert(block.exists());
	assert(facing.exists());
	if(m_location[index].exists())
		exit(index);
	m_location[index] = block;
	m_facing[index] = facing;
	Blocks& blocks = m_area.getBlocks();
	assert(m_blocks[index].empty());
	for(const auto& [x, y, z, v] : Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing))
	{
		BlockIndex occupied = blocks.offset(block, x, y, z);
		blocks.actor_record(occupied, index, CollisionVolume::create(v));
		m_blocks[index].add(occupied);
		// Record in vision facade if has location and can currently see.
	}
	vision_maybeUpdateLocation(index, block);
	// TODO: reduntand with exit also calling getReference.
	m_area.m_octTree.record(m_area, getReference(index));
	if(blocks.isOnSurface(block))
		m_onSurface.add(index);
	else
		m_onSurface.remove(index);
}
void Actors::exit(const ActorIndex& index)
{
	assert(m_location[index].exists());
	BlockIndex location = m_location[index];
	m_area.m_octTree.erase(m_area, getReference(index));
	auto& blocks = m_area.getBlocks();
	for(BlockIndex occupied : m_blocks[index])
		blocks.actor_erase(occupied, index);
	m_location[index].clear();
	m_blocks[index].clear();
	if(blocks.isOnSurface(location))
		m_onSurface.remove(index);
	move_pathRequestMaybeCancel(index);
}
void Actors::resetNeeds(const ActorIndex& index)
{
	m_mustSleep[index]->notTired(m_area);
	m_mustDrink[index]->notThirsty(m_area);
	m_mustEat[index]->notHungry(m_area);
}
void Actors::removeMassFromCorpse(const ActorIndex& index, const Mass& mass)
{
	assert(!isAlive(index));
	assert(mass <= m_mass[index]);
	m_mass[index] -= mass;
	if(m_mass[index] == 0)
		leaveArea(index);
}
bool Actors::isEnemy(const ActorIndex& index, const ActorIndex& other) const
{
	return m_area.m_simulation.m_hasFactions.isEnemy(getFaction(index), getFaction(other));
}
bool Actors::isAlly(const ActorIndex& index, const ActorIndex& other) const
{
	return m_area.m_simulation.m_hasFactions.isAlly(getFaction(index), getFaction(other));
}
//TODO: Zombies are not sentient.
bool Actors::isSentient(const ActorIndex& index) const { return AnimalSpecies::getSentient(m_species[index]); }
void Actors::die(const ActorIndex& index, CauseOfDeath causeOfDeath)
{
	m_causeOfDeath[index] = causeOfDeath;
	combat_onDeath(index);
	move_onDeath(index);
	m_mustDrink[index]->unschedule();
	m_mustEat[index]->unschedule();
	m_mustSleep[index]->unschedule();
	if(m_project[index] != nullptr)
		m_project[index]->removeWorker(index);
	if(m_location[index].exists())
		maybeSetStatic(index);
	vision_clearRequestIfExists(index);
	move_pathRequestMaybeCancel(index);
	if(!isSentient(index) && hasFaction(index))
		m_area.m_hasHaulTools.unregisterYokeableActor(m_area, index);
}
void Actors::passout(const ActorIndex&, const Step&)
{
	//TODO
}
void Actors::leaveArea(const ActorIndex& index)
{
	combat_onLeaveArea(index);
	move_onLeaveArea(index);
	if(!isSentient(index) && hasFaction(index))
		m_area.m_hasHaulTools.unregisterYokeableActor(m_area, index);
	exit(index);
}
void Actors::wait(const ActorIndex& index, const Step& duration)
{
	m_hasObjectives[index]->addTaskToStart(m_area, std::make_unique<WaitObjective>(m_area, duration, index));
}
void Actors::takeHit(const ActorIndex& index, Hit& hit, BodyPart& bodyPart)
{
	m_equipmentSet[index]->modifyImpact(m_area, hit, bodyPart.bodyPartType);
	m_body[index]->getHitDepth(hit, bodyPart);
	if(hit.depth != 0)
		m_body[index]->addWound(m_area, bodyPart, hit);
}
void Actors::setFaction(const ActorIndex& index, const FactionId& faction)
{
	m_faction[index] = faction;
	if(faction.empty())
		m_canReserve[index] = nullptr;
	else if(m_canReserve[index] == nullptr)
		m_canReserve[index] = std::make_unique<CanReserve>(faction);
	else
		m_canReserve[index]->setFaction(faction);
}
Mass Actors::getMass(const ActorIndex& index) const
{
	return getIntrinsicMass(index) + m_equipmentSet[index]->getMass() + canPickUp_getMass(index);
}
Volume Actors::getVolume(const ActorIndex& index) const
{
	return m_body[index]->getVolume(m_area);
}
Quantity Actors::getAgeInYears(const ActorIndex& index) const
{
	DateTime now(m_area.m_simulation.m_step);
	DateTime birthDate(m_birthStep[index]);
	Quantity differenceYears = Quantity::create(now.year - birthDate.year);
	if(now.day > birthDate.day)
		++differenceYears;
	return differenceYears;
}
Step Actors::getAge(const ActorIndex& index) const
{
	return m_area.m_simulation.m_step - m_birthStep[index];
}
std::wstring Actors::getActionDescription(const ActorIndex& index) const
{
	if(m_hasObjectives[index]->hasCurrent())
		return util::stringToWideString(const_cast<HasObjectives&>(*m_hasObjectives[index].get()).getCurrent().name());
	return L"no action";
}
void Actors::reserveAllBlocksAtLocationAndFacing(const ActorIndex& index, const BlockIndex& location, const Facing& facing)
{
	if(m_faction[index].empty())
		return;
	for(BlockIndex occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		m_area.getBlocks().reserve(occupied, *m_canReserve[index]);
}
void Actors::unreserveAllBlocksAtLocationAndFacing(const ActorIndex& index, const BlockIndex& location, const Facing& facing)
{
	if(m_faction[index].empty())
		return;
	for(BlockIndex occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		m_area.getBlocks().unreserve(occupied, *m_canReserve[index]);
}
void Actors::setBirthStep(const ActorIndex& index, const Step& step)
{
	m_birthStep[index] = step;
	m_canGrow[index]->updateGrowingStatus(m_area);
}
Percent Actors::getPercentGrown(const ActorIndex& index) const { return m_canGrow[index]->growthPercent(); }
void Actors::log(const ActorIndex& index) const
{
	Blocks& blocks = m_area.getBlocks();
	std::wcout << m_name[index];
	std::cout << "(" << AnimalSpecies::getName(m_species[index]) << ")";
	Portables<Actors, ActorIndex, ActorReferenceIndex>::log(index);
	if(objective_exists(index))
		std::cout << ", current objective: " << objective_getCurrentName(index);
	if(m_destination[index].exists())
		std::cout << ", destination: " << blocks.getCoordinates(m_destination[index]).toString();
	if(!m_path[index].empty())
		std::cout << ", path length: " << std::to_string(m_path[index].size());
	if(m_pathRequest[index] != nullptr)
		std::cout << ", path request exists";
	if(m_project[index] != nullptr)
		std::cout << ", project location: " << blocks.getCoordinates(m_project[index]->getLocation()).toString();
	if(m_carrying[index].exists())
	{
		std::cout << ", carrying: {";
		if(m_carrying[index].isActor())
			std::wcout << L"actor:" << getName(m_carrying[index].getActor());
		else
		{
			ItemIndex item = m_carrying[index].getItem();
			Items& items = m_area.getItems();
			ItemTypeId itemType = items.getItemType(item);
			std::cout << "item:" << ItemType::getName(itemType);
			if(ItemType::getIsGeneric(itemType))
				std::cout << ", quantity:" << std::to_string(items.getQuantity(item).get());
		}
		std::cout << "}";
	}
	std::cout << std::endl;
}
void Actors::satisfyNeeds(const ActorIndex& index)
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
void Actors::sleep_do(const ActorIndex& index) { m_mustSleep[index]->sleep(m_area); }
void Actors::sleep_wakeUp(const ActorIndex& index){ m_mustSleep[index]->wakeUp(m_area); }
void Actors::sleep_wakeUpEarly(const ActorIndex& index){ m_mustSleep[index]->wakeUpEarly(m_area); }
void Actors::sleep_setSpot(const ActorIndex& index, const BlockIndex& location) { m_mustSleep[index]->setLocation(location); }
void Actors::sleep_makeTired(const ActorIndex& index) { m_mustSleep[index]->tired(m_area); }
void Actors::sleep_clearObjective(const ActorIndex& index) { m_mustSleep[index]->clearObjective(); }
void Actors::sleep_maybeClearSpot(const ActorIndex& index) { return m_mustSleep[index]->clearSleepSpot(); }
bool Actors::sleep_isAwake(const ActorIndex& index) const { return m_mustSleep[index]->isAwake(); }
Percent Actors::sleep_getPercentDoneSleeping(const ActorIndex& index) const { return m_mustSleep[index]->getSleepPercent(); }
Percent Actors::sleep_getPercentTired(const ActorIndex& index) const { return m_mustSleep[index]->getTiredPercent(); }
BlockIndex Actors::sleep_getSpot(const ActorIndex& index) const { return m_mustSleep[index]->getLocation(); }
bool Actors::sleep_hasTiredEvent(const ActorIndex& index) const { return m_mustSleep[index]->hasTiredEvent(); }
// Skills.
SkillLevel Actors::skill_getLevel(const ActorIndex& index, const SkillTypeId& skillType) const { return m_skillSet[index]->get(skillType); }
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
