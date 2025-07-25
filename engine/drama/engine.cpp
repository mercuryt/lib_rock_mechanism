#include "engine.h"
#include "../definitions/moveType.h"
#include "../area/area.h"
#include "../objectives/leaveArea.h"
#include "../simulation/simulation.h"
#include "../actors/actors.h"
#include "../definitions/animalSpecies.h"
#include "arcs/animalsArrive.h"
#include "arcs/banditsArrive.h"
#include "../space/space.h"
#include "../deserializationMemo.h"
#include "../simulation/hasAreas.h"
#include "../numericTypes/types.h"
#include "../plants.h"
#include "../items/items.h"
#include "../util.h"
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
	std::unreachable();
	return "";
}
DramaArcType DramaArc::stringToType(std::string string)
{
	if(string == typeToString(DramaArcType::AnimalsArrive))
		return DramaArcType::AnimalsArrive;
	if(string == typeToString(DramaArcType::BanditsArrive))
		return DramaArcType::BanditsArrive;
	std::unreachable();
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
	std::unreachable();
	return std::make_unique<AnimalsArriveDramaArc>(data, deserializationMemo, dramaEngine);
}
DramaArc::DramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine) :
	m_engine(dramaEngine), m_type(DramaArc::stringToType(data["type"].get<std::string>()))
{
	if(data.contains("area"))
	{
		m_area = &deserializationMemo.m_simulation.m_hasAreas->getById(data["area"].get<AreaId>());
		assert(m_area != nullptr);
	}
}
Json DramaArc::toJson() const
{
	Json data{{"type", m_type}};
	if(m_area)
		data["area"] = m_area->m_id;
	return data;
}
void DramaArc::actorsLeave(SmallSet<ActorIndex> actorsLeaving)
{
	constexpr Priority priority = Priority::create(100);
	Actors& actors = m_area->getActors();
	for(ActorIndex actor : actorsLeaving)
	{
		if(actors.isAlive(actor))
		{
			std::unique_ptr<Objective> objective = std::make_unique<LeaveAreaObjective>(priority);
			actors.objective_addTaskToStart(actor, std::move(objective));
		}
	}
}
Point3D DramaArc::getEntranceToArea(const ShapeId& shape, const MoveTypeId& moveType) const
{
	SmallSet<Point3D> candidates;
	Space& space = m_area->getSpace();
	// TODO: optimize this: only check faces of getAll() cuboid.
	Cuboid cuboid = space.boundry();
	for(const Point3D& point : cuboid)
	{
		if(space.shape_moveTypeCanEnter(point, moveType) && space.isEdge(point) && space.isExposedToSky(point))
			candidates.insert(point);
	}
	Point3D candidate;
	static uint16_t minimumConnectedCount = 200;
	do {
		if(candidate.exists())
		{
			auto iterator = candidates.find(candidate);
			assert(iterator != candidates.end());
			candidates.erase(*iterator);
		}
		if(candidates.empty())
			return Point3D::null();
		candidate = candidates[m_area->m_simulation.m_random.getInRange(0u, candidates.size() - 1u)];
	}
	while (!pointIsConnectedToAtLeast(candidate, shape, moveType, minimumConnectedCount));
	assert(candidate.exists());
	return candidate;
}
Point3D DramaArc::findLocationOnEdgeForNear(const ShapeId& shape, const MoveTypeId& moveType, const Point3D& origin, const Distance& distance, const SmallSet<Point3D>& exclude) const
{
	Facing4 facing = getFacingAwayFromEdge(origin);
	Space& space = m_area->getSpace();
	auto predicate = [&](const Point3D& thisPoint) -> bool {
		if(exclude.contains(thisPoint))
			return false;
		// TODO: A single method doing both of these with one iteration would be faster.
		if(!space.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(thisPoint, shape, moveType, facing, {}))
			return false;
		for(Point3D occupiedPoint : Shape::getPointsOccupiedAt(shape, space, thisPoint, facing))
			if(space.isEdge(occupiedPoint))
				return true;
		return false;
	};
	// Get point in range of origin which satisifies predicate.
	return space.getPointInRangeWithCondition(origin, distance, predicate);
}
bool DramaArc::pointIsConnectedToAtLeast(const Point3D& origin, [[maybe_unused]] const ShapeId& shape, const MoveTypeId& moveType, uint16_t count) const
{
	SmallSet<Point3D> accumulated;
	std::stack<Point3D> open;
	open.push(origin);
	Space& space = m_area->getSpace();
	while(!open.empty())
	{
		Point3D candidate = open.top();
		open.pop();
		if(!accumulated.contains(candidate))
		{
			accumulated.insert(candidate);
			if(accumulated.size() == count)
				return true;
			for(const Point3D& adjacent : space.getDirectlyAdjacent(candidate))
				//TODO: check if shape can fit into point with any facing.
				if(!space.solid_is(adjacent) && space.shape_moveTypeCanEnter(adjacent, moveType))
					open.push(adjacent);
		}
	}
	return false;
}
Facing4 DramaArc::getFacingAwayFromEdge(const Point3D& point) const
{
	Space& space = m_area->getSpace();
	assert(space.isEdge(point));
	if(point.y() == 0)
		return Facing4::South;
	if(point.y() == space.m_dimensions[1])
		return Facing4::North;
	if(point.x() == space.m_dimensions[0])
		return Facing4::West;
	if(point.x() == 0)
		return Facing4::East;
	return Facing4::North;
}
std::vector<AnimalSpeciesId> DramaArc::getSentientSpecies() const
{
	std::vector<AnimalSpeciesId> output;
	for(AnimalSpeciesId i = AnimalSpeciesId::create(0); i < AnimalSpecies::size(); ++i)
		if(AnimalSpecies::getSentient(i))
			output.push_back(i);
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
		util::addUniqueToVectorAssert(m_arcsByArea.getOrCreate(dramaticArc->m_area->m_id), dramaticArc.get());
	m_arcs.push_back(std::move(dramaticArc));
}
void DramaEngine::remove(DramaArc& arc)
{
	if(arc.m_area)
		util::removeFromVectorByValueUnordered(m_arcsByArea[arc.m_area->m_id], &arc);
	auto iter = std::ranges::find_if(m_arcs, [&](const auto& e) -> bool { return e.get() == &arc;});
	assert(iter != m_arcs.end());
	m_arcs.erase(iter);
}
void DramaEngine::removeArcsForArea(Area& area)
{
	m_arcsByArea.erase(area.m_id);
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
	assert(m_arcsByArea.contains(area.m_id));
	for(DramaArc* arc : m_arcsByArea[area.m_id])
		if(arc->m_type == type)
		{
			remove(*arc);
			return;
		}
}
std::vector<DramaArc*>& DramaEngine::getArcsForArea(Area& area)
{
	return m_arcsByArea[area.m_id];
}
DramaEngine::~DramaEngine()
{
	m_arcs.clear();
}
