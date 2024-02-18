#include "craft.h"
#include "deserializationMemo.h"
#include "eventSchedule.hpp"
#include "hasShape.h"
#include "materialType.h"
#include "util.h"
#include "item.h"
#include "block.h"
#include "area.h"
#include <iostream>
AttackType* ItemType::getRangedAttackType() const
{
	for(const AttackType& attackType : weaponData->attackTypes)
		if(attackType.projectileItemType != nullptr)
			return const_cast<AttackType*>(&attackType);
	return nullptr;
}
bool ItemType::hasRangedAttack() const { return getRangedAttackType() != nullptr; }
bool ItemType::hasMeleeAttack() const
{
	for(const AttackType& attackType : weaponData->attackTypes)
		if(attackType.projectileItemType == nullptr)
			return true;
	return false;
}
Block* ItemType::getCraftLocation(const Block& location, Facing facing) const
{
	assert(craftLocationStepTypeCategory);
	auto [x, y, z] = util::rotateOffsetToFacing(craftLocationOffset, facing);
	return location.offset(x, y, z);
}
// RemarkItemForStockPilingEvent
ReMarkItemForStockPilingEvent::ReMarkItemForStockPilingEvent(Item& i, const Faction& f, Step duration, const Step start) : ScheduledEvent(i.getSimulation(), duration, start), m_item(i), m_faction(f) { }
void ReMarkItemForStockPilingEvent::execute() 
{ 
	m_item.m_canBeStockPiled.maybeSet(m_faction); 
	m_item.m_canBeStockPiled.m_scheduledEvents.erase(&m_faction); 
}
void ReMarkItemForStockPilingEvent::clearReferences() { }
// ItemCanBeStockPiled
ItemCanBeStockPiled::ItemCanBeStockPiled(const Json& data, DeserializationMemo& deserializationMemo, Item& i) : m_item(i)
{
	if(data.contains("data"))
		for(const Json& factionData : data["data"])
			m_data.insert(&deserializationMemo.faction(factionData));
	if(data.contains("scheduledEvents"))
		for(const Json& eventData : data["scheduledEvents"])
		{
			const Faction& faction = deserializationMemo.faction(eventData[0]);
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
		for(const Faction* faction : m_data)
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
void ItemCanBeStockPiled::scheduleReset(const Faction& faction, Step duration, Step start)
{
	assert(!m_scheduledEvents.contains(&faction));
	auto [iter, created] = m_scheduledEvents.emplace(&faction, m_item.getEventSchedule());
	assert(created);
	HasScheduledEvent<ReMarkItemForStockPilingEvent>& eventHandle = iter->second;
	eventHandle.schedule(m_item, faction, duration, start);
}
// Item
void Item::setVolume() { m_volume = m_quantity * m_itemType.volume; }
void Item::setMass()
{ 
	m_mass = m_quantity * singleUnitMass();
	if(m_itemType.internalVolume)
		for(Item* item : m_hasCargo.getItems())
			m_mass += item->getMass();
}
void Item::setLocation(Block& block)
{
	if(m_location != nullptr)
		exit();
	block.m_hasItems.add(*this);
	m_canLead.onMove();
}
void Item::exit()
{
	assert(m_location != nullptr);
	if(m_location->m_outdoors)
		m_location->m_area->m_hasItems.setItemIsNotOnSurface(*this);
	m_location->m_hasItems.remove(*this);
}
void Item::setTemperature(Temperature temperature)
{
	//TODO
	(void)temperature;
}
void Item::addQuantity(uint32_t delta)
{
	m_quantity += delta;
	m_reservable.setMaxReservations(m_quantity);
}
void Item::removeQuantity(uint32_t delta)
{
	if(m_quantity == delta)
		destroy();
	assert(delta < m_quantity);
	m_quantity -= delta;
	m_reservable.setMaxReservations(m_quantity);
}
void Item::install(Block& block, Facing facing, const Faction& faction)
{
	m_facing = facing;
	setLocation(block);
	m_installed = true;
	if(m_itemType.craftLocationStepTypeCategory)
	{
		Block* craftLocation = m_itemType.getCraftLocation(block, facing);
		if(craftLocation)
			craftLocation->m_area->m_hasCraftingLocationsAndJobs.at(faction).addLocation(*m_itemType.craftLocationStepTypeCategory, *craftLocation);
	}
}
void Item::destroy()
{
	if(m_location != nullptr)
	{
		Area* area = m_location->m_area;
		exit();
		area->m_hasItems.remove(*this);
	}
	getSimulation().destroyItem(*this);
}
bool Item::isPreparedMeal() const
{
	static const ItemType& preparedMealType = ItemType::byName("prepared meal");
	return &m_itemType == &preparedMealType;
}
void Item::log() const
{
	std::cout << m_itemType.name << "[" << m_materialType.name << "]";
	if(m_quantity != 1)
		std::cout << "(" << m_quantity << ")";
	if(m_craftJobForWorkPiece != nullptr)
		std::cout << "{" << m_craftJobForWorkPiece->getStep() << "}";
	HasShape::log();
	std::cout << std::endl;
}
// Generic.
Item::Item(Simulation& s, ItemId i, const ItemType& it, const MaterialType& mt, uint32_t q, CraftJob* cj):
	HasShape(s, it.shape, true, 0, q), m_quantity(q), m_id(i), m_itemType(it), m_materialType(mt), m_installed(false), m_craftJobForWorkPiece(cj), m_hasCargo(*this), m_canBeStockPiled(*this)
{
	assert(m_itemType.generic);
	assert(m_quantity);
	m_volume = m_itemType.volume * m_quantity;
	m_mass = m_volume * m_materialType.density;
}
// NonGeneric.
Item::Item(Simulation& s, ItemId i, const ItemType& it, const MaterialType& mt, uint32_t qual, Percent pw, CraftJob* cj):
	HasShape(s, it.shape, true), m_quantity(1u), m_id(i), m_itemType(it), m_materialType(mt), m_quality(qual), m_percentWear(pw), m_installed(false), m_craftJobForWorkPiece(cj), m_hasCargo(*this), m_canBeStockPiled(*this)
{
	assert(!m_itemType.generic);
	m_mass = m_itemType.volume * m_materialType.density;
	m_volume = m_itemType.volume;
}
Item::Item(const Json& data, DeserializationMemo& deserializationMemo, ItemId id) : 
	HasShape(data, deserializationMemo),
	m_quantity(data.contains("quantity") ? data["quantity"].get<uint32_t>() : 1u),
	m_id(id), m_itemType(*data["itemType"].get<const ItemType*>()), m_materialType(*data["materialType"].get<const MaterialType*>()),
	m_quality(data["quality"].get<uint32_t>()), m_percentWear(data["percentWear"].get<Percent>()), m_installed(data["installed"].get<bool>()),
	m_craftJobForWorkPiece(nullptr), m_hasCargo(*this), m_canBeStockPiled(data["canBeStockPiled"], deserializationMemo, *this) 
	{
		if(data.contains("location"))
			setLocation(deserializationMemo.blockReference(data["location"]));
		if(data.contains("cargo"))
			m_hasCargo.load(data["cargo"], deserializationMemo);
	}
void Item::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	if(data.contains("craftJobForWorkPiece"))
		m_craftJobForWorkPiece = deserializationMemo.m_craftJobs.at(data["craftJobForWorkPiece"]);
	if(data.contains("hasCargo"))
		m_hasCargo.load(data["hasCargo"], deserializationMemo);
}
Json Item::toJson() const
{
	Json data = HasShape::toJson();
	data["id"] = m_id;
	data["name"] = m_name;
	if(m_location)
		data["location"] = m_location->positionToJson();
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
void ItemHasCargo::add(HasShape& hasShape)
{
	//TODO: This method does not call hasShape.exit(), which is not consistant with the behaviour of CanPickup::pickup.
	assert(m_volume + hasShape.getVolume() <= m_item.m_itemType.internalVolume);
	assert(!contains(hasShape));
	assert(m_fluidVolume == 0 && m_fluidType == nullptr);
	m_shapes.push_back(&hasShape);
	m_volume += hasShape.getVolume();
	m_mass += hasShape.getMass();
	if(hasShape.isItem())
		m_items.push_back(static_cast<Item*>(&hasShape));
}
void ItemHasCargo::add(const FluidType& fluidType, Volume volume)
{
	assert(m_fluidVolume + volume <= m_item.m_itemType.internalVolume);
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
Item& ItemHasCargo::add(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
{
	assert(itemType.generic);
	for(Item* item : m_items)
		if(item->m_itemType == itemType && item->m_materialType == materialType)
		{
			// Add to existing stack.
			item->addQuantity(quantity);
			m_mass += itemType.volume * materialType.density  * quantity;
			return *item;
		}
	// Create new stack.
	Item& newItem = m_item.getSimulation().createItemGeneric(itemType, materialType, quantity);
	add(newItem);
	return newItem;
}
void ItemHasCargo::remove(const FluidType& fluidType, Volume volume)
{
	assert(m_fluidType == &fluidType);
	assert(m_fluidVolume >= volume);
	m_fluidVolume -= volume;
	if(m_fluidVolume == 0)
		m_fluidType = nullptr;
}
void ItemHasCargo::removeFluidVolume(Volume volume) { remove(*m_fluidType, volume); }
void ItemHasCargo::remove(HasShape& hasShape)
{
	assert(m_volume >= hasShape.getVolume());
	m_volume -= hasShape.getVolume();
	m_mass -= hasShape.getMass();
	std::erase(m_shapes, &hasShape);
	if(hasShape.isItem())
		std::erase(m_items, &hasShape);
}
void ItemHasCargo::remove(Item& item, uint32_t quantity)
{
	assert(contains(item));
	if(item.getQuantity() == quantity)
		remove(item);
	else
	{
		assert(item.getQuantity() >= quantity);
		item.removeQuantity(quantity);
		m_volume -= item.m_itemType.volume * quantity;
		m_mass -= item.singleUnitMass() * quantity;
	}
}
void ItemHasCargo::remove(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
{
	assert(containsGeneric(itemType, materialType, quantity));
	auto copy = m_items;
	for(Item* item : copy)
		if(item->m_itemType == itemType && item->m_materialType == materialType)
		{
			assert(item->getQuantity() >= quantity);
			if(item->getQuantity() == quantity)
				remove(*item);
			else
			{
				item->removeQuantity(quantity);
				m_mass -= itemType.volume * materialType.density * quantity;
			}
		}
}
void ItemHasCargo::load(HasShape& hasShape, uint32_t quantity)
{
	if(hasShape.isGeneric())
	{
		Item& item = static_cast<Item&>(hasShape);
		assert(quantity <= item.getQuantity());
		if(item.getQuantity() > quantity)
		{
			item.removeQuantity(quantity);
			add(item.m_itemType, item.m_materialType, quantity);
			return;
		}
	}
	hasShape.exit();
	add(hasShape);
}
void ItemHasCargo::unloadTo(HasShape& hasShape, Block& location)
{
	remove(hasShape);
	hasShape.setLocation(location);
}
Item& ItemHasCargo::unloadGenericTo(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, Block& location)
{
	remove(itemType, materialType, quantity);
	return location.m_hasItems.addGeneric(itemType, materialType, quantity);
}
std::vector<Actor*> ItemHasCargo::getActors()
{
	std::vector<Actor*> output;
	for(HasShape* hasShape : m_shapes)
		if(hasShape->isActor())
			output.push_back(static_cast<Actor*>(hasShape));
	return output;
}
bool ItemHasCargo::canAdd(HasShape& hasShape) const { return m_volume + hasShape.getVolume() <= m_item.m_itemType.internalVolume; }
bool ItemHasCargo::canAdd(FluidType& fluidType) const { return m_fluidType == nullptr || m_fluidType == &fluidType; }
bool ItemHasCargo::containsGeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity) const
{
	assert(itemType.generic);
	for(Item* item : m_items)
		if(item->m_itemType == itemType && item->m_materialType == materialType)
			return item->getQuantity() >= quantity;
	return false;
}

BlockHasItems::BlockHasItems(Block& b): m_block(b) { }
void BlockHasItems::add(Item& item)
{
	assert(std::ranges::find(m_items, &item) == m_items.end());
	m_items.push_back(&item);
	m_block.m_hasShapes.enter(item);
	//TODO: optimize by storing underground status in item or shape to prevent repeted set insertions / removals.
	if(m_block.m_underground)
		m_block.m_area->m_hasItems.setItemIsNotOnSurface(item);
	else
		m_block.m_area->m_hasItems.setItemIsOnSurface(item);
}
void BlockHasItems::remove(Item& item)
{
	assert(std::ranges::find(m_items, &item) != m_items.end());
	std::erase(m_items, &item);
	m_block.m_hasShapes.exit(item);
}
Item& BlockHasItems::addGeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
{
	assert(itemType.generic);
	auto found = std::ranges::find_if(m_items, [&](Item* item) { return item->m_itemType == itemType && item->m_materialType == materialType; });
	// Add to.
	if(found != m_items.end())
	{
		m_block.m_hasShapes.addQuantity(**found, quantity);
		return **found;
	}
	// Create.
	else
	{
		Item& item = m_block.m_area->m_simulation.createItemGeneric(itemType, materialType, quantity);
		m_items.push_back(&item);
		m_block.m_hasShapes.enter(item);
		if(m_block.m_outdoors)
			m_block.m_area->m_hasItems.setItemIsOnSurface(item);
		return item;
	}
}
void BlockHasItems::remove(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
{
	assert(itemType.generic);
	auto found = std::ranges::find_if(m_items, [&](Item* item) { return item->m_itemType == itemType && item->m_materialType == materialType; });
	assert(found != m_items.end());
	assert((*found)->getQuantity() >= quantity);
	// Remove all.
	if((*found)->getQuantity() == quantity)
	{
		remove(**found);
		// TODO: don't remove if it's about to be readded, requires knowing destination / origin.
		if(m_block.m_outdoors)
			m_block.m_area->m_hasItems.setItemIsNotOnSurface(**found);
	}
	else
	{
		// Remove some.
		(*found)->removeQuantity(quantity);
		(*found)->m_reservable.setMaxReservations((*found)->getQuantity());
		m_block.m_hasShapes.removeQuantity(**found, quantity);
	}
}
void BlockHasItems::setTemperature(Temperature temperature)
{
	for(Item* item : m_items)
		item->setTemperature(temperature);
}
void BlockHasItems::disperseAll()
{
	if(m_items.empty())
		return;
	std::vector<Block*> blocks;
	for(Block* block : m_block.getAdjacentOnSameZLevelOnly())
		if(!block->isSolid())
			blocks.push_back(block);
	auto copy = m_items;
	for(Item* item : copy)
	{
		//TODO: split up stacks of generics, prefer blocks with more empty space.
		Block* block = m_block.m_area->m_simulation.m_random.getInVector(blocks);
		item->setLocation(*block);
	}
}
uint32_t BlockHasItems::getCount(const ItemType& itemType, const MaterialType& materialType) const
{
	auto found = std::ranges::find_if(m_items, [&](Item* item)
	{
		return item->m_itemType == itemType && item->m_materialType == materialType;
	});
	if(found == m_items.end())
		return 0;
	else
		return (*found)->getQuantity();
}
// TODO: buggy
bool BlockHasItems::hasInstalledItemType(const ItemType& itemType) const
{
	auto found = std::ranges::find_if(m_items, [&](Item* item) { return item->m_itemType == itemType; });
	return found != m_items.end() && (*found)->m_installed;
}
bool BlockHasItems::hasEmptyContainerWhichCanHoldFluidsCarryableBy(const Actor& actor) const
{
	for(const Item* item : m_items)
		//TODO: account for container weight when full, needs to have fluid type passed in.
		if(item->m_itemType.internalVolume != 0 && item->m_itemType.canHoldFluids && actor.m_canPickup.canPickupAny(*item))
			return true;
	return false;
}
bool BlockHasItems::hasContainerContainingFluidTypeCarryableBy(const Actor& actor, const FluidType& fluidType) const
{
	for(const Item* item : m_items)
		if(item->m_itemType.internalVolume != 0 && item->m_itemType.canHoldFluids && actor.m_canPickup.canPickupAny(*item) && item->m_hasCargo.getFluidType() == fluidType)
			return true;
	return false;
}
void AreaHasItems::setItemIsOnSurface(Item& item)
{
	//assert(!m_onSurface.contains(&item));
	m_onSurface.insert(&item);
}
void AreaHasItems::setItemIsNotOnSurface(Item& item)
{
	//assert(m_onSurface.contains(&item));
	m_onSurface.erase(&item);
}
void AreaHasItems::onChangeAmbiantSurfaceTemperature()
{
	//TODO: Optimize by not repetedly fetching ambiant.
	for(Item* item : m_onSurface)
	{
		assert(item->m_location != nullptr);
		assert(item->m_location->m_outdoors);
		item->setTemperature(item->m_location->m_blockHasTemperature.get());
	}
}
void AreaHasItems::remove(Item& item)
{
	m_onSurface.erase(&item);
	m_area.m_hasStockPiles.removeItemFromAllFactions(item);
	if(item.m_itemType.internalVolume != 0)
		m_area.m_hasHaulTools.unregisterHaulTool(item);
	item.m_reservable.clearAll();
}
