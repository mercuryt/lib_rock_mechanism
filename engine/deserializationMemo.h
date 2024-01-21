#pragma once

#include "config.h"
#include "worldforge/worldLocation.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

class Project;
class HaulSubproject;
class Reservable;
class CanReserve;
class Simulation;
class HasShape;
struct ProjectRequirementCounts;
class StockPile;
class Faction;
struct CraftJob;
class Objective;
struct ObjectiveType;
class Plant;
class Block;
class Item;
struct Uniform;
struct WorldLocation;

struct DeserializationMemo final
{
	Simulation& m_simulation;
	std::unordered_map<uintptr_t, Project*> m_projects;
	std::unordered_map<uintptr_t, HaulSubproject*> m_haulSubprojects;
	std::unordered_map<uintptr_t, Reservable*> m_reservables;
	std::unordered_map<uintptr_t, CanReserve*> m_canReserves;
	std::unordered_map<uintptr_t, HasShape*> m_hasShapes;
	std::unordered_map<uintptr_t, ProjectRequirementCounts*> m_projectRequirementCounts;
	std::unordered_map<uintptr_t, StockPile*> m_stockpiles;
	std::unordered_map<uintptr_t, CraftJob*> m_craftJobs;
	std::unordered_map<uintptr_t, Objective*> m_objectives;
	std::unordered_map<uintptr_t, Uniform*> m_uniform;
	Faction& faction(std::wstring name);
	Plant& plantReference(const Json& data);
	Block& blockReference(const Json& data);
	Item& itemReference(const Json& data);
	HasShape& hasShapeReference(const Json& data);
	ProjectRequirementCounts& projectRequirementCountsReference(const Json& data);
	WorldLocation& getLocationByNormalizedLatLng(const Json& data);
	std::unique_ptr<Objective> loadObjective(const Json& data);
	std::unique_ptr<ObjectiveType> loadObjectiveType(const Json& data);
	DeserializationMemo(Simulation& simulation) : m_simulation(simulation) { }
};
