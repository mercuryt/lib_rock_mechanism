ItemContainsItemsOrFluids::ItemContainsItemsOrFluids(Item* i) : m_item(i), m_volume(0), m_mass(0), m_fluidType(nullptr) { }
void ItemContainsItemsOrFluids::add(Item& item)
{
	assert(!m_itemsAndVolumes.contains(&item));
	assert(canAdd(item));
	m_volume += item.getVolume();
	m_mass += item.getMass();
	assert(m_volume <= m_item.m_itemType.internalVolume);
	bool combined = false;
	if(item.m_itemType.generic)
		for(auto& pair : m_itemsAndVolumes)
			if(pair.first()->genericsCanCombine(item))
			{
				pair.first.m_quantity += item.m_quantity;
				item.destroy();
				combined = true;
			}
	if(!combined)
		m_itemsAndVolumes[&item] = item.getVolume();
}
void ItemContainsItemsOrFluids::add(const FluidType& fluidType, uint32_t volume)
{
	assert(canAdd(fluidType));
	m_fluidType = fluidType;
	m_volume += volume;
	m_mass += volume * fluidType.density;
}
void ItemContainsItemsOrFluids::remove(const FluidType& fluidType, uint32_t volume)
{
	assert(m_fluidType == fluidType);
	m_volume -= volume;
	m_mass -= volume * fluidType.density;
	if(m_volume == 0)
		m_fluidType = nullptr;
}
void ItemContainsItemsOrFluids::remove(Item& item)
{
	assert(m_itemsAndVolumes.contains(item));
	assert(m_volume >= item.getVolume());
	m_volume -= item.getVolume();
	m_mass -= item.getMass();
	m_itemsAndVolumes.remove(&item);
}
void ItemContainsItemsOrFluids::remove(Item& item, uint32_t m_quantity)
{
	assert(item.m_itemType.generic);
	assert(item.m_quantity >= m_quantity);
	item.m_quantity -= m_quantity;
	if(item.m_quantity == 0)
		m_itemsAndVolumes.remove(&item);
}
bool ItemContainsItemsOrFluids::canAdd(Item& item) const
{
	assert(&item != m_item);
	return m_fluidType == nullptr && m_volume + m_item.getVolume() <= m_item.m_itemType.internalVolume;
}
bool ItemContainsItemsOrFluids::canAdd(FluidType& fluidType) const
{
	return (m_fluidType == nullptr || m_fluidType == fluidType) && m_item.m_itemType.canHoldFluids && m_itemsAndVolumes.empty() && m_volume <= m_item.m_itemType.internalVolume;
}
Item::s_nextId = 1;
void Item::setVolume() const { m_mass = m_quantity * m_itemType.volume; }
void Item::setMass() const 
{ 
	m_mass = m_quantity * m_itemType.mass;
	if(m_itemType.internalVolume)
		for(Item* item : m_containsItemsOrFluids.getItems())
			m_mass += item.getMass();
}
// Generic.
Item::Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t q):
	m_id(i), m_itemType(it), m_materialType(mt), m_quantity(q), m_reservable(q), m_installed(false)
{
	assert(m_itemType.generic);
	mass = m_itemType.volume * m_materialType.mass * m_quantity;
	volume = m_itemType.volume * m_quantity;
}
// NonGeneric.
Item::Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t qual, uint32_t pw):
	m_id(i), m_itemType(it), m_materialType(mt), m_quantity(1), m_quality(qual), m_reservable(1), m_percentWear(pw), m_installed(false)
{
	assert(!m_itemType.generic);
	mass = m_itemType.volume * m_materialType.mass;
	volume = m_itemType.volume;
}
// Named.
Item::Item(uint32_t i, const ItemType& it, const MaterialType& mt, std::string n, uint32_t qual, uint32_t pw):
	m_id(i), m_itemType(it), m_materialType(mt), m_quantity(1), m_name(n), m_quality(qual), m_reservable(1), m_percentWear(pw), m_installed(false)
{
	assert(!m_itemType.generic);
	mass = m_itemType.volume * m_materialType.mass;
	volume = m_itemType.volume;
}
struct Item::Hash{ std::size_t operator()(const Item& item) const { return item.m_id; }};
// Generic items, created in local item set. 
static Item::Item& create(Area& area, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity)
{
	assert(m_itemType.generic);
	area.m_items.emplace(++s_nextId, m_itemType, m_materialType, m_quantity);
}
static Item::Item& create(Area& area, const uint32_t m_id,  const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity)
{
	assert(m_itemType.generic);
	if(!s_nextId > m_id) s_nextId = m_id + 1;
	area.m_items.emplace(m_id, m_itemType, m_materialType, m_quantity);
}
// Unnamed items, created in local item set.
static Item::Item& create(Area& area, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quality, uint32_t m_percentWear)
{
	assert(!m_itemType.generic);
	area.m_items.emplace(++s_nextId, m_itemType, m_materialType, 1, "", m_quality, m_percentWear);
}
static Item::Item& create(Area& area, const uint32_t m_id, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quality, uint32_t m_percentWear)
{
	assert(!m_itemType.generic);
	if(!s_nextId > m_id) s_nextId = m_id + 1;
	area.m_items.emplace(m_id, m_itemType, m_materialType, 1, "", m_quality, m_percentWear);
}
// Named items, created in global item set.
static Item::Item& create(const ItemType& m_itemType, const MaterialType& m_materialType, std::string m_name, uint32_t m_quality, uint32_t m_percentWear)
{
	assert(!m_itemType.generic);
	assert(!m_name.empty());
	s_globalItems.emplace(++s_nextId, m_itemType, m_materialType, 1, m_name, m_quality, m_percentWear);
}
static Item& create(const uint32_t m_id, const ItemType& m_itemType, const MaterialType& m_materialType, std::string m_name, uint32_t m_quality, uint32_t m_percentWear)
{
	assert(!m_itemType.generic);
	assert(!m_name.empty());
	if(!s_nextId > m_id) s_nextId = m_id + 1;
	s_globalItems.emplace(m_id, m_itemType, m_materialType, 1, m_name, m_quality, m_percentWear);
}
ItemQuery::ItemQuery(Item& item) : m_item(&item), m_itemType(nullptr), m_materialTypeCategory(nullptr), m_materialType(nullptr) { }
ItemQuery::ItemQuery(const ItemType& m_itemType) : m_item(nullptr), m_itemType(&m_itemType), MaterialTypeCategory(nullptr), m_materialType(nullptr) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(mtc), m_materialType(nullptr) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialType& mt) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(nullptr), m_materialType(mt) { }
bool ItemQuery::operator()(Item& item) const
{
	if(const m_item != nullptr)
		return item == m_item;
	if(m_itemType != item.m_itemType)
		return false;
	if(m_materialTypeCategory != nullptr)
		return m_materialTypeCategory == item.m_materialType.materialTypeCategory;
	if(m_materialType != nullptr)
		return m_materialType == item.m_materialType;
	return true;
}
void ItemQuery::specalize(Item& item)
{
	assert(m_itemType != nullptr && m_item == nullptr && item.m_itemType == m_itemType);
	m_item = item;
	m_itemType = nullptr;
}
void ItemQuery::specalize(MaterialType& m_materialType)
{
	assert(m_materialTypeCategory == nullptr || m_materialType.materialTypeCategory == m_materialTypeCategory);
	assert(m_materialType == nullptr);
	m_materialType = m_materialType;
	m_materialTypeCategory = nullptr;
}
// To be used by actor and vehicle.
HasItemsAndActors::HasItemsAndActors(): m_mass(0) { }
// Non generic types have Shape.
void HasItemsAndActors::add(Item& item)
{
	assert(!item.m_itemType.generic);
	assert(std::ranges::find(m_items, &item) == m_items.end());
	m_mass += item.m_itemType.volume * item.m_materialType.mass;
	m_items.push_back(&item);
}
void HasItemsAndActors::remove(Item& item)
{
	assert(!item.m_itemType.generic);
	assert(std::ranges::find(m_items, &item) != m_items.end());
	m_mass -= item.m_itemType.volume * item.m_materialType.mass;
	std::erase(m_items, &item);
}
void HasItemsAndActors::add(const ItemType& m_itemType, const MaterialType& m_materialType uint32_t m_quantity)
{
	assert(m_itemType.generic);
	m_mass += item.m_itemType.volume * item.m_materialType.mass * m_quantity;
	auto found = std::find(m_items, [&](Item* item) { return item.m_itemType == m_itemType && item.m_materialType == m_materialType; });
	// Add to stack.
	if(found != m_items.end())
	{
		found->m_quantity += m_quantity;
		found->m_reservable.setMaxReservations(found->m_quantity);
	}
	// Create.
	else
	{
		Item& item = Item::create(m_itemType, m_materialType, m_quantity);
		m_items.push_back(&item);
	}
}
void HasItemsAndActors::remove(const ItemType& m_itemType, const MaterialType& m_materialType uint32_t m_quantity)
{
	assert(m_itemType.generic);
	assert(std::ranges::find(m_items, &item) != m_items.end());
	m_mass -= item.m_itemType.volume * item.m_materialType.mass * m_quantity;
	auto found = std::find(m_items, [&](Item* item) { return item.m_itemType == m_itemType && item.m_materialType == m_materialType; });
	assert(found != m_items.end());
	assert(found->m_quantity >= m_quantity);
	// Remove all.
	if(found->m_quantity == m_quantity)
		m_items.erase(found);
	// Remove some.
	else
	{
		found->m_quantity -= m_quantity;
		found->m_reservable.setMaxReservations(found->m_quantity);
	}
}
template<class Other>
void HasItemsAndActors::tranferTo(Other& other, Item& item)
{
	other.add(item);
	remove(item);
}
template<class Other>
void HasItemsAndActors::transferTo(Other& other, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity)
{
	other.add(m_itemType, m_materialType, m_quantity);
	remove(m_itemType, m_materialType, m_quantity);
}
Item* HasItemsAndActors::get(ItemType& itemType) const
{
	auto found = std::ranges::find(m_items, itemType, &Item::ItemType);
	if(found == m_items.end())
		return nullptr;
	return &*found;
}
void HasItemsAndActors::add(Actor& actor)
{
	assert(std::ranges::find(m_actors, &actor) == m_actors.end());
	m_mass += actor.getMass();
	m_actors.push_back(&actor);
}
void HasItemsAndActors::remove(Actor& actor)
{
	assert(std::ranges::find(m_actors, &actor) != m_actors.end());
	m_mass -= actor.getMass();
	m_actors.push_back(&actor);
}
template<class Other>
void HasItemsAndActors::transferTo(Other& other, Actor& actor)
{
	assert(std::ranges::find(m_actors, &actor) != m_actors.end());
	remove(&actor);
	other.add(actor);
}
BlockHasItems::BlockHasItems(Block& b): m_volume(0) , m_block(b) { }
// Non generic types have Shape.
void BlockHasItems::add(Item& item)
{
	assert(!item.m_itemType.generic);
	assert(std::ranges::find(m_items, &item) == m_items.end());
	//TODO: rotation.
	for(auto& [m_x, m_y, m_z, v] : item.m_itemType.shape.positions)
	{
		bool impassible = block->impassible();
		Block* block = offset(m_x, m_y, m_z);
		assert(block != nullptr);
		assert(!block->isSolid());
		assert(std::ranges::find(block->m_items, &item) == m_items.end());
		block->m_blockHasItems.volume += v;
		block->m_blockHasItems.m_items.push_back(&item);
		// Invalidate move cache when impassably full.
		if(!impassible && block->impassible())
			block->m_hasActors.invalidateCache();
	}
}
void BlockHasItems::remove(Item& item)
{
	assert(!item.m_itemType.generic);
	assert(std::ranges::find(m_items, &item) != m_items.end());
	//TODO: rotation.
	for(auto& [m_x, m_y, m_z, v] : item.m_itemType.shape.positions)
	{
		Block* block = offset(m_x, m_y, m_z);
		assert(block != nullptr);
		assert(!block->isSolid());
		assert(std::ranges::find(block->m_items, &item) != m_items.end());
		bool impassible = block->impassible();
		block->m_blockHasItems.m_volume -= v;
		std::erase(block->m_blockHasItem.m_items, &item);
		// Invalidate move cache when no-longer impassably full.
		if(impassible && !block->impassible())
			block->m_hasActors.invalidateCache();
	}
}
void BlockHasItems::add(const ItemType& itemType, const MaterialType& materialType uint32_t quantity)
{
	assert(itemType.generic);
	auto found = std::find(m_items, [&](Item* item) { return item.m_itemType == itemType && item.m_materialType == materialType; });
	bool impassible = block->impassible();
	m_volume += itemType.volume * quantity;
	// Add to.
	if(found != m_items.end())
	{
		found->m_quantity += quantity;
		found->m_reservable.setMaxReservations(found->m_quantity);
	}
	// Create.
	else
	{
		Item& item = Item::create(itemType, materialType, quantity);
		m_items.push_back(&item);
	}
	//Invalidate move cache when impassably full.
	if(!impassible && block->impassible())
		block->m_hasActors.invalidateCache();
}
void BlockHasItems::remove(const ItemType& itemType, const MaterialType& materialType uint32_t quantity)
{
	assert(itemType.generic);
	assert(std::ranges::find(m_items, &item) != m_items.end());
	auto found = std::find(m_items, [&](Item* item) { return item.m_itemType == itemType && item.m_materialType == materialType; });
	assert(found != m_items.end());
	assert(found->m_quantity >= quantity);
	bool impassible = impassible();
	m_volume -= itemType.volume * quantity;
	// Remove all.
	if(found->m_quantity == quantity)
		m_items.erase(found);
	else
	{
		// Remove some.
		found->m_quantity -= quantity;
		found->m_reservable.setMaxReservations(found->m_quantity);
	}
	// Invalidate move cache when no-longer impassably full.
	if(impassible && !impassible())
		m_block.m_hasActors.invalidateCache();
}
void BlockHasItems::tranferTo(HasItems& other, Item& item)
{
	other.add(item);
	remove(item);
}
void BlockHasItems::transferTo(HasItems& other, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
{
	other.add(itemType, materialType, quantity);
	remove(itemType, materialType, quantity);
}
bool BlockHasItems::isImpassible() const { return m_volume >= Config::ImpassibleItemVolume; }
Item* BlockHasItems::get(ItemType& itemType) const
{
	auto found = std::ranges::find(m_items, itemType, &Item::ItemType);
	if(found == m_items.end())
		return nullptr;
	return &*found;
}
void BlockHasItemsAndActors::transferTo(Other& other, ToTransfer& toTransfer)
{
	other.add(toTransfer);
	remove(toTransfer);
}
