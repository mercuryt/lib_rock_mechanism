#include "items.h"
#include "../itemType.h"
#include "../area.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "../util.h"
#include "../actors/actors.h"
#include "../index.h"
#include "../moveType.h"
#include "../portables.h"
#include "../types.h"
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
	for(ItemIndex index : m_onSurface)
	{
		assert(m_location[index].exists());
		Temperature temperature = blocks.temperature_get(m_location[index]);
		setTemperature(index, temperature);
	}
}
ItemIndex Items::create(ItemParamaters itemParamaters)
{
	ItemIndex index = ItemIndex::create(size());
	// TODO: This 'toItem' call should not be neccessary. Why does ItemIndex + int = HasShapeIndex?
	resize(index + 1);
	const MoveTypeId& moveType = ItemType::getMoveType(itemParamaters.itemType);
	Portables::create(index, moveType, ItemType::getShape(itemParamaters.itemType), itemParamaters.faction, itemParamaters.isStatic, itemParamaters.quantity);
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
	m_referenceTarget[index] = std::make_unique<ItemReferenceTarget>(index);
	if(ItemType::getGeneric(itemType))
	{
		assert(m_quality[index].empty());
		assert(m_percentWear[index].empty());
	}
	else
		assert(m_quantity[index] == 1);
	if(itemParamaters.location.exists())
		setLocationAndFacing(index, itemParamaters.location, itemParamaters.facing);
	static const MoveTypeId rolling = MoveType::byName("roll");
	static const ItemTypeId panniers = ItemType::byName("panniers");
	if(itemType == panniers  || (moveType == rolling && ItemType::getInternalVolume(itemType) != 0))
		m_area.m_hasHaulTools.registerHaulTool(m_area, index);
	return index;
}
void Items::resize(const ItemIndex& newSize)
{
	Portables::resize(newSize);
	m_canBeStockPiled.resize(newSize);
	m_craftJobForWorkPiece.resize(newSize);
	m_hasCargo.resize(newSize);
	m_id.resize(newSize);
	m_installed.resize(newSize);
	m_itemType.resize(newSize);
	m_materialType.resize(newSize);
	m_name.resize(newSize);
	m_percentWear.resize(newSize);
	m_quality.resize(newSize);
	m_quantity.resize(newSize);
	m_referenceTarget.resize(newSize);
}
void Items::moveIndex(const ItemIndex& oldIndex, const ItemIndex& newIndex)
{
	Portables::moveIndex(oldIndex, newIndex.toHasShape());
	m_canBeStockPiled[newIndex] = std::move(m_canBeStockPiled[oldIndex]);
	m_craftJobForWorkPiece[newIndex] = m_craftJobForWorkPiece[oldIndex];
	m_hasCargo[newIndex] = std::move(m_hasCargo[oldIndex]);
	if(m_hasCargo[newIndex] != nullptr)
		m_hasCargo[newIndex]->updateCarrierIndexForAllCargo(m_area, newIndex);
	m_id[newIndex] = m_id[oldIndex];
	m_installed.set(newIndex, m_installed[oldIndex]);
	m_itemType[newIndex] = m_itemType[oldIndex];
	m_materialType[newIndex] = m_materialType[oldIndex];
	m_name[newIndex] = m_name[oldIndex];
	m_percentWear[newIndex] = m_percentWear[oldIndex];
	m_quality[newIndex] = m_quality[oldIndex];
	m_quantity[newIndex] = m_quantity[oldIndex];
	m_referenceTarget[newIndex] = std::move(m_referenceTarget[oldIndex]);
	m_referenceTarget[newIndex]->index = newIndex;
	if(m_onSurface.contains(oldIndex))
	{
		m_onSurface.remove(oldIndex);
		m_onSurface.add(newIndex);
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
	Facing facing = m_area.getBlocks().facingToSetWhenEnteringFrom(block, m_location[index]);
	return setLocationAndFacing(index, block, facing);
}
ItemIndex Items::setLocationAndFacing(const ItemIndex& index, const BlockIndex& block, Facing facing)
{
	assert(index.exists());
	assert(block.exists());
	assert(m_location[index] != block);
	if(m_location[index].exists())
		exit(index);
	m_location[index] = block;
	m_facing[index] = facing;
	Blocks& blocks = m_area.getBlocks();
	if(isGeneric(index) && m_static[index])
	{
		// Check for existing generic item to combine with.
		ItemIndex found = blocks.item_getGeneric(block, getItemType(index), getMaterialType(index));
		if(found.exists())
			// Return the index of the found item, which may be different then it was before 'index' was destroyed by merge.
			return merge(found, index);
	}
	const Quantity& quantity = m_quantity[index];
	auto& occupiedBlocks = m_blocks[index];
	for(auto [x, y, z, v] : Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing))
	{
		BlockIndex occupied = blocks.offset(block, x, y, z);
		blocks.item_record(occupied, index, CollisionVolume::create((quantity * v).get()));
		occupiedBlocks.add(occupied);
	}
	if(blocks.isOnSurface(block))
		m_onSurface.add(index);
	else
		m_onSurface.remove(index);
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
	if(blocks.isOnSurface(location))
		m_onSurface.remove(index);
}
void Items::setTemperature(const ItemIndex&, const Temperature&)
{
	//TODO
}
void Items::addQuantity(const ItemIndex& index, const Quantity& delta)
{
	BlockIndex location = m_location[index];
	Facing facing = m_facing[index];
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
		Facing facing = m_facing[index];
		exit(index);
		m_quantity[index] -= delta;
		setLocationAndFacing(index, location, facing);
		if(m_reservables[index] != nullptr)
			m_reservables[index]->setMaxReservations(m_quantity[index]);
	}
}
void Items::install(const ItemIndex& index, const BlockIndex& block, const Facing& facing, const FactionId& faction)
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
	Facing facing = m_facing[index];
	exit(index);
	m_quantity[index] += m_quantity[other];
	setLocationAndFacing(index, location, facing);
	assert(m_quantity[index] == m_reservables[index]->getMaxReservations());
	if(m_destroy[other] != nullptr)
		onDestroy_merge(index, *m_destroy[other]);
	// Store a reference to index because it's ItemIndex may change when other is destroyed.
	ItemReference ref = index.toReference(m_area);
	destroy(other);
	return ref.getIndex();
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
	resize(s);
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
	Portables::log(index);
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
		m_canBeStockPiled[index]->maybeUnset(faction);
	if(m_canBeStockPiled[index]->empty())
		m_canBeStockPiled[index] = nullptr;
}
bool Items::stockpile_canBeStockPiled(const ItemIndex& index, const FactionId& faction) const
{
	if(m_canBeStockPiled[index] == nullptr)
		return false;
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
}
void Items::loadCargoAndCraftJobs(const Json& data)
{
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
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
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
void ItemHasCargo::removeFluidVolume(const FluidTypeId& fluidType, const CollisionVolume& volume)
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
	assert(false);
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
