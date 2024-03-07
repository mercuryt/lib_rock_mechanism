#include "itemQuery.h"
#include "simulation.h"
ItemQuery::ItemQuery(Item& item) : m_item(&item), m_itemType(nullptr), m_materialTypeCategory(nullptr), m_materialType(nullptr) { }
ItemQuery::ItemQuery(const ItemType& m_itemType) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(nullptr), m_materialType(nullptr) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(&mtc), m_materialType(nullptr) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialType& mt) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(nullptr), m_materialType(&mt) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialType* mt) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(nullptr), m_materialType(mt) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory* mtc, const MaterialType* mt) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(mtc), m_materialType(mt) { }
ItemQuery::ItemQuery(const Json& data, DeserializationMemo& deserializationMemo) :
	m_item(data.contains("item") ? &deserializationMemo.m_simulation.getItemById(data["item"].get<ItemId>()) : nullptr),
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
bool ItemQuery::query(const Item& item) const
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
bool ItemQuery::operator==(const ItemQuery& itemQuery) const
{
	return m_item == itemQuery.m_item && m_itemType == itemQuery.m_itemType && 
		m_materialTypeCategory == itemQuery.m_materialTypeCategory && m_materialType == itemQuery.m_materialType;
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

