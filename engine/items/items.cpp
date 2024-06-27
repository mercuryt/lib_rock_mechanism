#include "items.h"
#include "../itemType.h"
#include "../area.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "../util.h"
#include "portables.h"
#include "types.h"
#include <ranges>
// RemarkItemForStockPilingEvent
ReMarkItemForStockPilingEvent::ReMarkItemForStockPilingEvent(ItemCanBeStockPiled& canBeStockPiled, Faction& faction, Step duration, const Step start) : 
	ScheduledEvent(canBeStockPiled.m_area.m_simulation, duration, start),
	m_faction(faction),
	m_canBeStockPiled(canBeStockPiled) { }
void ReMarkItemForStockPilingEvent::execute(Simulation&, Area*) 
{ 
	m_canBeStockPiled.maybeSet(m_faction); 
	m_canBeStockPiled.m_scheduledEvents.erase(&m_faction); 
}
void ReMarkItemForStockPilingEvent::clearReferences(Simulation&, Area*) { }
// ItemCanBeStockPiled
ItemCanBeStockPiled::ItemCanBeStockPiled(const Json& data, DeserializationMemo& deserializationMemo) : m_area(deserializationMemo.area(data["area"]))
{
	if(data.contains("data"))
		for(const Json& factionData : data["data"])
			m_data.insert(&deserializationMemo.faction(factionData));
	if(data.contains("scheduledEvents"))
		for(const Json& eventData : data["scheduledEvents"])
		{
			Faction& faction = deserializationMemo.faction(eventData[0]);
			Step start = eventData[1]["start"].get<Step>();
			Step duration = eventData[1]["duration"].get<Step>();
			scheduleReset(faction, duration, start);
		}
}
Json ItemCanBeStockPiled::toJson() const
{
	Json data;
	if(!m_data.empty())
	{
		data["data"] = Json::array();
		for(Faction* faction : m_data)
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
void ItemCanBeStockPiled::scheduleReset(Faction& faction, Step duration, Step start)
{
	assert(!m_scheduledEvents.contains(&faction));
	auto [iter, created] = m_scheduledEvents.emplace(&faction, m_area.m_eventSchedule);
	assert(created);
	HasScheduledEvent<ReMarkItemForStockPilingEvent>& eventHandle = iter->second;
	eventHandle.schedule(*this, faction, duration, start);
}
// Item
void Items::onChangeAmbiantSurfaceTemperature()
{
	Blocks& blocks = m_area.getBlocks();
	for(ItemIndex index : m_onSurface)
	{
		Temperature temperature = blocks.temperature_get(m_location.at(index));
		setTemperature(index, temperature);
	}
}
ItemIndex Items::create(ItemParamaters itemParamaters)
{
	ItemIndex index = HasShapes::getNextIndex();
	Portables::create(index, itemParamaters.itemType.moveType, itemParamaters.itemType.shape, itemParamaters.location, itemParamaters.facing, itemParamaters.isStatic);
	const ItemType& itemType = *(m_itemType.at(index) = &itemParamaters.itemType);
	m_materialType.at(index) = &itemParamaters.materialType;
	m_id.at(index) = itemParamaters.id ? itemParamaters.id : m_area.m_simulation.nextItemId();
	m_quality.at(index) = itemParamaters.quality;
	m_percentWear.at(index) = itemParamaters.percentWear;
	m_quantity.at(index) = itemParamaters.quantity;
	m_installed[index] = itemParamaters.installed;
	if(itemType.generic)
	{
		assert(!m_quality.at(index));
		assert(!m_percentWear.at(index));
	}
	else
		assert(m_quantity.at(index) == 1);
	if(itemParamaters.location != BLOCK_INDEX_MAX)
		setLocationAndFacing(index, itemParamaters.location, itemParamaters.facing);
	m_canBeStockPiled.insert(index, {m_area});
	return index;
}
void Items::resize(HasShapeIndex newSize)
{
	m_materialType.resize(newSize);
	m_id.resize(newSize);
	m_quality.resize(newSize);
	m_percentWear.resize(newSize);
	m_quantity.resize(newSize);
	m_installed.resize(newSize);
	m_canBeStockPiled.resize(newSize);
}
void Items::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	m_materialType[newIndex] = m_materialType[oldIndex];
	m_id[newIndex] = m_id[oldIndex];
	m_quality[newIndex] = m_quality[oldIndex];
	m_percentWear[newIndex] = m_percentWear[oldIndex];
	m_quantity[newIndex] = m_quantity[oldIndex];
	m_installed[newIndex] = m_installed[oldIndex];
	m_canBeStockPiled.moveKey(oldIndex, newIndex);
	m_hasCargo.moveKey(oldIndex, newIndex);
}
bool Items::indexCanBeMoved(HasShapeIndex index) const
{
	if(!Portables::indexCanBeMoved(index))
		return false;
	// If there is no location set index is cargo / in transit / part of some actor's equipmentSet.
	// These references prevent it from being moved.
	if(m_location.at(index) == BLOCK_INDEX_MAX)
		return false;
	return true;
}
void Items::setLocation(ItemIndex index, BlockIndex block)
{
	assert(m_location.at(index) != BLOCK_INDEX_MAX);
	Facing facing = m_area.getBlocks().facingToSetWhenEnteringFrom(block, m_location.at(index));
	setLocationAndFacing(index, block, facing);
}
void Items::setLocationAndFacing(ItemIndex index, BlockIndex block, Facing facing)
{
	if(m_location.at(index) != BLOCK_INDEX_MAX)
		exit(index);
	Blocks& blocks = m_area.getBlocks();
	if(isGeneric(index) && m_static[index])
	{
		ItemIndex found = blocks.item_getGeneric(index, getItemType(index), getMaterialType(index));
		if(found != ITEM_INDEX_MAX)
		{
			merge(found, index);
			return;
		}
	}
	for(auto [x, y, z, v] : m_shape.at(index)->makeOccupiedPositionsWithFacing(facing))
	{
		BlockIndex occupied = blocks.offset(block, x, y, z);
		blocks.item_record(occupied, index, v);
	}
	updateIsOnSurface(index, block);
}
void Items::exit(ItemIndex index)
{
	assert(m_location.at(index) != BLOCK_INDEX_MAX);
	BlockIndex location = m_location.at(index);
	auto& blocks = m_area.getBlocks();
	for(auto [x, y, z, v] : m_shape.at(index)->makeOccupiedPositionsWithFacing(m_facing.at(index)))
	{
		BlockIndex occupied = blocks.offset(location, x, y, z);
		blocks.item_erase(occupied, index);
	}
	m_location.at(index) = BLOCK_INDEX_MAX;
}
void Items::setTemperature(ItemIndex, Temperature)
{
	//TODO
}
void Items::addQuantity(ItemIndex index, Quantity delta)
{
	Quantity newQuantity = (m_quantity.at(index) += delta);
	if(m_reservables.contains(index))
		m_reservables.at(index).setMaxReservations(newQuantity);
}
void Items::removeQuantity(ItemIndex index, Quantity delta)
{
	if(m_quantity.at(index) == delta)
		destroy(index);
	else
	{
		assert(delta < m_quantity.at(index));
		Quantity newQuantity = (m_quantity.at(index) -= delta);
		if(m_reservables.contains(index))
			m_reservables.at(index).setMaxReservations(newQuantity);
	}
}
void Items::install(ItemIndex index, BlockIndex block, Facing facing, Faction& faction)
{
	setLocationAndFacing(index, block, facing);
	m_installed[index] = true;
	if(m_itemType.at(index)->craftLocationStepTypeCategory)
	{
		BlockIndex craftLocation = m_itemType.at(index)->getCraftLocation(m_area.getBlocks(), block, facing);
		if(craftLocation != BLOCK_INDEX_MAX)
			m_area.m_hasCraftingLocationsAndJobs.at(faction).addLocation(*m_itemType.at(index)->craftLocationStepTypeCategory, craftLocation);
	}
}
void Items::merge(ItemIndex index, ItemIndex other)
{
	assert(m_itemType.at(index) == m_itemType.at(other));
	assert(m_materialType.at(index) == m_materialType.at(other));
	if(m_reservables.contains(other))
		reservable_merge(index, m_reservables.at(other));
	m_quantity.at(index) += m_quantity.at(other);
	if(m_destroy.contains(other))
		onDestroy_merge(index, m_destroy.at(other));
	destroy(other);
}
void Items::setQuality(ItemIndex index, uint32_t quality)
{
	m_quality.at(index) = quality;
}
void Items::setWear(ItemIndex index, Percent wear)
{
	m_percentWear.at(index) = wear;
}
void Items::setQuantity(ItemIndex index, Quantity quantity)
{
	m_quantity.at(index) = quantity;
}
void Items::unsetCraftJobForWorkPiece(ItemIndex index)
{
	m_craftJobForWorkPiece.at(index) = nullptr;
}
void Items::destroy(ItemIndex index)
{
	if(m_location.at(index) != BLOCK_INDEX_MAX)
		exit(index);
	m_onSurface.erase(index);
	m_name.maybeErase(index);
	m_canBeStockPiled.maybeErase(index);
	m_hasCargo.maybeErase(index);
	//m_area.m_hasItems.remove(index);
	Portables::destroy(index);
}
bool Items::isGeneric(ItemIndex index) const { return m_itemType.at(index)->generic; }
bool Items::isPreparedMeal(ItemIndex index) const
{
	static const ItemType& preparedMealType = ItemType::byName("prepared meal");
	return m_itemType.at(index) == &preparedMealType;
}
Mass Items::getSingleUnitMass(ItemIndex index) const 
{ 
	return std::max(1.f, m_itemType.at(index)->volume * m_materialType.at(index)->density); 
}
Mass Items::getMass(ItemIndex index) const
{
	Mass output = m_itemType.at(index)->volume * m_materialType.at(index)->density * m_quantity.at(index);
	if(m_hasCargo.contains(index))
		output += m_hasCargo.at(index).getMass();
	return output;
}
void Items::log(ItemIndex index) const
{
	std::cout << m_itemType.at(index)->name << "[" << m_materialType.at(index)->name << "]";
	if(m_quantity.at(index) != 1)
		std::cout << "(" << m_quantity.at(index) << ")";
	if(m_craftJobForWorkPiece.at(index) != nullptr)
		std::cout << "{" << m_craftJobForWorkPiece.at(index)->getStep() << "}";
	Portables::log(index);
	std::cout << std::endl;
}
// Wrapper methods.
void Items::stockpile_maybeUnsetAndScheduleReset(ItemIndex index, Faction& faction, Step duration)
{
	m_canBeStockPiled.at(index).maybeUnsetAndScheduleReset(faction, duration);
}
void Items::stockpile_set(ItemIndex index, Faction& faction)
{
	m_canBeStockPiled.at(index).set(faction);
}
void Items::stockpile_maybeUnset(ItemIndex index, Faction& faction)
{
	m_canBeStockPiled.at(index).maybeUnset(faction);
}
bool Items::stockpile_canBeStockPiled(ItemIndex index, const Faction& faction) const
{
	return m_canBeStockPiled.at(index).contains(faction);
}
/*
Items::Item(ItemIndex index, const Json& data, DeserializationMemo& deserializationMemo, ItemId id) : 
	HasShape(data, deserializationMemo),
	m_hasCargo(*this),
	m_canBeStockPiled(data["canBeStockPiled"], deserializationMemo, *this), m_craftJobForWorkPiece(nullptr), m_itemType(*data["itemType"].get<const ItemType*>()),
	m_materialType(*data["materialType"].get<const MaterialType*>()),
	m_id(id), m_quality(data["quality"].get<uint32_t>()),
	m_percentWear(data["percentWear"].get<Percent>()), m_quantity(data.contains("quantity") ? data["quantity"].get<Quantity>() : 1u), m_installed(data["installed"].get<bool>()) 
	{
		if(data.contains("location"))
		{
			setLocation(data["location"].get<BlockIndex>(), &deserializationMemo.area(data["area"].get<AreaId>()));
		}
	}
void Items::load(ItemIndex index, const Json& data, DeserializationMemo& deserializationMemo)
{
	if(data.contains("craftJobForWorkPiece"))
		m_craftJobForWorkPiece = deserializationMemo.m_craftJobs.at(data["craftJobForWorkPiece"]);
	if(data.contains("cargo"))
		m_hasCargo.load(data["cargo"], deserializationMemo);
}
Json Items::toJson(ItemIndex index) const
{
	Json data = HasShape::toJson();
	data["id"] = m_id;
	data["name"] = m_name;
	if(m_location != BLOCK_INDEX_MAX)
	{
		data["location"] = m_location;
		data["area"] = m_area;
	}
	data["itemType"] = m_itemType;
	data["materialType"] = m_materialType;
	data["quality"] = m_quality;
	data["quantity"] = m_quantity;
	data["percentWear"] = m_percentWear;
	data["installed"] = m_installed;
	data["canBeStockPiled"] = m_canBeStockPiled.toJson();
	if(m_itemType.internalVolume != 0)
		data["cargo"] = m_hasCargo.toJson();
	return data;
}
// HasCargo.
void ItemHasCargo::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	m_volume = data["volume"].get<Volume>();
	m_mass = data["mass"].get<Mass>();
	if(data.contains("fluidVolume"))
	{
		m_fluidType = data["fluidType"].get<const FluidType*>();
		m_fluidVolume = data["fluidVolume"].get<Volume>();
	}
	if(data.contains("actors"))
		for(const Json& actorReference : data["actors"])
		{
			Actor& actor = deserializationMemo.actorReference(actorReference);
			m_shapes.push_back(static_cast<HasShape*>(&actor));
		}
	if(data.contains("items"))
		for(const Json& itemReference : data["items"])
		{
			Item& item = deserializationMemo.itemReference(itemReference);
			m_shapes.push_back(static_cast<HasShape*>(&item));
			m_items.push_back(&item);
		}
}
Json ItemHasCargo::toJson() const
{
	Json data{{"volume", m_volume}, {"mass", m_mass}, {"actors", Json::array()}, {"items", Json::array()}};
	for(HasShape* hasShape : m_shapes)
		if(hasShape->isItem())
			data["items"].push_back(static_cast<Item*>(hasShape)->m_id);
		else
		{
			assert(hasShape->isActor());
			data["actors"].push_back(static_cast<Actor*>(hasShape)->m_id);
		}
	if(m_fluidType)
	{
		assert(m_shapes.empty());
		data["fluidType"] = m_fluidType;
		data["fluidVolume"] = m_fluidVolume;
	}
	return data;
}
*/
void ItemHasCargo::addActor(Area& area, ActorIndex actor)
{
	Actors& actors = area.m_actors;
	assert(m_volume + actors.getVolume(actor) <= m_itemType.internalVolume);
	assert(!containsActor(actor));
	assert(m_fluidVolume == 0 && m_fluidType == nullptr);
	m_actors.push_back(actor);
	m_volume += actors.getVolume(actor);
	m_mass += actors.getMass(actor);
}
void ItemHasCargo::addItem(Area& area, ItemIndex item)
{
	Items& items = area.m_items;
	//TODO: This method does not call hasShape.exit(), which is not consistant with the behaviour of CanPickup::pickup.
	assert(m_volume + items.getVolume(item) <= m_itemType.internalVolume);
	assert(!containsItem(item));
	assert(m_fluidVolume == 0 && m_fluidType == nullptr);
	m_items.push_back(item);
	m_volume += items.getVolume(item);
	m_mass += items.getMass(item);
}
void ItemHasCargo::addFluid(const FluidType& fluidType, Volume volume)
{
	assert(m_fluidVolume + volume <= m_itemType.internalVolume);
	if(m_fluidType == nullptr)
	{
		m_fluidType = &fluidType;
		m_fluidVolume = volume;
	}
	else
	{
		assert(m_fluidType == &fluidType);
		m_fluidVolume += volume;
	}
	m_mass += volume *= fluidType.density;
}
ItemIndex ItemHasCargo::addItemGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	assert(itemType.generic);
	Items& items = area.m_items;
	for(ItemIndex item : m_items)
		if(items.getItemType(item) == itemType && items.getMaterialType(item)  == materialType)
		{
			// Add to existing stack.
			items.addQuantity(item, quantity);
			m_mass += itemType.volume * materialType.density  * quantity;
			m_volume += itemType.volume * quantity;
			return item;
		}
	// Create new stack.
	ItemIndex newItem = items.create({
		.simulation=area.m_simulation,
		.itemType=itemType,
		.materialType=materialType,
		.quantity=quantity,	
	});
	addItem(area, newItem);
	return newItem;
}
void ItemHasCargo::removeFluidVolume(const FluidType& fluidType, Volume volume)
{
	assert(m_fluidType == &fluidType);
	assert(m_fluidVolume >= volume);
	m_fluidVolume -= volume;
	if(m_fluidVolume == 0)
		m_fluidType = nullptr;
}
void ItemHasCargo::removeActor(Area& area, ActorIndex actor)
{
	assert(containsActor(actor));
	Actors& actors = area.m_actors;
	m_volume -= actors.getVolume(actor);
	m_mass -= actors.getMass(actor);
	util::removeFromVectorByValueUnordered(m_actors, actor);
}
void ItemHasCargo::removeItem(Area& area, ItemIndex item)
{
	assert(containsItem(item));
	Items& items = area.m_items;
	m_volume -= items.getVolume(item);
	m_mass -= items.getMass(item);
	util::removeFromVectorByValueUnordered(m_items, item);
}
void ItemHasCargo::removeItemGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	Items& items = area.m_items;
	for(ItemIndex item : m_items)
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
		{
			if(items.getQuantity(item) == quantity)
				// TODO: should be reusing an iterator here.
				util::removeFromVectorByValueUnordered(m_items, item);
			else
			{
				assert(quantity < items.getQuantity(item));
				items.removeQuantity(item, quantity);
			}
			m_volume -= items.getVolume(item);
			m_mass -= items.getMass(item);
			return;
		}
	assert(false);
}
ItemIndex ItemHasCargo::unloadGenericTo(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity, BlockIndex location)
{
	removeItemGeneric(area, itemType, materialType, quantity);
	return area.getBlocks().item_addGeneric(location, itemType, materialType, quantity);
}
bool ItemHasCargo::canAddActor(Area& area, ActorIndex actor) const { return m_volume + area.m_actors.getVolume(actor) <= m_itemType.internalVolume; }
bool ItemHasCargo::canAddFluid(FluidType& fluidType) const { return m_fluidType == nullptr || m_fluidType == &fluidType; }
bool ItemHasCargo::containsGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const
{
	assert(itemType.generic);
	Items& items = area.m_items;
	for(ItemIndex item : m_items)
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
			return items.getQuantity(item) >= quantity;
	return false;
}
