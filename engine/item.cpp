#include "craft.h"
#include "deserializationMemo.h"
#include "eventSchedule.hpp"
#include "hasShape.h"
#include "materialType.h"
#include "util.h"
#include "item.h"
#include "block.h"
#include "area.h"
#include "simulation.h"

#include <iostream>
#include <numbers>
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
// Static methods.
const ItemType& ItemType::byName(std::string name)
{
	auto found = std::ranges::find(itemTypeDataStore, name, &ItemType::name);
	assert(found != itemTypeDataStore.end());
	return *found;
}
ItemType& ItemType::byNameNonConst(std::string name)
{
	return const_cast<ItemType&>(byName(name));
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
ItemId ItemParamaters::getId()
{
	if(!id)
		id = simulation->m_nextItemId++;
	return id;
}
// Item
Item::Item(ItemParamaters itemParamaters) : HasShape(*itemParamaters.simulation, itemParamaters.itemType.shape, true, 0, itemParamaters.quantity, itemParamaters.faction), m_quantity(itemParamaters.quantity), m_id(itemParamaters.getId()), m_itemType(itemParamaters.itemType), m_materialType(itemParamaters.materialType), m_quality(itemParamaters.quality), m_percentWear(itemParamaters.percentWear), m_installed(itemParamaters.installed), m_craftJobForWorkPiece(itemParamaters.craftJob), m_hasCargo(*this), m_canBeStockPiled(*this)
{
	if(m_itemType.generic)
	{
		assert(!m_quality);
		assert(!m_percentWear);
	}
	else
		assert(m_quantity == 1);
	if(itemParamaters.location)
	{
		setLocation(*itemParamaters.location);
		m_location->m_area->m_hasItems.add(*this);
	}
}
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
	bool willCombine = isGeneric() && block.m_hasItems.getCount(m_itemType, m_materialType);
	block.m_hasItems.add(*this);
	if(!willCombine)
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
void Item::addQuantity(Quantity delta)
{
	m_quantity += delta;
	m_reservable.setMaxReservations(m_quantity);
}
void Item::removeQuantity(Quantity delta)
{
	if(m_quantity == delta)
		destroy();
	else
	{
		assert(delta < m_quantity);
		m_quantity -= delta;
		m_reservable.setMaxReservations(m_quantity);
	}
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
void Item::merge(Item& item)
{
	assert(m_itemType == item.m_itemType);
	assert(m_materialType == item.m_materialType);
	m_quantity += item.getQuantity();
	m_reservable.merge(item.m_reservable);
	m_onDestroy.merge(item.m_onDestroy);
	item.destroy();
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
Mass Item::singleUnitMass() const 
{ 
	return std::max(1.f, m_itemType.volume * m_materialType.density); 
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
Item::Item(Simulation& s, ItemId i, const ItemType& it, const MaterialType& mt, Quantity q, CraftJob* cj):
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
	m_quantity(data.contains("quantity") ? data["quantity"].get<Quantity>() : 1u),
	m_id(id), m_itemType(*data["itemType"].get<const ItemType*>()), m_materialType(*data["materialType"].get<const MaterialType*>()),
	m_mass(getMass()),
	m_quality(data["quality"].get<uint32_t>()), m_percentWear(data["percentWear"].get<Percent>()), m_installed(data["installed"].get<bool>()),
	m_craftJobForWorkPiece(nullptr), m_hasCargo(*this), m_canBeStockPiled(data["canBeStockPiled"], deserializationMemo, *this) 
	{
		if(data.contains("location"))
		{
			setLocation(deserializationMemo.blockReference(data["location"]));
			m_location->m_area->m_hasItems.add(*this);
		}
	}
void Item::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	if(data.contains("craftJobForWorkPiece"))
		m_craftJobForWorkPiece = deserializationMemo.m_craftJobs.at(data["craftJobForWorkPiece"]);
	if(data.contains("cargo"))
		m_hasCargo.load(data["cargo"], deserializationMemo);
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
Item& ItemHasCargo::add(const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
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
void ItemHasCargo::remove([[maybe_unused]] const FluidType& fluidType, Volume volume)
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
void ItemHasCargo::remove(Item& item, Quantity quantity)
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
void ItemHasCargo::remove(const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
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
void ItemHasCargo::load(HasShape& hasShape, Quantity quantity)
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
Item& ItemHasCargo::unloadGenericTo(const ItemType& itemType, const MaterialType& materialType, Quantity quantity, Block& location)
{
	remove(itemType, materialType, quantity);
	return location.m_hasItems.addGeneric(itemType, materialType, quantity);
}
void ItemHasCargo::destroyCargo(Item& item)
{
	remove(item);
	// Normal destroy only removes from area if location is set, cargo is removed explicitly.
	// TODO: prevent cargo from having destroy called on it directly.
	if(m_item.m_location)
		m_item.m_location->m_area->m_hasItems.remove(static_cast<Item&>(item));
	static_cast<Item&>(item).destroy();
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
bool ItemHasCargo::containsGeneric(const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const
{
	assert(itemType.generic);
	for(Item* item : m_items)
		if(item->m_itemType == itemType && item->m_materialType == materialType)
			return item->getQuantity() >= quantity;
	return false;
}
