#include "hasItems.h"
SimulationHasItems::SimulationHasItems(const Json data, [[maybe_unused]] DeserializationMemo& deserializationMemo, Simulation& simulation) :
       	m_simulation(simulation), m_nextId(data["nextId"].get<ActorId>()) { }
Item& SimulationHasItems::createItem(ItemParamaters params)
{
	params.simulation = &m_simulation;
	assert(!m_items.contains(params.getId()));
	auto [iter, emplaced] = m_items.emplace(
		params.getId(),
		params
	);
	assert(emplaced);
	return iter->second;
}
// Nongeneric
// No name or id.
Item& SimulationHasItems::createItemNongeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, CraftJob* cj)
{
	const ItemId id = ++ m_nextId;
	return loadItemNongeneric(id, itemType, materialType, quality, percentWear, L"", cj);
}
// Generic
// No id.
Item& SimulationHasItems::createItemGeneric(const ItemType& itemType, const MaterialType& materialType, Quantity quantity, CraftJob* cj)
{

	const ItemId id = ++ m_nextId;
	return loadItemGeneric(id, itemType, materialType, quantity, cj);
}
// Id.
Item& SimulationHasItems::loadItemGeneric(const ItemId id, const ItemType& itemType, const MaterialType& materialType, Quantity quantity, CraftJob* cj)
{
	if(m_nextId <= id) m_nextId = id + 1;
	auto [iter, emplaced] = m_items.try_emplace(id, m_simulation, id, itemType, materialType, quantity, cj);
	assert(emplaced);
	return iter->second;
}
Item& SimulationHasItems::loadItemNongeneric(const ItemId id, const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, std::wstring name, CraftJob* cj)
{
	if(m_nextId <= id) m_nextId = id + 1;
	auto [iter, emplaced] = m_items.try_emplace(id, m_simulation, id, itemType, materialType, quality, percentWear, cj);
	assert(emplaced);
	Item& item = iter->second;
	item.m_name = name;
	return item;
}
void SimulationHasItems::destroyItem(Item& item)
{
	m_items.erase(item.m_id);
}
void SimulationHasItems::clearAll()
{
	for(auto& pair : m_items)
		pair.second.m_onDestroy.unsubscribeAll();
	m_items.clear();
}
Item& SimulationHasItems::loadItemFromJson(const Json& data, DeserializationMemo& deserializationMemo)
{
	auto id = data["id"].get<ItemId>();
	m_items.try_emplace(id, data, deserializationMemo, id);
	return m_items.at(id);
}
Item& SimulationHasItems::getById(ItemId id) 
{
	if(!m_items.contains(id))
	{
		//TODO: Load from world DB.
		assert(false);
	}
	else
		return m_items.at(id); 
	return m_items.begin()->second;
} 
Json SimulationHasItems::toJson() const
{
	return {{"nextId", m_nextId}};
}
