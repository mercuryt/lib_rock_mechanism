#include "items.h"
#include "../itemType.h"
#include "../area.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "../util.h"
#include "actors/actors.h"
#include "index.h"
#include "portables.h"
#include "types.h"
#include <memory>
#include <ranges>
// RemarkItemForStockPilingEvent
ReMarkItemForStockPilingEvent::ReMarkItemForStockPilingEvent(Area& area, ItemCanBeStockPiled& canBeStockPiled, FactionId faction, Step duration, const Step start) : 
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
	data["data"].get_to(m_data);
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
	auto [iter, created] = m_scheduledEvents.emplace(faction, area.m_eventSchedule);
	assert(created);
	HasScheduledEvent<ReMarkItemForStockPilingEvent>& eventHandle = iter->second;
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
// Item
void Items::onChangeAmbiantSurfaceTemperature()
{
	Blocks& blocks = m_area.getBlocks();
	for(ItemIndex index : m_onSurface)
	{
		Temperature temperature = blocks.temperature_get(m_location[index]);
		setTemperature(index, temperature);
	}
}
ItemIndex Items::create(ItemParamaters itemParamaters)
{
	ItemIndex index = HasShapes::getNextIndex().toItem();
	Portables::create(index, ItemType::getMoveType(itemParamaters.itemType), ItemType::getShape(itemParamaters.itemType), itemParamaters.location, itemParamaters.facing, itemParamaters.isStatic);
	ItemTypeId itemType = m_itemType[index] = itemParamaters.itemType;
	m_materialType[index] = itemParamaters.materialType;
	m_id[index] = itemParamaters.id.exists() ? itemParamaters.id : m_area.m_simulation.m_items.getNextId();
	m_quality[index] = itemParamaters.quality;
	m_percentWear[index] = itemParamaters.percentWear;
	m_quantity[index] = itemParamaters.quantity;
	m_referenceTarget[index] = std::make_unique<ItemReferenceTarget>(index);
	m_installed.set(index, itemParamaters.installed);
	if(ItemType::getGeneric(itemType))
	{
		assert(m_quality[index].empty());
		assert(m_percentWear[index].empty());
	}
	else
		assert(m_quantity[index] == 1);
	if(itemParamaters.location.exists())
		setLocationAndFacing(index, itemParamaters.location, itemParamaters.facing);
	assert(m_canBeStockPiled[index] == nullptr);
	return index;
}
void Items::resize(HasShapeIndex newSize)
{
	Portables::resize(newSize);
	ItemIndex size = newSize.toItem();
	m_materialType.resize(size);
	m_id.resize(size);
	m_quality.resize(size);
	m_percentWear.resize(size);
	m_quantity.resize(size);
	m_installed.resize(size);
	m_canBeStockPiled.resize(size);
	m_hasCargo.resize(size);
}
void Items::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	Portables::moveIndex(oldIndex, newIndex);
	ItemIndex o = oldIndex.toItem();
	ItemIndex n = newIndex.toItem();
	m_referenceTarget[n] = std::move(m_referenceTarget[o]);
	m_referenceTarget[n]->index = n;
	m_materialType[n] = m_materialType[o];
	m_id[n] = m_id[o];
	m_quality[n] = m_quality[o];
	m_percentWear[n] = m_percentWear[o];
	m_quantity[n] = m_quantity[o];
	m_installed.set(n, m_installed[o]);
	m_canBeStockPiled[n] = std::move(m_canBeStockPiled[o]);
	m_hasCargo[n] = std::move(m_hasCargo[o]);
	m_hasCargo[n]->updateCarrierIndexForAllCargo(m_area, n);
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : m_blocks[newIndex])
		blocks.item_updateIndex(block, o, n);
}
void Items::setLocation(ItemIndex index, BlockIndex block)
{
	assert(m_location[index].exists());
	Facing facing = m_area.getBlocks().facingToSetWhenEnteringFrom(block, m_location[index]);
	setLocationAndFacing(index, block, facing);
	Blocks& blocks = m_area.getBlocks();
	if(blocks.isOnSurface(block))
		m_onSurface.add(index);
	else
		m_onSurface.remove(index);
}
void Items::setLocationAndFacing(ItemIndex index, BlockIndex block, Facing facing)
{
	if(m_location[index].exists())
		exit(index);
	Blocks& blocks = m_area.getBlocks();
	if(isGeneric(index) && m_static[index])
	{
		ItemIndex found = blocks.item_getGeneric(block, getItemType(index), getMaterialType(index));
		if(found.exists())
		{
			merge(found, index);
			return;
		}
	}
	for(auto [x, y, z, v] : Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing))
	{
		BlockIndex occupied = blocks.offset(block, x, y, z);
		blocks.item_record(occupied, index, CollisionVolume::create(v));
	}
	if(blocks.isOnSurface(block))
		m_onSurface.add(index);
	else
		m_onSurface.remove(index);
}
void Items::exit(ItemIndex index)
{
	assert(m_location[index].exists());
	BlockIndex location = m_location[index];
	auto& blocks = m_area.getBlocks();
	for(auto [x, y, z, v] : Shape::makeOccupiedPositionsWithFacing(m_shape[index], m_facing[index]))
	{
		BlockIndex occupied = blocks.offset(location, x, y, z);
		blocks.item_erase(occupied, index);
	}
	m_location[index].clear();
	if(blocks.isOnSurface(location))
		m_onSurface.remove(index);
}
void Items::setTemperature(ItemIndex, Temperature)
{
	//TODO
}
void Items::addQuantity(ItemIndex index, Quantity delta)
{
	Quantity newQuantity = (m_quantity[index] += delta);
	if(m_reservables[index] != nullptr)
		m_reservables[index]->setMaxReservations(newQuantity);
}
// May destroy.
void Items::removeQuantity(ItemIndex index, Quantity delta)
{
	if(m_quantity[index] == delta)
		destroy(index);
	else
	{
		assert(delta < m_quantity[index]);
		Quantity newQuantity = (m_quantity[index] -= delta);
		if(m_reservables[index] != nullptr)
			m_reservables[index]->setMaxReservations(newQuantity);
	}
}
void Items::install(ItemIndex index, BlockIndex block, Facing facing, FactionId faction)
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
void Items::merge(ItemIndex index, ItemIndex other)
{
	assert(m_itemType[index] == m_itemType[other]);
	assert(m_materialType[index] == m_materialType[other]);
	if(m_reservables[other] != nullptr)
		reservable_merge(index, *m_reservables[other]);
	m_quantity[index] += m_quantity[other];
	if(m_destroy[other] != nullptr)
		onDestroy_merge(index, *m_destroy[other]);
	destroy(other);
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
	m_quantity[index] = quantity;
}
void Items::unsetCraftJobForWorkPiece(ItemIndex index)
{
	m_craftJobForWorkPiece[index] = nullptr;
}
void Items::destroy(ItemIndex index)
{
	if(m_location[index].exists())
		exit(index);
	m_onSurface.remove(index);
	m_name[index].clear();
	m_canBeStockPiled[index] = nullptr;
	m_hasCargo[index] = nullptr;
	m_area.m_simulation.m_items.removeItem(m_id[index]);
	Portables::destroy(index);
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
	return Mass::create(std::max(1u, (ItemType::getVolume(m_itemType[index]) * MaterialType::getDensity(m_materialType[index])).get()));
}
Mass Items::getMass(ItemIndex index) const
{
	Mass output = ItemType::getVolume(m_itemType[index]) * MaterialType::getDensity(m_materialType[index]) * m_quantity[index];
	if(m_hasCargo[index] != nullptr)
		output += m_hasCargo[index]->getMass();
	return output;
}
Volume Items::getVolume(ItemIndex index) const
{
	return ItemType::getVolume(m_itemType[index]) * m_quantity[index];
}
MoveTypeId Items::getMoveType(ItemIndex index) const
{
	return ItemType::getMoveType(m_itemType[index]);
}
void Items::log(ItemIndex index) const
{
	std::cout << ItemType::getName(m_itemType[index]) << "[" << MaterialType::getName(m_materialType[index]) << "]";
	if(m_quantity[index] != 1)
		std::cout << "(" << m_quantity[index].get() << ")";
	if(m_craftJobForWorkPiece[index] != nullptr)
		std::cout << "{" << m_craftJobForWorkPiece[index]->getStep().get() << "}";
	Portables::log(index);
	std::cout << std::endl;
}
// Wrapper methods.
void Items::stockpile_maybeUnsetAndScheduleReset(ItemIndex index, FactionId faction, Step duration)
{
	m_canBeStockPiled[index]->maybeUnsetAndScheduleReset(m_area, faction, duration);
}
void Items::stockpile_set(ItemIndex index, FactionId faction)
{
	m_canBeStockPiled[index]->set(faction);
}
void Items::stockpile_maybeUnset(ItemIndex index, FactionId faction)
{
	m_canBeStockPiled[index]->maybeUnset(faction);
}
bool Items::stockpile_canBeStockPiled(ItemIndex index, const FactionId faction) const
{
	return m_canBeStockPiled[index]->contains(faction);
}
void Items::load(const Json& data)
{
	static_cast<Portables*>(this)->load(data);
	data["onSurface"].get_to(m_onSurface);
	data["name"].get_to(m_name);
	data["itemType"].get_to(m_itemType);
	data["materialType"].get_to(m_materialType);
	data["id"].get_to(m_id);
	data["quality"].get_to(m_quality);
	data["quantity"].get_to(m_quantity);
	m_canBeStockPiled.resize(m_id.size());
	for(const Json& pair : data["canBeStockPiled"])
	{
		ItemIndex index = pair[0];
		auto& canBeStockPiled = m_canBeStockPiled[index] = std::make_unique<ItemCanBeStockPiled>();
		canBeStockPiled->load(pair[1], m_area);
	}
	m_craftJobForWorkPiece.resize(m_id.size());
	for(const Json& pair : data["craftJobForWorkPiece"])
	{
		ItemIndex index = pair[0].get<ItemIndex>();
		m_craftJobForWorkPiece[index] = m_area.m_simulation.getDeserializationMemo().m_craftJobs.at(pair[1]);
	}
	m_hasCargo.resize(m_id.size());
	for(const Json& pair : data["hasCargo"])
	{
		ItemIndex index = pair[0];
		m_hasCargo[index] = std::make_unique<ItemHasCargo>();
		ItemHasCargo& hasCargo = *m_hasCargo[index].get();
		nlohmann::from_json(pair[1], hasCargo);
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
void to_json(Json& data, std::unique_ptr<ItemHasCargo> hasCargo) { data = *hasCargo; }
void to_json(Json& data, std::unique_ptr<ItemCanBeStockPiled> canBeStockPiled) { data = canBeStockPiled == nullptr ? Json{false} : canBeStockPiled->toJson(); }
Json Items::toJson() const
{
	Json data = Portables::toJson();
	data["id"] = m_id;
	data["name"] = m_name;
	data["location"] = m_location;
	data["itemType"] = m_itemType;
	data["materialType"] = m_materialType;
	data["quality"] = m_quality;
	data["quantity"] = m_quantity;
	data["percentWear"] = m_percentWear;
	data["installed"] = m_installed;
	data["canBeStockPiled"] = Json::array();
	int i = 0;
	for(const auto& canBeStockPiled : m_canBeStockPiled)
	{
		if(canBeStockPiled != nullptr)
			data["canBeStockPiled"][i] = canBeStockPiled->toJson();
		++i;
	}
	data["cargo"] = Json::array();
	i = 0;
	for(const std::unique_ptr<ItemHasCargo>& cargo : m_hasCargo)
	{
		if(cargo != nullptr)
			data["canBeStockPiled"][i] = *cargo;
		++i;
	}
	return data;
}
// HasCargo.
void ItemHasCargo::addActor(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	assert(m_volume + actors.getVolume(actor) <= m_maxVolume);
	assert(!containsActor(actor));
	assert(m_fluidVolume == 0 && m_fluidType.empty());
	getActors().add(actor);
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
	getItems().add(item);
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
void ItemHasCargo::removeFluidVolume(FluidTypeId fluidType, CollisionVolume volume)
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
	getActors().remove(actor);
}
void ItemHasCargo::removeItem(Area& area, ItemIndex item)
{
	assert(containsItem(item));
	Items& items = area.getItems();
	m_volume -= items.getVolume(item);
	m_mass -= items.getMass(item);
	getItems().remove(item);
}
void ItemHasCargo::removeItemGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity)
{
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
		{
			if(items.getQuantity(item) == quantity)
				// TODO: should be reusing an iterator here.
				getItems().remove(item);
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
ItemIndex ItemHasCargo::unloadGenericTo(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity, BlockIndex location)
{
	removeItemGeneric(area, itemType, materialType, quantity);
	return area.getBlocks().item_addGeneric(location, itemType, materialType, quantity);
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
bool ItemHasCargo::canAddActor(Area& area, ActorIndex actor) const { return m_volume + area.getActors().getVolume(actor) <= m_maxVolume; }
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
