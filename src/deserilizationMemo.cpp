#include "deserilizationMemo.h"
#include "construct.h"
#include "simulation.h"
#include "objective.h"
#include "goTo.h"
#include "harvest.h"
#include "givePlantsFluid.h"
#include "kill.h"
#include "targetedHaul.h"
#include "sowSeeds.h"
#include "station.h"
#include "wait.h"
#include "wander.h"
#include "goTo.h"
Faction& DeserilizationMemo::faction(std::wstring name) { return m_simulation.m_hasFactions.byName(name); }
std::unique_ptr<Objective> DeserilizationMemo::loadObjective(const Json& data)
{
	ObjectiveTypeId typeId = data["type"].get<ObjectiveTypeId>();
	switch(typeId)
	{
		case ObjectiveTypeId::Construct:
			return std::make_unique<ConstructObjective>(data, *this);
		case ObjectiveTypeId::Craft:
			return std::make_unique<CraftObjective>(data, *this);
		case ObjectiveTypeId::Dig:
			return std::make_unique<DigObjective>(data, *this);
		case ObjectiveTypeId::Drink:
			return std::make_unique<DrinkObjective>(data, *this);
		case ObjectiveTypeId::Eat:
			return std::make_unique<EatObjective>(data, *this);
		case ObjectiveTypeId::GetToSafeTemperature:
			return std::make_unique<GetToSafeTemperatureObjective>(data, *this);
		case ObjectiveTypeId::GivePlantsFluid:
			return std::make_unique<GivePlantsFluidObjective>(data, *this);
		case ObjectiveTypeId::GoTo:
			return std::make_unique<GoToObjective>(data, *this);
		case ObjectiveTypeId::Harvest:
			return std::make_unique<HarvestObjective>(data, *this);
		case ObjectiveTypeId::Haul:
			return std::make_unique<TargetedHaulObjective>(data, *this);
		case ObjectiveTypeId::Kill:
			return std::make_unique<KillObjective>(data, *this);
		//case ObjectiveTypeId::Medical:
			//return std::make_unique<MedicalObjective>(data, *this);
		case ObjectiveTypeId::Rest:
			return std::make_unique<RestObjective>(data, *this);
		case ObjectiveTypeId::Sleep:
			return std::make_unique<SleepObjective>(data, *this);
		case ObjectiveTypeId::Station:
			return std::make_unique<StationObjective>(data, *this);
		case ObjectiveTypeId::SowSeeds:
			return std::make_unique<SowSeedsObjective>(data, *this);
		case ObjectiveTypeId::StockPile:
			return std::make_unique<StockPileObjective>(data, *this);
		case ObjectiveTypeId::Wait:
			return std::make_unique<WaitObjective>(data, *this);
		default:
			assert(typeId == ObjectiveTypeId::Wander);
			return std::make_unique<WanderObjective>(data, *this);
	}
}
std::unique_ptr<ObjectiveType> DeserilizationMemo::loadObjectiveType(const Json& data)
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
