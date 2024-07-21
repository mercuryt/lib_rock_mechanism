#include "itemQuery.h"
#include "items/items.h"
#include "reference.h"
#include "simulation.h"
#include "simulation/hasItems.h"
#include "materialType.h"
#include "types.h"
#include "area.h"
ItemQuery::ItemQuery(const ItemReference& item) : m_item(item) { }
ItemQuery::ItemQuery(const ItemType& m_itemType) : m_itemType(&m_itemType) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc) : m_itemType(&m_itemType), m_materialTypeCategory(&mtc) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialType& mt) : m_itemType(&m_itemType), m_materialType(&mt) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialType* mt) : m_itemType(&m_itemType), m_materialType(mt) { }
ItemQuery::ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory* mtc, const MaterialType* mt) : m_itemType(&m_itemType), m_materialTypeCategory(mtc), m_materialType(mt) { }
ItemQuery::ItemQuery(const Json& data, Area& area) :
	m_itemType(data.contains("itemType") ? &ItemType::byName(data["itemType"].get<std::string>()) : nullptr),
	m_materialTypeCategory(data.contains("materialTypeCategory") ? &MaterialTypeCategory::byName(data["materialTypeCategory"].get<std::string>()) : nullptr),
	m_materialType(data.contains("materialType") ? &MaterialType::byName(data["materialType"].get<std::string>()) : nullptr) 
{ 
	if(data.contains("item"))
		m_item.load(data["item"], area);
}
bool ItemQuery::query(Area& area, const ItemIndex item) const
{
	if(m_item.exists())
		return item == m_item.getIndex();
	Items& items = area.getItems();
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
	assert(m_itemType != nullptr && !m_item.exists() && &area.getItems().getItemType(item) == m_itemType);
	m_item.setTarget(area.getItems().getReferenceTarget(item));
	m_itemType = nullptr;
}
void ItemQuery::specalize(const MaterialType& materialType)
{
	assert(m_materialTypeCategory == nullptr || materialType.materialTypeCategory == m_materialTypeCategory);
	m_materialType = &materialType;
	m_materialTypeCategory = nullptr;
}
void to_json(Json& data, const ItemQuery& itemQuery)
{
	if(itemQuery.m_item.exists())
		data["item"] = itemQuery.m_item;
	if(itemQuery.m_itemType != nullptr)
		data["itemType"] = itemQuery.m_itemType;
	if(itemQuery.m_materialType!= nullptr)
		data["materialType"] = itemQuery.m_materialType;
	if(itemQuery.m_materialTypeCategory!= nullptr)
		data["materialTypeCategory"] = itemQuery.m_materialTypeCategory;
}
