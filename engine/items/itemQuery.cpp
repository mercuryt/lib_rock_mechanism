#include "itemQuery.h"
#include "simulation.h"
#include "simulation/hasItems.h"
#include "materialType.h"
#include "types.h"
#include "area.h"
ItemQuery::ItemQuery(ItemIndex item) : m_item(item) { }
ItemQuery::ItemQuery(const ItemType& m_itemType) : m_itemType(&m_itemType) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc) : m_itemType(&m_itemType), m_materialTypeCategory(&mtc) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialType& mt) : m_itemType(&m_itemType), m_materialType(&mt) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialType* mt) : m_itemType(&m_itemType), m_materialType(mt) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory* mtc, const MaterialType* mt) : m_itemType(&m_itemType), m_materialTypeCategory(mtc), m_materialType(mt) { }
/*
ItemQuery::ItemQuery(const Json& data, DeserializationMemo& deserializationMemo) :
	m_item(data.contains("item") ? &deserializationMemo.m_simulation.m_hasItems->getById(data["item"].get<ItemId>()) : nullptr),
	m_itemType(data.contains("itemType") ? &ItemType::byName(data["itemType"].get<std::string>()) : nullptr),
	m_materialTypeCategory(data.contains("materialTypeCategory") ? &MaterialTypeCategory::byName(data["materialTypeCategory"].get<std::string>()) : nullptr),
	m_materialType(data.contains("materialType") ? &MaterialType::byName(data["materialType"].get<std::string>()) : nullptr) { }
Json ItemQuery::toJson() const
{
	Json data;
	if(m_item)
		data["item"] = m_item->m_id;
	if(m_itemType)
		data["itemType"] = m_itemType->name;
	if(m_materialType)
		data["materialType"] = m_materialType->name;
	if(m_materialTypeCategory)
		data["materialTypeCategory"] = m_materialTypeCategory->name;
	return data;
}
*/
bool ItemQuery::query(Area& area, const ItemIndex item) const
{
	if(m_item != ITEM_INDEX_MAX)
		return item == m_item;
	Items& items = area.m_items;
	if(m_itemType != &items.getItemType(item))
		return false;
	if(m_materialTypeCategory != nullptr)
		return m_materialTypeCategory == items.getMaterialType(item).materialTypeCategory;
	if(m_materialType != nullptr)
		return m_materialType == &items.getMaterialType(item);
	return true;
}
bool ItemQuery::operator==(const ItemQuery& itemQuery) const
{
	return m_item == itemQuery.m_item && m_itemType == itemQuery.m_itemType && 
		m_materialTypeCategory == itemQuery.m_materialTypeCategory && m_materialType == itemQuery.m_materialType;
}
void ItemQuery::specalize(Area& area, ItemIndex item)
{
	assert(m_itemType != nullptr && m_item == BLOCK_INDEX_MAX && &area.m_items.getItemType(item) == m_itemType);
	m_item = item;
	m_itemType = nullptr;
}
void ItemQuery::specalize(const MaterialType& materialType)
{
	assert(m_materialTypeCategory == nullptr || materialType.materialTypeCategory == m_materialTypeCategory);
	m_materialType = &materialType;
	m_materialTypeCategory = nullptr;
}

