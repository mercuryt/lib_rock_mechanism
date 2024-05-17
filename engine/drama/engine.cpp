#include "engine.h"
#include "../moveType.h"
#include "../area.h"
#include "../actor.h"
#include "../objectives/leaveArea.h"
#include "../simulation.h"
#include "animalSpecies.h"
#include "arcs/animalsArrive.h"
#include "arcs/banditsArrive.h"
#include "deserializationMemo.h"
#include "../simulation/hasAreas.h"
#include <memory>
#include <new>
std::vector<DramaArcType> DramaArc::getTypes()
{
	static std::vector<DramaArcType> output{
		DramaArcType::AnimalsArrive,
		DramaArcType::BanditsArrive,
	};
	return output;
}
NLOHMANN_JSON_SERIALIZE_ENUM(DramaArcType, {
	{DramaArcType::AnimalsArrive, "animals arrive"},
	{DramaArcType::BanditsArrive, "bandits arrive"}
});
std::string DramaArc::typeToString(DramaArcType type)
{
	switch(type)
	{
		case DramaArcType::AnimalsArrive:
			return "animals arrive";
		case DramaArcType::BanditsArrive:
			return "bandits arrive";
	}
	assert(false);
	return "";
}
DramaArcType DramaArc::stringToType(std::string string)
{
	if(string == typeToString(DramaArcType::AnimalsArrive))
		return DramaArcType::AnimalsArrive;
	if(string == typeToString(DramaArcType::BanditsArrive))
		return DramaArcType::BanditsArrive;
	assert(false);
	return DramaArcType::AnimalsArrive;
}
// Static method.
std::unique_ptr<DramaArc> DramaArc::load(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine)
{
	DramaArcType type = data["type"].get<DramaArcType>();
	switch(type)
	{
		case DramaArcType::AnimalsArrive:
			return std::make_unique<AnimalsArriveDramaArc>(data, deserializationMemo, dramaEngine);
			break;
		case DramaArcType::BanditsArrive:
			return std::make_unique<BanditsArriveDramaArc>(data, deserializationMemo, dramaEngine);
			break;
	}
	assert(false);
	return std::make_unique<AnimalsArriveDramaArc>(data, deserializationMemo, dramaEngine);
}
DramaArc::DramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine) : 
	m_engine(dramaEngine), m_type(DramaArc::stringToType(data["type"].get<std::string>()))
{
	if(data.contains("area"))
		m_area = &deserializationMemo.m_simulation.m_hasAreas->getById(data["area"].get<AreaId>());
}
Json DramaArc::toJson() const
{
	Json data{{"type", m_type}};
	if(m_area)
		data["area"] = m_area->m_id;
	return data;
}
void DramaArc::actorsLeave(std::vector<Actor*> actors)
{
	constexpr uint8_t priority = 100;
	for(Actor* actor : actors)
	{
		if(actor->isAlive())
		{
			std::unique_ptr<Objective> objective = std::make_unique<LeaveAreaObjective>(*actor, priority);
			actor->m_hasObjectives.addTaskToStart(std::move(objective));
		}
	}
}
Block* DramaArc::getEntranceToArea(Area& area, const Shape& shape, const MoveType& moveType) const
{
	std::vector<Block*> blocks;
	for(Block& block : area.getBlocks())
		if(block.m_hasShapes.moveTypeCanEnter(moveType) && block.m_isEdge && !block.m_underground)
			blocks.push_back(&block);
	Block* candidate = nullptr;
	auto& random = area.m_simulation.m_random;
	static uint16_t minimumConnectedCount = 200;
	do {
		if(candidate)
		{
			auto iterator = std::ranges::find(blocks, candidate);
			assert(iterator != blocks.end());
			blocks.erase(iterator);
		}
		if(blocks.empty())
			return nullptr;
		candidate = random.getInVector(blocks);
	}
	while (!blockIsConnectedToAtLeast(*candidate, shape, moveType, minimumConnectedCount));
	assert(candidate);
	return candidate;
}
Block* DramaArc::findLocationOnEdgeForNear(const Shape& shape, const MoveType& moveType, Block& origin, DistanceInBlocks distance, std::unordered_set<Block*> exclude) const
{
	Facing facing = getFacingAwayFromEdge(origin);
	auto predicate = [&](Block& thisBlock) -> bool {
		if(exclude.contains(&thisBlock))
			return false;
		// TODO: A single method doing both of these with one iteration would be faster.
		if(!thisBlock.m_hasShapes.shapeAndMoveTypeCanEnterEverWithFacing(shape, moveType, facing) ||
			!thisBlock.m_hasShapes.shapeAndMoveTypeCanEnterCurrentlyWithFacing(shape, moveType, facing)
		)
			return false;
		for(Block* occupiedBlock : shape.getBlocksOccupiedAt(thisBlock, facing))
			if(occupiedBlock->m_isEdge)
				return true;
		return false;
	};
	// Get block in range of origin which satisifies predicate.
	return origin.getBlockInRangeWithCondition(distance, predicate);
}
bool DramaArc::blockIsConnectedToAtLeast(const Block& origin, [[maybe_unused]] const Shape& shape, const MoveType& moveType, uint16_t count) const
{
	std::unordered_set<const Block*> accumulated;
	std::stack<const Block*> open;
	open.push(&origin);
	while(!open.empty())
	{
		const Block* candidate = open.top();
		open.pop();
		if(!accumulated.contains(candidate))
		{
			accumulated.insert(candidate);
			if(accumulated.size() == count)
				return true;
			for(const Block* adjacent : candidate->m_adjacents)
				//TODO: check if shape can fit into block with any facing.
				if(adjacent && !adjacent->isSolid() && adjacent->m_hasShapes.moveTypeCanEnter(moveType))
					open.push(adjacent);
		}
	}
	return false;
}
Facing DramaArc::getFacingAwayFromEdge(const Block& block) const
{
	assert(block.m_isEdge);
	if(!block.getBlockNorth())
		return 2;
	if(!block.getBlockSouth())
		return 0;
	if(!block.getBlockEast())
		return 3;
	if(!block.getBlockWest())
		return 1;
	return 0;
}
std::vector<const AnimalSpecies*> DramaArc::getSentientSpecies() const
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : animalSpeciesDataStore)
		if(species.sentient)
			output.push_back(&species);
	return output;
}
DramaEngine::DramaEngine(const Json& data, DeserializationMemo& deserializationMemo, Simulation& simulation) : m_simulation(simulation)
{
	for(const Json& arcData : data["arcs"])
	{
		std::unique_ptr<DramaArc> arc = DramaArc::load(arcData, deserializationMemo, *this);
		add(std::move(arc));
	}
}
Json DramaEngine::toJson() const
{
	Json arcs = Json::array();
	for(const std::unique_ptr<DramaArc>& arc : m_arcs)
		arcs.push_back(arc->toJson());
	return {{"arcs", arcs}};
}
void DramaEngine::add(std::unique_ptr<DramaArc> dramaticArc)
{
	if(dramaticArc->m_area)
		m_arcsByArea[dramaticArc->m_area].insert(dramaticArc.get());
	m_arcs.push_back(std::move(dramaticArc));
}
void DramaEngine::remove(DramaArc& arc)
{
	if(arc.m_area)
		m_arcsByArea.at(arc.m_area).erase(&arc);
	auto iter = std::ranges::find_if(m_arcs, [&](const auto& e) -> bool { return e.get() == &arc;});
	assert(iter != m_arcs.end());
	m_arcs.erase(iter);
}
void DramaEngine::removeArcsForArea(Area& area)
{
	m_arcsByArea.erase(&area);
}
void DramaEngine::createArcsForArea(Area& area)
{
	std::unique_ptr<DramaArc> arc = std::make_unique<AnimalsArriveDramaArc>(*this, area);
	add(std::move(arc));
}
void DramaEngine::createArcTypeForArea(DramaArcType type, Area& area)
{
	std::unique_ptr<DramaArc> arc;
	switch(type)
	{
		case DramaArcType::AnimalsArrive:
			arc = std::make_unique<AnimalsArriveDramaArc>(*this, area);
			break;
		case DramaArcType::BanditsArrive:
			arc = std::make_unique<BanditsArriveDramaArc>(*this, area);
			break;
	}
	add(std::move(arc));
}
void DramaEngine::removeArcTypeFromArea(DramaArcType type, Area& area)
{
	assert(m_arcsByArea.contains(&area));
	for(DramaArc* arc : m_arcsByArea.at(&area))
		if(arc->m_type == type)
		{
			remove(*arc);
			return;
		}
}
std::unordered_set<DramaArc*>& DramaEngine::getArcsForArea(Area& area)
{ 
	return m_arcsByArea[&area]; 
}
DramaEngine::~DramaEngine()
{
	m_arcs.clear();
}
