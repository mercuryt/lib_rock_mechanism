#include "items.h"
#include "../definitions/itemType.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../simulation/simulation.h"
#include "../simulation/hasItems.h"
#include "../util.h"
#include "../actors/actors.h"
#include "../numericTypes/index.h"
#include "../definitions/moveType.h"
#include "../numericTypes/types.h"
#include "../portables.h"
#include <memory>
#include <ranges>
// RemarkItemForStockPilingEvent
ReMarkItemForStockPilingEvent::ReMarkItemForStockPilingEvent(Area& area, ItemCanBeStockPiled& canBeStockPiled, FactionId faction, Step duration, Step start) :
	ScheduledEvent(area.m_simulation, duration, start),
	m_faction(faction),
	m_canBeStockPiled(&canBeStockPiled) { }
void ReMarkItemForStockPilingEvent::execute(Simulation&, Area*)
{
	m_canBeStockPiled->maybeSet(m_faction);
	m_canBeStockPiled->m_scheduledEvents.erase(m_faction);
}
void ReMarkItemForStockPilingEvent::clearReferences(Simulation&, Area*) { }
// ItemCanBeStockPiled
void ItemCanBeStockPiled::load(const Json& data, Area& area)
{
	if(data.contains("data"))
		for(const Json& faction : data["data"])
			m_data.insert(faction.get<FactionId>());
	if(data.contains("scheduledEvents"))
		for(const Json& pair : data["scheduledEvents"])
		{
			FactionId faction = pair[0].get<FactionId>();
			Step start = pair[1]["start"].get<Step>();
			Step duration = pair[1]["duration"].get<Step>();
			scheduleReset(area, faction, duration, start);
		}
}
Json ItemCanBeStockPiled::toJson() const
{
	Json data;
	if(!m_data.empty())
	{
		data["data"] = Json::array();
		for(FactionId faction : m_data)
			data["data"].push_back(faction);
		if(!m_scheduledEvents.empty())
		{
			data["scheduledEvents"] = Json::array();
			for(auto [faction, event] : m_scheduledEvents)
			{
				Json pair;
				pair[0] = faction;
				pair[1] = Json();
				pair[1]["start"] = event.getStartStep();
				pair[1]["duration"] = event.duration();
				data["scheduledEvents"].push_back(pair);
			}
		}
	}
	return data;
}
void ItemCanBeStockPiled::scheduleReset(Area& area, FactionId faction, Step duration, Step start)
{
	assert(!m_scheduledEvents.contains(faction));
	auto pair = m_scheduledEvents.emplace(faction, area.m_eventSchedule);
	HasScheduledEvent<ReMarkItemForStockPilingEvent>& eventHandle = pair;
	eventHandle.schedule(area, *this, faction, duration, start);
}
void ItemCanBeStockPiled::unsetAndScheduleReset(Area& area, FactionId faction, Step duration)
{
	unset(faction);
	scheduleReset(area, faction, duration);
}
void ItemCanBeStockPiled::maybeUnsetAndScheduleReset(Area& area, FactionId faction, Step duration)
{
	maybeUnset(faction);
	scheduleReset(area, faction, duration);
}
Items::Items(Area& area) :
	Portables(area)
{ }
ItemIndex Items::create(ItemParamaters itemParamaters)
{
	// Detect stacks combining, add to quantity and bail without creating a new item.
	if(ItemType::getGeneric(itemParamaters.itemType))
	{
		assert(itemParamaters.quality.empty());
		assert(itemParamaters.percentWear.empty());
		if(itemParamaters.location.exists())
		{
			ItemIndex toCombineWith = m_area.getSpace().item_getOneWithCondition(itemParamaters.location, [this, &itemParamaters](ItemIndex item){
				return getItemType(item) == itemParamaters.itemType && getMaterialType(item) == itemParamaters.materialType;
			});
			if(toCombineWith.exists())
			{
				addQuantity(toCombineWith, itemParamaters.quantity);
				return toCombineWith;
			}
		}
	}
	else
		assert(itemParamaters.quantity == 1);
	ItemIndex index = ItemIndex::create(size());
	// TODO: This 'toItem' call should not be neccessary. Why does ItemIndex + int = HasShapeIndex?
	resize(index + 1);
	const MoveTypeId moveType = ItemType::getMoveType(itemParamaters.itemType);
	ShapeId shape = ItemType::getShape(itemParamaters.itemType);
	if(itemParamaters.quantity > 1)
		shape = Shape::mutateMultiplyVolume(shape, itemParamaters.quantity);
	Portables<Items, ItemIndex, ItemReferenceIndex, false>::create(index, moveType, shape, itemParamaters.faction, itemParamaters.isStatic, itemParamaters.quantity);
	assert(m_canBeStockPiled[index] == nullptr);
	m_craftJobForWorkPiece[index] = itemParamaters.craftJob;
	assert(m_hasCargo[index] == nullptr);
	m_id[index] = itemParamaters.id.exists() ? itemParamaters.id : m_area.m_simulation.m_items.getNextId();
	m_installed.set(index, itemParamaters.installed);
	ItemTypeId itemType = m_itemType[index] = itemParamaters.itemType;
	m_solid[index] = itemParamaters.materialType;
	m_name[index] = itemParamaters.name;
	m_percentWear[index] = itemParamaters.percentWear;
	m_quality[index] = itemParamaters.quality;
	m_quantity[index] = itemParamaters.quantity;
	const auto& constructedShape = ItemType::getConstructedShape(itemType);
	if(constructedShape != nullptr)
	{
		// Make a copy so we can rotate m_solidPoints and m_features.
		m_constructedShape[index] = std::make_unique<ConstructedShape>(*constructedShape);
		// Include decks for pathing purposes.
		m_compoundShape[index] = m_constructedShape[index]->getShapeIncludingDecks();
	}
	if(itemParamaters.location.exists())
		// A generic item type might merge with an existing stack on creation, in that case return the index of the existing stack.
		index = location_set(index, itemParamaters.location, itemParamaters.facing);
	static const MoveTypeId rolling = MoveType::byName("roll");
	static ItemTypeId panniers = ItemType::byName("panniers");
	if(itemType == panniers || (moveType == rolling && ItemType::getInternalVolume(itemType) != 0))
		m_area.m_hasHaulTools.registerHaulTool(m_area, index);
	return index;
}
void Items::moveIndex(ItemIndex oldIndex, ItemIndex newIndex)
{
	forEachData([&](auto& data){ data.moveIndex(oldIndex, newIndex); });
	updateStoredIndicesPortables(oldIndex, newIndex);
	if(m_hasCargo[newIndex] != nullptr)
		m_hasCargo[newIndex]->updateCarrierIndexForAllCargo(m_area, newIndex);
	if(m_onSurface[oldIndex])
	{
		// No need to update Area::m_hasTemperatures here, just bookkeeping.
		m_onSurface.maybeUnset(oldIndex);
		m_onSurface.set(newIndex);
	}
	if(hasLocation(newIndex))
	{
		Space& space = m_area.getSpace();
		const Cuboid boundry = this->boundry(newIndex);
		space.item_updateIndex(boundry, oldIndex, newIndex);
	}
}
void Items::addQuantity(ItemIndex index, Quantity delta)
{
	assert(isStatic(index));
	assert(delta != 0);
	Point3D location = m_location[index];
	Facing4 facing = m_facing[index];
	Quantity newQuantity = getQuantity(index) + delta;
	// TODO: Update in place rather then exit, update, enter.
	if(location.exists())
	{
		location_clearStatic(index);
		setQuantity(index, newQuantity);
		location_setStatic(index, location, facing);
	}
	else
		setQuantity(index, newQuantity);
}
// May destroy.
void Items::removeQuantity(ItemIndex index, Quantity delta, CanReserve* canReserve)
{
	assert(isStatic(index));
	if(canReserve != nullptr)
		reservable_maybeUnreserve(index, *canReserve, delta);
	if(m_quantity[index] == delta)
		remove(index);
	else
	{
		assert(delta < m_quantity[index]);
		// TODO: Update in place rather then exit, update, enter.
		Point3D location = m_location[index];
		if(location.exists())
		{
			Facing4 facing = m_facing[index];
			location_clearStatic(index);
			setQuantity(index, getQuantity(index) - delta);
			location_setStatic(index, location, facing);
		}
		else
			setQuantity(index, getQuantity(index) - delta);
		if(m_reservables[index] != nullptr)
			m_reservables[index]->setMaxReservations(m_quantity[index]);
	}
}
void Items::install(ItemIndex index, Point3D point, Facing4 facing, FactionId faction)
{
	maybeSetStatic(index);
	location_setStatic(index, point, facing);
	m_installed.set(index);
	if(ItemType::getCraftLocationStepTypeCategory(m_itemType[index]).exists())
	{
		ItemTypeId item = m_itemType[index];
		Point3D craftLocation = ItemType::getCraftLocation(item, point, facing);
		if(craftLocation.exists())
			m_area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addLocation(ItemType::getCraftLocationStepTypeCategory(item), craftLocation);
	}
}
ItemIndex Items::merge(ItemIndex index, ItemIndex other)
{
	assert(isStatic(index));
	assert(isStatic(other));
	assert(index != other);
	assert(m_itemType[index] == m_itemType[other]);
	assert(m_solid[index] == m_solid[other]);
	assert(m_location[other].empty());
	if(m_reservables[other] != nullptr)
		reservable_merge(index, *m_reservables[other]);
	addQuantity(index, getQuantity(other));
	assert(m_quantity[index] == m_reservables[index]->getMaxReservations());
	if(m_destroy[other] != nullptr)
		onDestroy_merge(index, *m_destroy[other]);
	// Store a reference to index because it's ItemIndex may change when other is destroyed.
	ItemReference ref = m_area.getItems().m_referenceData.getReference(index);
	remove(other);
	return ref.getIndex(m_referenceData);
}
void Items::setQuality(ItemIndex index, Quality quality)
{
	m_quality[index] = quality;
}
void Items::setWear(ItemIndex index, Percent wear)
{
	m_percentWear[index] = wear;
}
void Items::setQuantity(ItemIndex index, Quantity quantity)
{
	setShape(index, Shape::mutateMultiplyVolume(ItemType::getShape(m_itemType[index]), quantity));
	m_quantity[index] = quantity;
}
void Items::unsetCraftJobForWorkPiece(ItemIndex index)
{
	m_craftJobForWorkPiece[index] = nullptr;
}
void Items::resetMoveType(ItemIndex index)
{
	m_moveType[index] = ItemType::getMoveType(m_itemType[index]);
}
void Items::setStatic(ItemIndex index)
{
	const auto& constructed = m_constructedShape[index];
	if(constructed != nullptr)
	{
		Space& space = m_area.getSpace();
		for(const Cuboid cuboid : m_occupied[index])
			space.unsetDynamic(cuboid);
		m_static.set(index);
	}
	else
		HasShapes<Items, ItemIndex>::setStatic(index);
}
void Items::unsetStatic(ItemIndex index)
{
	const auto& constructed = m_constructedShape[index];
	if(constructed != nullptr)
	{
		Space& space = m_area.getSpace();
		for(const Cuboid cuboid : m_occupied[index])
			space.setDynamic(cuboid);
		m_static.unset(index);
	}
	else
		HasShapes<Items, ItemIndex>::unsetStatic(index);
}
void Items::setOnSurface(ItemIndex index, const bool value)
{
	HasShapes::setOnSurface(index, value);
	if(value)
		m_area.m_hasTemperature.addItemAboveGround(m_area, index);
	else
		m_area.m_hasTemperature.removeItemAboveGround(m_area, index);
}
void Items::moveQuantity(ItemIndex index, Quantity quantity, Point3D destination)
{
	if(m_quantity[index] == quantity)
	{
		location_set(index, destination, m_facing[index]);
		return;
	}
	removeQuantity(index, quantity);
	create({
		.itemType = m_itemType[index],
		.materialType = m_solid[index],
		.location = destination,
		.quantity = quantity,
	});
}
ItemIndex Items::moveTo(Items& other, ItemIndex index)
{
	ItemIndex output = Portables<Items, ItemIndex, ItemReferenceIndex, false>::moveTo(other, index);
	// Discard canBeStockPiled data, it relates to space.
	other.m_canBeStockPiled.add(std::make_unique<ItemCanBeStockPiled>());
	other.m_craftJobForWorkPiece.add();
	other.m_hasCargo.add(std::move(m_hasCargo[index]));
	other.m_hasCargo.back()->moveContentsFromTo(m_area, other.m_area, output);
	other.m_id.add(m_id[index]);
	other.m_installed.add(m_installed[index]);
	other.m_itemType.add(m_itemType[index]);
	other.m_solid.add(m_solid[index]);
	other.m_name.add(m_name[index]);
	other.m_percentWear.add(m_percentWear[index]);
	other.m_quality.add(m_quality[index]);
	other.m_quantity.add(m_quantity[index]);
	other.m_onSurface.add(true);
	// The new index of the pilot is not known yet.
	other.m_pilot.add();
	other.m_constructedShape.add(std::move(m_constructedShape[index]));
	remove(index);
	return output;
}
void Items::remove(ItemIndex index)
{
	// No need to explicitly unschedule events here, destorying the event holder will do it.
	if(hasLocation(index))
		location_clear(index);
	static const MoveTypeId rolling = MoveType::byName("roll");
	if(m_moveType[index] == rolling && ItemType::getInternalVolume(m_itemType[index]) != 0)
		m_area.m_hasHaulTools.unregisterHaulTool(m_area, index);
	ItemIndex s = ItemIndex::create(size() - 1);
	if(index != s)
		moveIndex(s, index);
	m_area.m_hasStockPiles.removeItemFromAllFactions(index);
	onRemove(index);
	resize(s);
	// Will do the same move / resize logic internally, so stays in sync with moves from the DataVectors.
	m_referenceData.remove(index);
}
void Items::removeAll(const SmallSet<ItemIndex>& indices)
{
	// Turn indices to references because indices are invalidated by destroy.
	SmallSet<ItemReference> references;
	references.reserve(indices.size());
	for(ItemIndex index : indices)
		references.insert(getReference(index));
	for(const ItemReference ref : references)
		// Turn refernce back to index to send to destroy.
		remove(ref.getIndex(m_referenceData));

}
bool Items::isGeneric(ItemIndex index) const { return ItemType::getGeneric(m_itemType[index]); }
bool Items::isPreparedMeal(ItemIndex index) const
{
	static ItemTypeId preparedMealType = ItemType::byName("prepared meal");
	return m_itemType[index] == preparedMealType;
}
CraftJob& Items::getCraftJobForWorkPiece(ItemIndex index) const
{
	assert(isWorkPiece(index));
	return *m_craftJobForWorkPiece[index];
}
Mass Items::getSingleUnitMass(ItemIndex index) const
{
	return Mass::create(std::max(1, (ItemType::getFullDisplacement(m_itemType[index]) * MaterialType::getDensity(m_solid[index])).get()));
}
Mass Items::getMass(ItemIndex index) const
{
	MaterialTypeId materialType = m_solid[index];
	Mass output;
	if(materialType.exists())
		output = ItemType::getFullDisplacement(m_itemType[index]) * MaterialType::getDensity(materialType) * m_quantity[index];
	else
		output = m_constructedShape[index]->getMass();
	if(m_hasCargo[index] != nullptr)
		output += m_hasCargo[index]->getMass();
	output += onDeck_getMass(index);
	return output;
}
FullDisplacement Items::getVolume(ItemIndex index) const
{
	return ItemType::getFullDisplacement(m_itemType[index]) * m_quantity[index];
}
MoveTypeId Items::getMoveType(ItemIndex index) const
{
	return m_moveType[index];
}
bool Items::canCombine(ItemIndex index, ItemIndex toMerge) const
{
	if(!isStatic(toMerge))
		return false;
	return m_area.getSpace().shape_staticCanEnterCurrentlyWithFacing(getLocation(index), getShape(toMerge), getFacing(index), {});
}
bool Items::canMelt(ItemIndex index) const
{
	MaterialTypeId materialType = getMaterialType(index);
	if(materialType.exists())
		return MaterialType::canMelt(materialType);
	else
		// Is constructed shape.
		return m_constructedShape[index]->canMelt();
}
std::string Items::description(ItemIndex index) const
{
	if(!m_name[index].empty())
		return m_name[index];
	if(isGeneric(index))
		return ItemType::getName(m_itemType[index]) + "(quantity: " + m_quantity[index].toS() + ")";
	return ItemType::getName(m_itemType[index]) + "(quality: " + m_quality[index].toS() + ", wear: " + m_percentWear[index].toS() + "%)";
}
void Items::log(ItemIndex index) const
{
	std::cout << ItemType::getName(m_itemType[index]) << "[" << MaterialType::getName(m_solid[index]) << "]";
	if(m_quantity[index] != 1)
		std::cout << "(" << m_quantity[index].get() << ")";
	if(m_craftJobForWorkPiece[index] != nullptr)
		std::cout << "{" << m_craftJobForWorkPiece[index]->getStep().get() << "}";
	Portables<Items, ItemIndex, ItemReferenceIndex, false>::log(index);
	std::cout << std::endl;
}
// Wrapper methods.
void Items::stockpile_maybeUnsetAndScheduleReset(ItemIndex index, FactionId faction, Step duration)
{
	if(m_canBeStockPiled[index] != nullptr)
		m_canBeStockPiled[index]->maybeUnsetAndScheduleReset(m_area, faction, duration);
}
void Items::stockpile_set(ItemIndex index, FactionId faction)
{
	if(m_canBeStockPiled[index] == nullptr)
		m_canBeStockPiled[index] = std::make_unique<ItemCanBeStockPiled>();
	m_canBeStockPiled[index]->set(faction);
}
void Items::stockpile_maybeUnset(ItemIndex index, FactionId faction)
{
	if(m_canBeStockPiled[index] != nullptr)
	{
		m_canBeStockPiled[index]->maybeUnset(faction);
		if(m_canBeStockPiled[index]->empty())
			m_canBeStockPiled[index] = nullptr;
	}
}
bool Items::stockpile_canBeStockPiled(ItemIndex index, FactionId faction) const
{
	if(m_canBeStockPiled[index] == nullptr)
		return false;
	return m_canBeStockPiled[index]->contains(faction);
}
void Items::load(const Json& data)
{
	static_cast<Portables<Items, ItemIndex, ItemReferenceIndex, false>*>(this)->load(data);
	data["onSurface"].get_to(m_onSurface);
	data["name"].get_to(m_name);
	data["itemType"].get_to(m_itemType);
	data["materialType"].get_to(m_solid);
	data["id"].get_to(m_id);
	data["quality"].get_to(m_quality);
	data["quantity"].get_to(m_quantity);
	data["percentWear"].get_to(m_percentWear);
	data["pilot"].get_to(m_pilot);
	m_hasCargo.resize(m_id.size());
	Space& space = m_area.getSpace();
	for(ItemIndex index : getAll())
	{
		m_area.m_simulation.m_items.registerItem(m_id[index], m_area.getItems(), index);
		Point3D location = m_location[index];
		if(location.exists())
		{
			const MapWithCuboidKeys<CollisionVolume> toOccupy = Shape::getCuboidsOccupiedAtWithVolume(m_shape[index], space, location, m_facing[index]);
			if(m_static[index])
				space.item_recordStatic(toOccupy, index);
			else
				space.item_recordDynamic(toOccupy, index);
		}
	}
	m_canBeStockPiled.resize(m_id.size());
	for(auto iter = data["canBeStockPiled"].begin(); iter != data["canBeStockPiled"].end(); ++iter)
	{
		ItemIndex index = ItemIndex::create(std::stoi(iter.key()));
		auto& canBeStockPiled = m_canBeStockPiled[index] = std::make_unique<ItemCanBeStockPiled>();
		canBeStockPiled->load(iter.value(), m_area);
	}
	m_constructedShape.resize(m_id.size());
	for(auto iter = data["constructedShape"].begin(); iter != data["constructedShape"].end(); ++iter)
	{
		ItemIndex index = ItemIndex::create(std::stoi(iter.key()));
		m_constructedShape[index] = std::make_unique<ConstructedShape>(iter.value());
	}
}
void Items::loadCargoAndCraftJobs(const Json& data)
{
	m_craftJobForWorkPiece.resize(m_id.size());
	for(auto iter = data["craftJobForWorkPiece"].begin(); iter != data["craftJobForWorkPiece"].end(); ++iter)
	{
		ItemIndex index = ItemIndex::create(std::stoi(iter.key()));
		m_craftJobForWorkPiece[index] = m_area.m_simulation.getDeserializationMemo().m_craftJobs.at(iter.value().get<uintptr_t>());
	}
	m_hasCargo.resize(m_id.size());
	for(auto iter = data["hasCargo"].begin(); iter != data["hasCargo"].end(); ++ iter)
	{
		ItemIndex index = ItemIndex::create(std::stoi(iter.key()));
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(iter.value());
	}
}
SmallSet<ItemIndex> Items::getAll() const
{
	// TODO: Replace with std::iota?
	SmallSet<ItemIndex> output;
	output.reserve(m_shape.size());
	for(auto i = ItemIndex::create(0); i < size(); ++i)
		output.insert(i);
	return output;
}
void to_json(Json& data, std::unique_ptr<ItemHasCargo> hasCargo) { data = hasCargo->toJson(); }
void to_json(Json& data, std::unique_ptr<ItemCanBeStockPiled> canBeStockPiled) { data = canBeStockPiled == nullptr ? Json{false} : canBeStockPiled->toJson(); }
Json Items::toJson() const
{
	Json data = Portables<Items, ItemIndex, ItemReferenceIndex, false>::toJson();
	data["id"] = m_id;
	data["name"] = m_name;
	data["location"] = m_location;
	data["itemType"] = m_itemType;
	data["materialType"] = m_solid;
	data["quality"] = m_quality;
	data["quantity"] = m_quantity;
	data["percentWear"] = m_percentWear;
	data["installed"] = m_installed;
	data["onSurface"] = m_onSurface;
	data["canBeStockPiled"] = Json::object();
	int i = 0;
	for(const auto& canBeStockPiled : m_canBeStockPiled)
	{
		if(canBeStockPiled != nullptr)
			data["canBeStockPiled"][std::to_string(i)] = canBeStockPiled->toJson();
		++i;
	}
	data["pilot"] = m_pilot;
	data["constructedShape"] = Json::object();
	i = 0;
	for(const auto& constructedShape : m_constructedShape)
	{
		if(constructedShape != nullptr)
			data["constructedShape"][std::to_string(i)] = constructedShape->toJson();
		++i;
	}
	data["hasCargo"] = Json::object();
	i = 0;
	for(const std::unique_ptr<ItemHasCargo>& cargo : m_hasCargo)
	{
		if(cargo != nullptr)
			data["hasCargo"][std::to_string(i)] = cargo->toJson();
		++i;
	}
	data["craftJobForWorkPiece"] = Json::object();
	i = 0;
	for(const CraftJob* job : m_craftJobForWorkPiece)
	{
		if(job != nullptr)
			data["craftJobForWorkPiece"][std::to_string(i)] = reinterpret_cast<uintptr_t>(job);
	}
	return data;
}
// HasCargo.
ItemHasCargo::ItemHasCargo(ItemTypeId itemType) : m_maxVolume(ItemType::getInternalVolume(itemType)) { }
ItemHasCargo::ItemHasCargo(const Json& data)
{
	data["maxVolume"].get_to(m_maxVolume);
	data["volume"].get_to(m_volume);
	data["mass"].get_to(m_mass);
	if(data.contains("actors"))
		data["actors"].get_to(m_actors);
	if(data.contains("items"))
		data["items"].get_to(m_items);
	if(data.contains("fluidType"))
		data["fluidType"].get_to(m_fluidType);
	if(data.contains("fluidVolume"))
		data["fluidVolume"].get_to(m_fluidVolume);
}
Json ItemHasCargo::toJson() const
{
	Json output = {{"maxVolume", m_maxVolume}, {"volume", m_volume}, {"mass", m_mass}};
	if(!m_actors.empty())
		output["actors"] = m_actors;
	if(!m_items.empty())
		output["items"] = m_items;
	if(!m_fluidType.empty())
		output["fluidType"] = m_fluidType;
	if(!m_fluidVolume.empty())
		output["fluidVolume"] = m_fluidVolume;
	return output;
}
void ItemHasCargo::addActor(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	assert(m_volume + actors.getVolume(actor) <= m_maxVolume);
	assert(!containsActor(actor));
	assert(m_fluidVolume == 0 && m_fluidType.empty());
	m_actors.insert(actor);
	m_volume += actors.getVolume(actor);
	m_mass += actors.getMass(actor);
}
void ItemHasCargo::addItem(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	//TODO: This method does not call hasShape.exit(), which is not consistant with the behaviour of CanPickup::pickup.
	assert(m_volume + items.getVolume(item) <= m_maxVolume);
	assert(!containsItem(item));
	assert(m_fluidVolume == 0 && m_fluidType.empty());
	m_items.insert(item);
	m_volume += items.getVolume(item);
	m_mass += items.getMass(item);
}
void ItemHasCargo::addFluid(FluidTypeId fluidType, CollisionVolume volume)
{
	assert(m_fluidVolume + volume <= m_maxVolume.toCollisionVolume());
	if(m_fluidType.empty())
	{
		m_fluidType = fluidType;
		m_fluidVolume = volume;
	}
	else
	{
		assert(m_fluidType == fluidType);
		m_fluidVolume += volume;
	}
	m_mass += volume.toVolume() * FluidType::getDensity(fluidType);
}
ItemIndex ItemHasCargo::addItemGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity)
{
	assert(ItemType::getGeneric(itemType));
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
		{
			// Add to existing stack.
			items.addQuantity(item, quantity);
			m_mass += ItemType::getFullDisplacement(itemType) * MaterialType::getDensity(materialType) * quantity;
			m_volume += ItemType::getFullDisplacement(itemType) * quantity;
			return item;
		}
	// Create new stack.
	ItemIndex newItem = items.create({
		.itemType=itemType,
		.materialType=materialType,
		.quantity=quantity,
	});
	addItem(area, newItem);
	return newItem;
}
void ItemHasCargo::removeFluidVolume([[maybe_unused]] FluidTypeId fluidType, CollisionVolume volume)
{
	assert(m_fluidType == fluidType);
	assert(m_fluidVolume >= volume);
	m_fluidVolume -= volume;
	if(m_fluidVolume == 0)
		m_fluidType.clear();
}
void ItemHasCargo::removeActor(Area& area, ActorIndex actor)
{
	assert(containsActor(actor));
	Actors& actors = area.getActors();
	m_volume -= actors.getVolume(actor);
	m_mass -= actors.getMass(actor);
	m_actors.erase(actor);
}
void ItemHasCargo::removeItem(Area& area, ItemIndex item)
{
	assert(containsItem(item));
	Items& items = area.getItems();
	m_volume -= items.getVolume(item);
	m_mass -= items.getMass(item);
	m_items.erase(item);
}
void ItemHasCargo::removeItemGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity)
{
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
		{
			if(items.getQuantity(item) == quantity)
				// TODO: should be reusing an iterator here.
				m_items.erase(item);
			else
			{
				assert(quantity < items.getQuantity(item));
				items.removeQuantity(item, quantity);
			}
			m_volume -= items.getVolume(item);
			m_mass -= items.getMass(item);
			return;
		}
	std::unreachable();
}
ItemIndex ItemHasCargo::unloadGenericTo(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity, Point3D location)
{
	removeItemGeneric(area, itemType, materialType, quantity);
	return area.getSpace().item_addGeneric(location, itemType, materialType, quantity);
}
void ItemHasCargo::updateCarrierIndexForAllCargo(Area& area, ItemIndex newIndex)
{
	Items& items = area.getItems();
	for(ItemIndex item : m_items)
		items.updateCarrierIndex(item, newIndex);
	Actors& actors = area.getActors();
	for(ActorIndex actor : m_actors)
		actors.updateCarrierIndex(actor, newIndex);
}
void ItemHasCargo::moveContentsFromTo(Area& fromArea, Area& toArea, ItemIndex carrier)
{
	Actors& toActors = toArea.getActors();
	Actors& fromActors = fromArea.getActors();
	for(ActorIndex& actor : m_actors)
	{
		ActorIndex newIndex = fromActors.moveTo(toActors, actor);
		actor = newIndex;
		toActors.setCarrier(newIndex, ActorOrItemIndex::create(carrier));
	}
	Items& toItems = toArea.getItems();
	Items& fromItems = fromArea.getItems();
	for(ItemIndex& item : m_items)
	{
		ItemIndex newIndex = fromItems.moveTo(toItems, item);
		item = newIndex;
		toItems.setCarrier(newIndex, ActorOrItemIndex::create(carrier));
	}
}
bool ItemHasCargo::canAddActor(Area& area, ActorIndex actor) const { return m_volume + area.getActors().getVolume(actor) <= m_maxVolume; }
bool ItemHasCargo::canAddItem(Area& area, ItemIndex item) const { return m_volume + area.getItems().getVolume(item) <= m_maxVolume; }
bool ItemHasCargo::canAddFluid(FluidTypeId fluidType) const { return m_fluidType.empty() || m_fluidType == fluidType; }
bool ItemHasCargo::containsGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity) const
{
	assert(ItemType::getGeneric(itemType));
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
			return items.getQuantity(item) >= quantity;
	return false;
}
