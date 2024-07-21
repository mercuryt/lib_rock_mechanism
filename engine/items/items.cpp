#include "items.h"
#include "../itemType.h"
#include "../area.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "../util.h"
#include "actors/actors.h"
#include "bloomHashMap.h"
#include "portables.h"
#include "types.h"
#include <memory>
#include <ranges>
// RemarkItemForStockPilingEvent
ReMarkItemForStockPilingEvent::ReMarkItemForStockPilingEvent(Simulation& simulation, ItemCanBeStockPiled& canBeStockPiled, FactionId faction, Step duration, const Step start) : 
	ScheduledEvent(simulation, duration, start),
	m_faction(faction),
	m_canBeStockPiled(canBeStockPiled) { }
void ReMarkItemForStockPilingEvent::execute(Simulation&, Area*) 
{ 
	m_canBeStockPiled.maybeSet(m_faction); 
	m_canBeStockPiled.m_scheduledEvents.erase(m_faction); 
}
void ReMarkItemForStockPilingEvent::clearReferences(Simulation&, Area*) { }
// ItemCanBeStockPiled
void ItemCanBeStockPiled::load(const Json& data, EventSchedule& eventSchedule)
{
	data["data"].get_to(m_data);
	for(const Json& pair : data["scheduledEvents"])
	{
		FactionId faction = pair[0].get<FactionId>();
		Step start = pair[1]["start"].get<Step>();
		Step duration = pair[1]["duration"].get<Step>();
		scheduleReset(eventSchedule, faction, duration, start);
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
void ItemCanBeStockPiled::scheduleReset(EventSchedule& eventSchedule, FactionId faction, Step duration, Step start)
{
	assert(!m_scheduledEvents.contains(faction));
	auto [iter, created] = m_scheduledEvents.emplace(&faction, eventSchedule);
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
	m_id.at(index) = itemParamaters.id ? itemParamaters.id : m_area.m_simulation.m_items.getNextId();
	m_quality.at(index) = itemParamaters.quality;
	m_percentWear.at(index) = itemParamaters.percentWear;
	m_quantity.at(index) = itemParamaters.quantity;
	m_referenceTarget.at(index) = std::make_unique<ItemReferenceTarget>(index);
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
	assert(m_canBeStockPiled.at(index) == nullptr);
	return index;
}
void Items::resize(HasShapeIndex newSize)
{
	Portables::resize(newSize);
	m_materialType.resize(newSize);
	m_id.resize(newSize);
	m_quality.resize(newSize);
	m_percentWear.resize(newSize);
	m_quantity.resize(newSize);
	m_installed.resize(newSize);
	m_canBeStockPiled.resize(newSize);
	m_hasCargo.resize(newSize);
}
void Items::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	Portables::moveIndex(oldIndex, newIndex);
	m_referenceTarget.at(newIndex) = std::move(m_referenceTarget.at(oldIndex));
	m_referenceTarget.at(newIndex)->index = newIndex;
	m_materialType[newIndex] = m_materialType[oldIndex];
	m_id[newIndex] = m_id[oldIndex];
	m_quality[newIndex] = m_quality[oldIndex];
	m_percentWear[newIndex] = m_percentWear[oldIndex];
	m_quantity[newIndex] = m_quantity[oldIndex];
	m_installed[newIndex] = m_installed[oldIndex];
	m_canBeStockPiled.at(newIndex) = std::move(m_canBeStockPiled.at(oldIndex));
	m_hasCargo.at(newIndex) = std::move(m_hasCargo.at(oldIndex));
	m_hasCargo.at(newIndex)->updateCarrierIndexForAllCargo(m_area, newIndex);
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
	if(m_reservables.at(index) != nullptr)
		m_reservables.at(index)->setMaxReservations(newQuantity);
}
// May destroy.
void Items::removeQuantity(ItemIndex index, Quantity delta)
{
	if(m_quantity.at(index) == delta)
		destroy(index);
	else
	{
		assert(delta < m_quantity.at(index));
		Quantity newQuantity = (m_quantity.at(index) -= delta);
		if(m_reservables.at(index) != nullptr)
			m_reservables.at(index)->setMaxReservations(newQuantity);
	}
}
void Items::install(ItemIndex index, BlockIndex block, Facing facing, FactionId faction)
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
	if(m_reservables.at(other) != nullptr)
		reservable_merge(index, *m_reservables.at(other));
	m_quantity.at(index) += m_quantity.at(other);
	if(m_destroy.at(other) != nullptr)
		onDestroy_merge(index, *m_destroy.at(other));
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
	m_name.at(index).clear();
	m_canBeStockPiled.at(index) = nullptr;
	m_hasCargo.at(index) = nullptr;
	m_area.m_simulation.m_items.removeItem(index);
	Portables::destroy(index);
}
bool Items::isGeneric(ItemIndex index) const { return m_itemType.at(index)->generic; }
bool Items::isPreparedMeal(ItemIndex index) const
{
	static const ItemType& preparedMealType = ItemType::byName("prepared meal");
	return m_itemType.at(index) == &preparedMealType;
}
CraftJob& Items::getCraftJobForWorkPiece(ItemIndex index) const 
{
	assert(isWorkPiece(index));
	return *m_craftJobForWorkPiece.at(index); 
}
Mass Items::getSingleUnitMass(ItemIndex index) const 
{ 
	return std::max(1.f, m_itemType.at(index)->volume * m_materialType.at(index)->density); 
}
Mass Items::getMass(ItemIndex index) const
{
	Mass output = m_itemType.at(index)->volume * m_materialType.at(index)->density * m_quantity.at(index);
	if(m_hasCargo.at(index) != nullptr)
		output += m_hasCargo.at(index)->getMass();
	return output;
}
Volume Items::getVolume(ItemIndex index) const
{
	return m_itemType.at(index)->volume * m_quantity.at(index);
}
const MoveType& Items::getMoveType(ItemIndex index) const
{
	return m_itemType.at(index)->moveType;
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
void Items::stockpile_maybeUnsetAndScheduleReset(ItemIndex index, FactionId faction, Step duration)
{
	m_canBeStockPiled.at(index)->maybeUnsetAndScheduleReset(m_area.m_eventSchedule, faction, duration);
}
void Items::stockpile_set(ItemIndex index, FactionId faction)
{
	m_canBeStockPiled.at(index)->set(faction);
}
void Items::stockpile_maybeUnset(ItemIndex index, FactionId faction)
{
	m_canBeStockPiled.at(index)->maybeUnset(faction);
}
bool Items::stockpile_canBeStockPiled(ItemIndex index, const FactionId faction) const
{
	return m_canBeStockPiled.at(index)->contains(faction);
}
void Items::load(const Json& data)
{
	static_cast<Portables*>(this)->load(data);
	m_onSurface = data["onSurface"].get<std::unordered_set<ItemIndex>>();
	m_name = data["name"].get<std::vector<std::wstring>>();
	m_itemType = data["itemType"].get<std::vector<const ItemType*>>();
	m_materialType = data["materialType"].get<std::vector<const MaterialType*>>();
	m_id = data["id"].get<std::vector<ItemId>>();
	m_quality = data["quality"].get<std::vector<Quality>>();
	m_quantity = data["quality"].get<std::vector<Quantity>>();
	m_canBeStockPiled.resize(m_id.size());
	for(const Json& pair : data["canBeStockPiled"])
	{
		ItemIndex index = pair[0];
		auto& canBeStockPiled = m_canBeStockPiled.at(index) = std::make_unique<ItemCanBeStockPiled>();
		canBeStockPiled->load(pair[1], m_area.m_eventSchedule);
	}
	m_craftJobForWorkPiece.resize(m_id.size());
	for(const Json& pair : data["craftJobForWorkPiece"])
	{
		ItemIndex index = pair[0].get<ItemIndex>();
		m_craftJobForWorkPiece.at(index) = m_area.m_simulation.getDeserializationMemo().m_craftJobs.at(pair[1]);
	}
	m_hasCargo.resize(m_id.size());
	for(const Json& pair : data["hasCargo"])
	{
		ItemIndex index = pair[0];
		m_hasCargo.at(index) = std::make_unique<ItemHasCargo>();
		ItemHasCargo& hasCargo = *m_hasCargo.at(index).get();
		nlohmann::from_json(pair[1], hasCargo);
	}
}
void to_json(Json& data, std::unique_ptr<ItemHasCargo> hasCargo) { data = *hasCargo; }
void to_json(Json& data, std::unique_ptr<ItemCanBeStockPiled> canBeStockPiled) { data = canBeStockPiled->toJson(); }
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
	data["canBeStockPiled"] = m_canBeStockPiled;
	data["cargo"] = m_hasCargo;
	return data;
}
// HasCargo.
void ItemHasCargo::addActor(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	assert(m_volume + actors.getVolume(actor) <= m_maxVolume);
	assert(!containsActor(actor));
	assert(m_fluidVolume == 0 && m_fluidType == nullptr);
	getActors().push_back(actor);
	m_volume += actors.getVolume(actor);
	m_mass += actors.getMass(actor);
}
void ItemHasCargo::addItem(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	//TODO: This method does not call hasShape.exit(), which is not consistant with the behaviour of CanPickup::pickup.
	assert(m_volume + items.getVolume(item) <= m_maxVolume);
	assert(!containsItem(item));
	assert(m_fluidVolume == 0 && m_fluidType == nullptr);
	getItems().push_back(item);
	m_volume += items.getVolume(item);
	m_mass += items.getMass(item);
}
void ItemHasCargo::addFluid(const FluidType& fluidType, Volume volume)
{
	assert(m_fluidVolume + volume <= m_maxVolume);
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
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
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
	Actors& actors = area.getActors();
	m_volume -= actors.getVolume(actor);
	m_mass -= actors.getMass(actor);
	util::removeFromVectorByValueUnordered(getActors(), actor);
}
void ItemHasCargo::removeItem(Area& area, ItemIndex item)
{
	assert(containsItem(item));
	Items& items = area.getItems();
	m_volume -= items.getVolume(item);
	m_mass -= items.getMass(item);
	util::removeFromVectorByValueUnordered(getItems(), item);
}
void ItemHasCargo::removeItemGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
		{
			if(items.getQuantity(item) == quantity)
				// TODO: should be reusing an iterator here.
				util::removeFromVectorByValueUnordered(getItems(), item);
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
bool ItemHasCargo::canAddFluid(FluidType& fluidType) const { return m_fluidType == nullptr || m_fluidType == &fluidType; }
bool ItemHasCargo::containsGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const
{
	assert(itemType.generic);
	Items& items = area.getItems();
	for(ItemIndex item : getItems())
		if(items.getItemType(item) == itemType && items.getMaterialType(item) == materialType)
			return items.getQuantity(item) >= quantity;
	return false;
}
