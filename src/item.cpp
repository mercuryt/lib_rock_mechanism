#include "util.h"
#include "item.h"
#include "block.h"
#include "area.h"
#include <iostream>
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
void Item::destroy()
{
	if(m_location != nullptr)
		exit();
	getSimulation().destroyItem(*this);
}
bool Item::isPreparedMeal() const
{
	static const ItemType& preparedMealType = ItemType::byName("prepared meal");
	return &m_itemType == &preparedMealType;
}
void Item::log() const
{
	std::cout << m_itemType.name;
	if(m_quantity != 1)
		std::cout << "(" << m_quantity << ")";
	if(m_craftJobForWorkPiece != nullptr)
		std::cout << "{" << m_craftJobForWorkPiece->getStep() << "}";
	HasShape::log();
	std::cout << std::endl;
}
// Generic.
Item::Item(Simulation& s, uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t q, CraftJob* cj):
	HasShape(s, it.shape, true, 0, q), m_quantity(q), m_id(i), m_itemType(it), m_materialType(mt), m_installed(false), m_craftJobForWorkPiece(cj), m_hasCargo(*this)
{
	assert(m_itemType.generic);
	m_volume = m_itemType.volume * m_quantity;
	m_mass = m_volume * m_materialType.density;
}
// NonGeneric.
Item::Item(Simulation& s, uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t qual, Percent pw, CraftJob* cj):
	HasShape(s, it.shape, true), m_quantity(1u), m_id(i), m_itemType(it), m_materialType(mt), m_quality(qual), m_percentWear(pw), m_installed(false), m_craftJobForWorkPiece(cj), m_hasCargo(*this)
{
	assert(!m_itemType.generic);
	m_mass = m_itemType.volume * m_materialType.density;
	m_volume = m_itemType.volume;
}
// Named.
Item::Item(Simulation& s, uint32_t i, const ItemType& it, const MaterialType& mt, std::string n, uint32_t qual, Percent pw, CraftJob* cj):
	HasShape(s, it.shape, true), m_quantity(1u), m_id(i), m_itemType(it), m_materialType(mt), m_name(n), m_quality(qual), m_percentWear(pw), m_installed(false), m_craftJobForWorkPiece(cj), m_hasCargo(*this)
{
	assert(!m_itemType.generic);
	m_mass = m_itemType.volume * m_materialType.density;
	m_volume = m_itemType.volume;
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
	Item& newItem = m_item.getSimulation().createItem(itemType, materialType, quantity);
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
	for(Item* item : m_items)
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
	return location.m_hasItems.add(itemType, materialType, quantity);
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

ItemQuery::ItemQuery(Item& item) : m_item(&item), m_itemType(nullptr), m_materialTypeCategory(nullptr), m_materialType(nullptr) { }
ItemQuery::ItemQuery(const ItemType& m_itemType) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(nullptr), m_materialType(nullptr) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(&mtc), m_materialType(nullptr) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialType& mt) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(nullptr), m_materialType(&mt) { }
bool ItemQuery::operator()(const Item& item) const
{
	if(m_item != nullptr)
		return &item == m_item;
	if(m_itemType != &item.m_itemType)
		return false;
	if(m_materialTypeCategory != nullptr)
		return m_materialTypeCategory == item.m_materialType.materialTypeCategory;
	if(m_materialType != nullptr)
		return m_materialType == &item.m_materialType;
	return true;
}
void ItemQuery::specalize(Item& item)
{
	assert(m_itemType != nullptr && m_item == nullptr && &item.m_itemType == m_itemType);
	m_item = &item;
	m_itemType = nullptr;
}
void ItemQuery::specalize(const MaterialType& materialType)
{
	assert(m_materialTypeCategory == nullptr || materialType.materialTypeCategory == m_materialTypeCategory);
	m_materialType = &materialType;
	m_materialTypeCategory = nullptr;
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
Item& BlockHasItems::add(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
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
		Item& item = m_block.m_area->m_simulation.createItem(itemType, materialType, quantity);
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
	std::vector<Block*> blocks;
	for(Block* block : m_block.getAdjacentOnSameZLevelOnly())
		if(!block->isSolid())
			blocks.push_back(block);
	for(Item* item : m_items)
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
}
