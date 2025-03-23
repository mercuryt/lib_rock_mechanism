#include "items.h"
#include "../itemType.h"
#include "../area/area.h"
#include "../simulation/simulation.h"
#include "../simulation/hasItems.h"
#include "../util.h"
#include "../actors/actors.h"
#include "../index.h"
#include "../moveType.h"
#include "../types.h"
#include "../portables.hpp"
#include <memory>
#include <ranges>
// RemarkItemForStockPilingEvent
ReMarkItemForStockPilingEvent::ReMarkItemForStockPilingEvent(Area& area, ItemCanBeStockPiled& canBeStockPiled, const FactionId& faction, const Step& duration, const Step start) :
	ScheduledEvent(area.m_simulation, duration, start),
	m_faction(faction),
	m_canBeStockPiled(canBeStockPiled) { }
void ReMarkItemForStockPilingEvent::execute(Simulation&, Area*)
{
	m_canBeStockPiled.maybeSet(m_faction);
	m_canBeStockPiled.m_scheduledEvents.erase(m_faction);
}
void ReMarkItemForStockPilingEvent::clearReferences(Simulation&, Area*) { }
// ItemCanBeStockPiled
void ItemCanBeStockPiled::load(const Json& data, Area& area)
{
	if(data.contains("data"))
		for(const Json& faction : data["data"])
			m_data.add(faction.get<FactionId>());
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
void ItemCanBeStockPiled::scheduleReset(Area& area, const FactionId& faction, const Step& duration, const Step start)
{
	assert(!m_scheduledEvents.contains(faction));
	auto pair = m_scheduledEvents.emplace(faction, area.m_eventSchedule);
	HasScheduledEvent<ReMarkItemForStockPilingEvent>& eventHandle = pair;
	eventHandle.schedule(area, *this, faction, duration, start);
}
void ItemCanBeStockPiled::unsetAndScheduleReset(Area& area, const FactionId& faction, const Step& duration)
{
	unset(faction);
	scheduleReset(area, faction, duration);
}
void ItemCanBeStockPiled::maybeUnsetAndScheduleReset(Area& area, const FactionId& faction, const Step& duration)
{
	maybeUnset(faction);
	scheduleReset(area, faction, duration);
}
// Item
void Items::onChangeAmbiantSurfaceTemperature()
{
	Blocks& blocks = m_area.getBlocks();
	m_onSurface.forEach([&](const ItemIndex& index){
		assert(m_location[index].exists());
		Temperature temperature = blocks.temperature_get(m_location[index]);
		setTemperature(index, temperature);
	});
}
Items::Items(Area& area) :
	Portables(area, false)
{ }
template<typename Action>
void Items::forEachData(Action&& action)
{
	forEachDataPortables(action);
	action(m_canBeStockPiled);
	action(m_craftJobForWorkPiece);
	action(m_hasCargo);
	action(m_id);
	action(m_installed);
	action(m_itemType);
	action(m_materialType);
	action(m_name);
	action(m_percentWear);
	action(m_quality);
	action(m_quantity);
	action(m_onSurface);
}
ItemIndex Items::create(ItemParamaters itemParamaters)
{
	ItemIndex index = ItemIndex::create(size());
	// TODO: This 'toItem' call should not be neccessary. Why does ItemIndex + int = HasShapeIndex?
	resize(index + 1);
	const MoveTypeId& moveType = ItemType::getMoveType(itemParamaters.itemType);
	Portables<Items, ItemIndex, ItemReferenceIndex>::create(index, moveType, ItemType::getShape(itemParamaters.itemType), itemParamaters.faction, itemParamaters.isStatic, itemParamaters.quantity);
	assert(m_canBeStockPiled[index] == nullptr);
	m_craftJobForWorkPiece[index] = itemParamaters.craftJob;
	assert(m_hasCargo[index] == nullptr);
	m_id[index] = itemParamaters.id.exists() ? itemParamaters.id : m_area.m_simulation.m_items.getNextId();
	m_installed.set(index, itemParamaters.installed);
	ItemTypeId itemType = m_itemType[index] = itemParamaters.itemType;
	m_materialType[index] = itemParamaters.materialType;
	m_name[index] = itemParamaters.name;
	m_percentWear[index] = itemParamaters.percentWear;
	m_quality[index] = itemParamaters.quality;
	m_quantity[index] = itemParamaters.quantity;
	if(ItemType::getGeneric(itemType))
	{
		assert(m_quality[index].empty());
		assert(m_percentWear[index].empty());
	}
	else
		assert(m_quantity[index] == 1);
	if(itemParamaters.location.exists())
		// A generic item type might merge with an existing stack on creation, in that case return the index of the existing stack.
		index = setLocationAndFacing(index, itemParamaters.location, itemParamaters.facing);
	static const MoveTypeId rolling = MoveType::byName("roll");
	static const ItemTypeId panniers = ItemType::byName("panniers");
	if(itemType == panniers  || (moveType == rolling && ItemType::getInternalVolume(itemType) != 0))
		m_area.m_hasHaulTools.registerHaulTool(m_area, index);
	return index;
}
void Items::moveIndex(const ItemIndex& oldIndex, const ItemIndex& newIndex)
{
	forEachData([&](auto& data){ data.moveIndex(oldIndex, newIndex); });
	updateStoredIndicesPortables(oldIndex, newIndex);
	if(m_hasCargo[newIndex] != nullptr)
		m_hasCargo[newIndex]->updateCarrierIndexForAllCargo(m_area, newIndex);
	if(m_onSurface[oldIndex])
	{
		m_onSurface.unset(oldIndex);
		m_onSurface.set(newIndex);
	}
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : m_blocks[newIndex])
		blocks.item_updateIndex(block, oldIndex, newIndex);
}
ItemIndex Items::setLocation(const ItemIndex& index, const BlockIndex& block)
{
	assert(index.exists());
	assert(m_location[index].exists());
	assert(block.exists());
	Facing4 facing = m_area.getBlocks().facingToSetWhenEnteringFrom(block, m_location[index]);
	return setLocationAndFacing(index, block, facing);
}
ItemIndex Items::setLocationAndFacing(const ItemIndex& index, const BlockIndex& block, Facing4 facing)
{
	assert(index.exists());
	assert(block.exists());
	assert(m_location[index] != block);
	Blocks& blocks = m_area.getBlocks();
	if(isGeneric(index) && m_static[index])
	{
		// Check for existing generic item to combine with.
		ItemIndex found = blocks.item_getGeneric(block, getItemType(index), getMaterialType(index));
		if(found.exists())
			// Return the index of the found item, which may be different then it was before 'index' was destroyed by merge.
			return merge(found, index);
	}
	BlockIndex previousLocation = m_location[index];
	Facing4 previousFacing = m_facing[index];
	SmallMap<ActorOrItemIndex, Offset3D> onDeckWithOffsets;
	if(previousLocation.exists())
	{
		for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		{
			onDeckWithOffsets.insert(onDeck, blocks.relativeOffsetTo(previousLocation, onDeck.getLocation(m_area)));
			onDeck.exit(m_area);
		}
		exit(index);
	}
	m_location[index] = block;
	m_facing[index] = facing;
	const Quantity& quantity = m_quantity[index];
	auto& occupiedBlocks = m_blocks[index];
	for(const auto& pair : Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing))
	{
		BlockIndex occupied = blocks.offset(block, pair.offset);
		blocks.item_record(occupied, index, CollisionVolume::create((quantity * pair.volume.get()).get()));
		occupiedBlocks.add(occupied);
	}
	if(blocks.isExposedToSky(block))
		m_onSurface.set(index);
	else
		m_onSurface.unset(index);
	onSetLocation(index, previousFacing, onDeckWithOffsets);
	return index;
}
void Items::exit(const ItemIndex& index)
{
	assert(m_location[index].exists());
	BlockIndex location = m_location[index];
	auto& blocks = m_area.getBlocks();
	for(BlockIndex occupied : m_blocks[index])
		blocks.item_erase(occupied, index);
	m_location[index].clear();
	m_blocks[index].clear();
	if(blocks.isExposedToSky(location))
		m_onSurface.unset(index);
}
void Items::setTemperature(const ItemIndex& index, const Temperature& temperature)
{
	const MaterialTypeId& materialType = m_materialType[index];
	if(MaterialType::canBurn(materialType) && MaterialType::getIgnitionTemperature(materialType) <= temperature)
	{
		// TODO: item is on fire.
	}
	else if(MaterialType::canMelt(materialType) && MaterialType::getMeltingPoint(materialType) <= temperature)
	{
		const CollisionVolume volume = getTotalCollisionVolume(index);
		const BlockIndex location = m_location[index];
		destroy(index);
		m_area.getBlocks().fluid_add(location, volume, MaterialType::getMeltsInto(materialType));
	}
}
void Items::addQuantity(const ItemIndex& index, const Quantity& delta)
{
	assert(delta != 0);
	BlockIndex location = m_location[index];
	Facing4 facing = m_facing[index];
	// TODO: Update in place rather then exit, update, enter.
	exit(index);
	m_quantity[index] += delta;
	setLocationAndFacing(index, location, facing);
	if(m_reservables[index] != nullptr)
		m_reservables[index]->setMaxReservations(m_quantity[index]);
}
// May destroy.
void Items::removeQuantity(const ItemIndex& index, const Quantity& delta, CanReserve* canReserve)
{
	if(canReserve != nullptr)
		reservable_maybeUnreserve(index, *canReserve, delta);
	if(m_quantity[index] == delta)
		destroy(index);
	else
	{
		assert(delta < m_quantity[index]);
		// TODO: Update in place rather then exit, update, enter.
		BlockIndex location = m_location[index];
		Facing4 facing = m_facing[index];
		exit(index);
		m_quantity[index] -= delta;
		setLocationAndFacing(index, location, facing);
		if(m_reservables[index] != nullptr)
			m_reservables[index]->setMaxReservations(m_quantity[index]);
	}
}
void Items::install(const ItemIndex& index, const BlockIndex& block, const Facing4& facing, const FactionId& faction)
{
	setLocationAndFacing(index, block, facing);
	m_installed.set(index);
	if(ItemType::getCraftLocationStepTypeCategory(m_itemType[index]).exists())
	{
		ItemTypeId item = m_itemType[index];
		BlockIndex craftLocation = ItemType::getCraftLocation(item, m_area.getBlocks(), block, facing);
		if(craftLocation.exists())
			m_area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addLocation(ItemType::getCraftLocationStepTypeCategory(item), craftLocation);
	}
}
ItemIndex Items::merge(const ItemIndex& index, const ItemIndex& other)
{
	assert(index != other);
	assert(m_itemType[index] == m_itemType[other]);
	assert(m_materialType[index] == m_materialType[other]);
	if(m_reservables[other] != nullptr)
		reservable_merge(index, *m_reservables[other]);
	BlockIndex location = m_location[index];
	Facing4 facing = m_facing[index];
	// Exit, add quantity, and then set location to update CollisionVolume with new quantity.
	exit(index);
	m_quantity[index] += m_quantity[other];
	setLocationAndFacing(index, location, facing);
	assert(m_quantity[index] == m_reservables[index]->getMaxReservations());
	if(m_destroy[other] != nullptr)
		onDestroy_merge(index, *m_destroy[other]);
	// Store a reference to index because it's ItemIndex may change when other is destroyed.
	ItemReference ref = m_area.getItems().m_referenceData.getReference(index);
	destroy(other);
	return ref.getIndex(m_referenceData);
}
void Items::setQuality(const ItemIndex& index, const Quality& quality)
{
	m_quality[index] = quality;
}
void Items::setWear(const ItemIndex& index, const Percent& wear)
{
	m_percentWear[index] = wear;
}
void Items::setQuantity(const ItemIndex& index, const Quantity& quantity)
{
	m_quantity[index] = quantity;
}
void Items::unsetCraftJobForWorkPiece(const ItemIndex& index)
{
	m_craftJobForWorkPiece[index] = nullptr;
}
void Items::destroy(const ItemIndex& index)
{
	// No need to explicitly unschedule events here, destorying the event holder will do it.
	if(hasLocation(index))
		exit(index);
	static const MoveTypeId rolling = MoveType::byName("roll");
	if(m_moveType[index] == rolling && ItemType::getInternalVolume(m_itemType[index]) != 0)
		m_area.m_hasHaulTools.unregisterHaulTool(m_area, index);
	const auto& s = ItemIndex::create(size() - 1);
	if(index != s)
		moveIndex(s, index);
	m_area.m_hasStockPiles.removeItemFromAllFactions(index);
	resize(s);
	// Will do the same move / resize logic internally, so stays in sync with moves from the DataVectors.
	m_referenceData.remove(index);
}
bool Items::isGeneric(const ItemIndex& index) const { return ItemType::getGeneric(m_itemType[index]); }
bool Items::isPreparedMeal(const ItemIndex& index) const
{
	static ItemTypeId preparedMealType = ItemType::byName("prepared meal");
	return m_itemType[index] == preparedMealType;
}
CraftJob& Items::getCraftJobForWorkPiece(const ItemIndex& index) const
{
	assert(isWorkPiece(index));
	return *m_craftJobForWorkPiece[index];
}
Mass Items::getSingleUnitMass(const ItemIndex& index) const
{
	return Mass::create(std::max(1u, (ItemType::getVolume(m_itemType[index]) * MaterialType::getDensity(m_materialType[index])).get()));
}
Mass Items::getMass(const ItemIndex& index) const
{
	Mass output = ItemType::getVolume(m_itemType[index]) * MaterialType::getDensity(m_materialType[index]) * m_quantity[index];
	if(m_hasCargo[index] != nullptr)
		output += m_hasCargo[index]->getMass();
	return output;
}
Volume Items::getVolume(const ItemIndex& index) const
{
	return ItemType::getVolume(m_itemType[index]) * m_quantity[index];
}
CollisionVolume Items::getTotalCollisionVolume(const ItemIndex& index) const
{
	return Shape::getTotalCollisionVolume(m_shape[index]) * m_quantity[index];
}
MoveTypeId Items::getMoveType(const ItemIndex& index) const
{
	return ItemType::getMoveType(m_itemType[index]);
}
void Items::log(const ItemIndex& index) const
{
	std::cout << ItemType::getName(m_itemType[index]) << "[" << MaterialType::getName(m_materialType[index]) << "]";
	if(m_quantity[index] != 1)
		std::cout << "(" << m_quantity[index].get() << ")";
	if(m_craftJobForWorkPiece[index] != nullptr)
		std::cout << "{" << m_craftJobForWorkPiece[index]->getStep().get() << "}";
	Portables<Items, ItemIndex, ItemReferenceIndex>::log(index);
	std::cout << std::endl;
}
// Wrapper methods.
void Items::stockpile_maybeUnsetAndScheduleReset(const ItemIndex& index, const FactionId& faction, const Step& duration)
{
	if(m_canBeStockPiled[index] != nullptr)
		m_canBeStockPiled[index]->maybeUnsetAndScheduleReset(m_area, faction, duration);
}
void Items::stockpile_set(const ItemIndex& index, const FactionId& faction)
{
	if(m_canBeStockPiled[index] == nullptr)
		m_canBeStockPiled[index] = std::make_unique<ItemCanBeStockPiled>();
	m_canBeStockPiled[index]->set(faction);
}
void Items::stockpile_maybeUnset(const ItemIndex& index, const FactionId& faction)
{
	if(m_canBeStockPiled[index] != nullptr)
	{
		m_canBeStockPiled[index]->maybeUnset(faction);
		if(m_canBeStockPiled[index]->empty())
			m_canBeStockPiled[index] = nullptr;
	}
}
bool Items::stockpile_canBeStockPiled(const ItemIndex& index, const FactionId& faction) const
{
	if(m_canBeStockPiled[index] == nullptr)
		return false;
	return m_canBeStockPiled[index]->contains(faction);
}
void Items::load(const Json& data)
{
	static_cast<Portables<Items, ItemIndex, ItemReferenceIndex>*>(this)->load(data);
	data["onSurface"].get_to(m_onSurface);
	data["name"].get_to(m_name);
	data["itemType"].get_to(m_itemType);
	data["materialType"].get_to(m_materialType);
	data["id"].get_to(m_id);
	data["quality"].get_to(m_quality);
	data["quantity"].get_to(m_quantity);
	data["percentWear"].get_to(m_percentWear);
	m_hasCargo.resize(m_id.size());
	Blocks& blocks = m_area.getBlocks();
	for(ItemIndex index : getAll())
	{
		m_area.m_simulation.m_items.registerItem(m_id[index], m_area.getItems(), index);
		if(m_location[index].exists())
			for (const auto& pair : Shape::makeOccupiedPositionsWithFacing(m_shape[index], m_facing[index]))
			{
				BlockIndex occupied = blocks.offset(m_location[index], pair.offset);
				blocks.item_record(occupied, index, CollisionVolume::create((m_quantity[index] * pair.volume.get()).get()));
			}
	}
	m_canBeStockPiled.resize(m_id.size());
	for(auto iter = data["canBeStockPiled"].begin(); iter != data["canBeStockPiled"].end(); ++iter)
	{
		const ItemIndex index = ItemIndex::create(std::stoi(iter.key()));
		auto& canBeStockPiled = m_canBeStockPiled[index] = std::make_unique<ItemCanBeStockPiled>();
		canBeStockPiled->load(iter.value(), m_area);
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
ItemIndices Items::getAll() const
{
	// TODO: Replace with std::iota?
	ItemIndices output;
	output.reserve(m_shape.size());
	for(auto i = ItemIndex::create(0); i < size(); ++i)
		output.add(i);
	return output;
}
void to_json(Json& data, std::unique_ptr<ItemHasCargo> hasCargo) { data = hasCargo->toJson(); }
void to_json(Json& data, std::unique_ptr<ItemCanBeStockPiled> canBeStockPiled) { data = canBeStockPiled == nullptr ? Json{false} : canBeStockPiled->toJson(); }
Json Items::toJson() const
{
	Json data = Portables<Items, ItemIndex, ItemReferenceIndex>::toJson();
	data["id"] = m_id;
	data["name"] = m_name;
	data["location"] = m_location;
	data["itemType"] = m_itemType;
	data["materialType"] = m_materialType;
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
void ItemHasCargo::addActor(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	assert(m_volume + actors.getVolume(actor) <= m_maxVolume);
	assert(!containsActor(actor));
	assert(m_fluidVolume == 0 && m_fluidType.empty());
	m_actors.add(actor);
	m_volume += actors.getVolume(actor);
	m_mass += actors.getMass(actor);
}
void ItemHasCargo::addItem(Area& area, const ItemIndex& item)
{
	Items& items = area.getItems();
	//TODO: This method does not call hasShape.exit(), which is not consistant with the behaviour of CanPickup::pickup.
	assert(m_volume + items.getVolume(item) <= m_maxVolume);
	assert(!containsItem(item));
	assert(m_fluidVolume == 0 && m_fluidType.empty());
	m_items.add(item);
	m_volume += items.getVolume(item);
	m_mass += items.getMass(item);
}
void ItemHasCargo::addFluid(const FluidTypeId& fluidType, const CollisionVolume& volume)
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
ItemIndex ItemHasCargo::addItemGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	assert(ItemType::getGeneric(itemType));
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
		if(items.getItemType(item) == itemType && items.getMaterialType(item)  == materialType)
		{
			// Add to existing stack.
			items.addQuantity(item, quantity);
			m_mass += ItemType::getVolume(itemType) * MaterialType::getDensity(materialType)  * quantity;
			m_volume += ItemType::getVolume(itemType) * quantity;
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
void ItemHasCargo::removeFluidVolume([[maybe_unused]] const FluidTypeId& fluidType, const CollisionVolume& volume)
{
	assert(m_fluidType == fluidType);
	assert(m_fluidVolume >= volume);
	m_fluidVolume -= volume;
	if(m_fluidVolume == 0)
		m_fluidType.clear();
}
void ItemHasCargo::removeActor(Area& area, const ActorIndex& actor)
{
	assert(containsActor(actor));
	Actors& actors = area.getActors();
	m_volume -= actors.getVolume(actor);
	m_mass -= actors.getMass(actor);
	m_actors.remove(actor);
}
void ItemHasCargo::removeItem(Area& area, const ItemIndex& item)
{
	assert(containsItem(item));
	Items& items = area.getItems();
	m_volume -= items.getVolume(item);
	m_mass -= items.getMass(item);
	m_items.remove(item);
}
void ItemHasCargo::removeItemGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
		{
			if(items.getQuantity(item) == quantity)
				// TODO: should be reusing an iterator here.
				m_items.remove(item);
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
ItemIndex ItemHasCargo::unloadGenericTo(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity, const BlockIndex& location)
{
	removeItemGeneric(area, itemType, materialType, quantity);
	return area.getBlocks().item_addGeneric(location, itemType, materialType, quantity);
}
void ItemHasCargo::updateCarrierIndexForAllCargo(Area& area, const ItemIndex& newIndex)
{
	Items& items = area.getItems();
	for(ItemIndex item : m_items)
		items.updateCarrierIndex(item, newIndex);
	Actors& actors = area.getActors();
	for(ActorIndex actor : m_actors)
		actors.updateCarrierIndex(actor, newIndex);
}
bool ItemHasCargo::canAddActor(Area& area, const ActorIndex& actor) const { return m_volume + area.getActors().getVolume(actor) <= m_maxVolume; }
bool ItemHasCargo::canAddItem(Area& area, const ItemIndex& item) const { return m_volume + area.getItems().getVolume(item) <= m_maxVolume; }
bool ItemHasCargo::canAddFluid(const FluidTypeId& fluidType) const { return m_fluidType.empty() || m_fluidType == fluidType; }
bool ItemHasCargo::containsGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity) const
{
	assert(ItemType::getGeneric(itemType));
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
			return items.getQuantity(item) >= quantity;
	return false;
}
