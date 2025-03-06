#include "deserializationMemo.h"
#include "simulation/simulation.h"
#include "simulation/hasActors.h"
#include "simulation/hasAreas.h"
#include "simulation/hasItems.h"
#include "targetedHaul.h"
#include "construct.h"
#include "dig.h"
#include "craft.h"
#include "stockpile.h"
#include "eat.h"
#include "drink.h"
#include "blocks/blocks.h"
#include "actors/actors.h"
#include "items/items.h"
#include "plants.h"
#include "objective.h"
#include "objectives/construct.h"
#include "objectives/drink.h"
#include "objectives/exterminate.h"
#include "objectives/goTo.h"
#include "objectives/kill.h"
#include "objectives/sleep.h"
#include "objectives/targetedHaul.h"
#include "objectives/wander.h"
#include "objectives/craft.h"
#include "objectives/eat.h"
#include "objectives/giveItem.h"
#include "objectives/getToSafeTemperature.h"
#include "objectives/leaveArea.h"
#include "objectives/sowSeeds.h"
#include "objectives/unequipItem.h"
#include "objectives/woodcutting.h"
#include "objectives/dig.h"
#include "objectives/equipItem.h"
#include "objectives/givePlantsFluid.h"
#include "objectives/harvest.h"
#include "objectives/rest.h"
#include "objectives/station.h"
#include "objectives/stockpile.h"
#include "objectives/wait.h"

//#include "worldforge/world.h"
#include <cstdint>
Area& DeserializationMemo::area(const Json& data) { return m_simulation.m_hasAreas->getById(data.get<AreaId>()); }
ProjectRequirementCounts& DeserializationMemo::projectRequirementCountsReference(const Json& data)
{
	uintptr_t formerAddress = data.get<uintptr_t>();
	assert(m_projectRequirementCounts.contains(formerAddress));
	return *m_projectRequirementCounts[formerAddress];
}
//WorldLocation& DeserializationMemo::getLocationByNormalizedLatLng(const Json& data) { return m_simulation.m_world->getLocationByNormalizedLatLng(data.get<LatLng>()); }
std::unique_ptr<Objective> DeserializationMemo::loadObjective(const Json& data, Area& area, ActorIndex actor)
{
	auto name = data["type"].get<std::wstring>();
	std::unique_ptr<Objective> output;
	if(name == L"construct")
		output = std::make_unique<ConstructObjective>(data, *this);
	else if(name.substr(0, 5) == L"craft")
		output = std::make_unique<CraftObjective>(data, *this);
	else if(name == L"dig")
		output = std::make_unique<DigObjective>(data, *this);
	else if(name == L"drink")
		output =  std::make_unique<DrinkObjective>(data, *this, area, actor);
	else if(name == L"eat")
		output =  std::make_unique<EatObjective>(data, *this, area, actor);
	else if(name == L"get To safe temperature")
		output =  std::make_unique<GetToSafeTemperatureObjective>(data, *this);
	else if(name == L"give plants fluid")
		output =  std::make_unique<GivePlantsFluidObjective>(data, area, actor, *this);
	else if(name == L"go to")
		output =  std::make_unique<GoToObjective>(data, *this);
	else if(name == L"harvest")
		output =  std::make_unique<HarvestObjective>(data, area, *this);
	else if(name == L"haul")
		output =  std::make_unique<TargetedHaulObjective>(data, *this);
	else if(name == L"kill")
		output =  std::make_unique<KillObjective>(data, *this, area);
	//else if(name == L"medical")
		//output =  std::make_unique<MedicalObjective>(data, *this);
	else if(name == L"rest")
		output =  std::make_unique<RestObjective>(data, area, actor, *this);
	else if(name == L"sleep")
		output =  std::make_unique<SleepObjective>(data, *this);
	else if(name == L"station")
		output =  std::make_unique<StationObjective>(data, *this);
	else if(name == L"sow seeds")
		output =  std::make_unique<SowSeedsObjective>(data, area, actor, *this);
	else if(name == L"stockpile")
		output =  std::make_unique<StockPileObjective>(data, *this, area);
	else if(name == L"wait")
		output =  std::make_unique<WaitObjective>(data, area, actor, *this);
	else
	{
		assert(name == L"wander");
		output = std::make_unique<WanderObjective>(data, *this);
	}
	m_objectives[data["address"].get<uintptr_t>()] = output.get();
	return output;
}
