#include "deserializationMemo.h"
#include "simulation.h"
#include "targetedHaul.h"
#include "objective.h"
#include "objectives/givePlantsFluid.h"
#include "objectives/goTo.h"
#include "objectives/harvest.h"
#include "objectives/kill.h"
#include "objectives/sowSeeds.h"
#include "objectives/station.h"
#include "objectives/wait.h"
#include "objectives/wander.h"
#include "objectives/rest.h"
//#include "worldforge/world.h"
#include <cstdint>
Faction& DeserializationMemo::faction(std::wstring name) { return m_simulation.m_hasFactions.byName(name); }
Plant& DeserializationMemo::plantReference(const Json& data) { return m_simulation.getBlockForJsonQuery(data).m_hasPlant.get(); }
Block& DeserializationMemo::blockReference(const Json& data) { return m_simulation.getBlockForJsonQuery(data); }
Item& DeserializationMemo::itemReference(const Json& data) { return m_simulation.getItemById(data.get<ItemId>()); }
Actor& DeserializationMemo::actorReference(const Json& data) { return m_simulation.getActorById(data.get<ActorId>()); }
ProjectRequirementCounts& DeserializationMemo::projectRequirementCountsReference(const Json& data) { return *m_projectRequirementCounts.at(data.get<uintptr_t>()); }
//WorldLocation& DeserializationMemo::getLocationByNormalizedLatLng(const Json& data) { return m_simulation.m_world->getLocationByNormalizedLatLng(data.get<LatLng>()); }
std::unique_ptr<Objective> DeserializationMemo::loadObjective(const Json& data)
{
	ObjectiveTypeId typeId = data["type"].get<ObjectiveTypeId>();
	std::unique_ptr<Objective> output;
	switch(typeId)
	{
		case ObjectiveTypeId::Construct:
			output = std::make_unique<ConstructObjective>(data, *this);
			break;
		case ObjectiveTypeId::Craft:
			output = std::make_unique<CraftObjective>(data, *this);
			break;
		case ObjectiveTypeId::Dig:
			output = std::make_unique<DigObjective>(data, *this);
			break;
		case ObjectiveTypeId::Drink:
			output =  std::make_unique<DrinkObjective>(data, *this);
			break;
		case ObjectiveTypeId::Eat:
			output =  std::make_unique<EatObjective>(data, *this);
			break;
		case ObjectiveTypeId::GetToSafeTemperature:
			output =  std::make_unique<GetToSafeTemperatureObjective>(data, *this);
			break;
		case ObjectiveTypeId::GivePlantsFluid:
			output =  std::make_unique<GivePlantsFluidObjective>(data, *this);
			break;
		case ObjectiveTypeId::GoTo:
			output =  std::make_unique<GoToObjective>(data, *this);
			break;
		case ObjectiveTypeId::Harvest:
			output =  std::make_unique<HarvestObjective>(data, *this);
			break;
		case ObjectiveTypeId::Haul:
			output =  std::make_unique<TargetedHaulObjective>(data, *this);
			break;
		case ObjectiveTypeId::Kill:
			output =  std::make_unique<KillObjective>(data, *this);
			break;
		//case ObjectiveTypeId::Medical:
			//output =  std::make_unique<MedicalObjective>(data, *this);
			//break;
		case ObjectiveTypeId::Rest:
			output =  std::make_unique<RestObjective>(data, *this);
			break;
		case ObjectiveTypeId::Sleep:
			output =  std::make_unique<SleepObjective>(data, *this);
			break;
		case ObjectiveTypeId::Station:
			output =  std::make_unique<StationObjective>(data, *this);
			break;
		case ObjectiveTypeId::SowSeeds:
			output =  std::make_unique<SowSeedsObjective>(data, *this);
			break;
		case ObjectiveTypeId::StockPile:
			output =  std::make_unique<StockPileObjective>(data, *this);
			break;
		case ObjectiveTypeId::Wait:
			output =  std::make_unique<WaitObjective>(data, *this);
			break;
		default:
			assert(typeId == ObjectiveTypeId::Wander);
			output =  std::make_unique<WanderObjective>(data, *this);
	}
	m_objectives[data["address"].get<uintptr_t>()] = output.get();
	return output;
}
std::unique_ptr<ObjectiveType> DeserializationMemo::loadObjectiveType(const Json& data)
{
	ObjectiveTypeId typeId = data["type"].get<ObjectiveTypeId>();
	switch(typeId)
	{
		case ObjectiveTypeId::Construct:
			return std::make_unique<ConstructObjectiveType>(data, *this);
		case ObjectiveTypeId::Craft:
			return std::make_unique<CraftObjectiveType>(data, *this);
		case ObjectiveTypeId::Dig:
			return std::make_unique<DigObjectiveType>(data, *this);
		case ObjectiveTypeId::GivePlantsFluid:
			return std::make_unique<GivePlantsFluidObjectiveType>(data, *this);
		case ObjectiveTypeId::Harvest:
			return std::make_unique<HarvestObjectiveType>(data, *this);
		//case ObjectiveTypeId::Medical:
			//return std::make_unique<MedicalObjectiveType>(data, *this);
		case ObjectiveTypeId::SowSeeds:
			return std::make_unique<SowSeedsObjectiveType>(data, *this);
		default:
			assert(typeId == ObjectiveTypeId::StockPile);
			return std::make_unique<StockPileObjectiveType>(data, *this);
	}
}
