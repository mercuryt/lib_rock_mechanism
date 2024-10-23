#include "itemQuery.h"
#include "items/items.h"
#include "reference.h"
#include "simulation.h"
#include "simulation/hasItems.h"
#include "materialType.h"
#include "types.h"
#include "area.h"
ItemQuery::ItemQuery(const ItemReference& item) : m_item(item) { }
ItemQuery::ItemQuery(const ItemTypeId& itemType) : m_itemType(itemType) { }
ItemQuery::ItemQuery(const ItemTypeId& itemType, const MaterialCategoryTypeId& mtc) : m_itemType(itemType), m_materialTypeCategory(mtc) { }
ItemQuery::ItemQuery(const ItemTypeId& itemType, const MaterialTypeId& mt) : m_itemType(itemType), m_materialType(mt) { }
ItemQuery::ItemQuery(const ItemTypeId& itemType, const MaterialCategoryTypeId& mtc, const MaterialTypeId& mt) : m_itemType(itemType), m_materialTypeCategory(mtc), m_materialType(mt) { }
ItemQuery::ItemQuery(const Json& data, Area& area) :
	m_itemType(data.contains("itemType") ? ItemType::byName(data["itemType"].get<std::string>()) : ItemTypeId::null()),
	m_materialTypeCategory(data.contains("materialTypeCategory") ? MaterialTypeCategory::byName(data["materialTypeCategory"].get<std::string>()) : MaterialCategoryTypeId::null()),
	m_materialType(data.contains("materialType") ? MaterialType::byName(data["materialType"].get<std::string>()) : MaterialTypeId::null()) 
{ 
	if(data.contains("item"))
		m_item.load(data["item"], area);
}
Json ItemQuery::toJson() const
{
	Json output;
	if(m_item.exists())
		output["item"] = m_item;
	if(m_itemType.exists())
		output["itemType"] = m_itemType;
	if(m_materialType.exists())
		output["materialType"] = m_materialType;
	if(m_materialTypeCategory.exists())
		output["materialTypeCategory"] = m_materialTypeCategory;
	return output;
}
bool ItemQuery::query(Area& area, const ItemIndex& item) const
{
	if(m_item.exists())
		return item == m_item.getIndex();
	Items& items = area.getItems();
	if(m_itemType != items.getItemType(item))
		return false;
	if(m_materialTypeCategory.exists())
		return m_materialTypeCategory == MaterialType::getMaterialTypeCategory(items.getMaterialType(item));
	if(m_materialType.exists())
		return m_materialType == items.getMaterialType(item);
	return true;
}
bool ItemQuery::operator==(const ItemQuery& itemQuery) const
{
	return m_item == itemQuery.m_item && m_itemType == itemQuery.m_itemType && 
		m_materialTypeCategory == itemQuery.m_materialTypeCategory && m_materialType == itemQuery.m_materialType;
}
void ItemQuery::specalize(Area& area, const ItemIndex& item)
{
	assert(m_itemType.exists() && !m_item.exists() && area.getItems().getItemType(item) == m_itemType);
	m_item.setTarget(area.getItems().getReferenceTarget(item));
	m_itemType.clear();
}
void ItemQuery::specalize(const MaterialTypeId& materialType)
{
	assert(m_materialTypeCategory.empty() || MaterialType::getMaterialTypeCategory(materialType) == m_materialTypeCategory);
	m_materialType = materialType;
	m_materialTypeCategory.clear();
}
void to_json(Json& data, const ItemQuery& itemQuery)
{
	if(itemQuery.m_item.exists())
		data["item"] = itemQuery.m_item;
	if(itemQuery.m_itemType.empty())
		data["itemType"] = itemQuery.m_itemType;
	if(itemQuery.m_materialType.empty())
		data["materialType"] = itemQuery.m_materialType;
	if(itemQuery.m_materialTypeCategory.empty())
		data["materialTypeCategory"] = itemQuery.m_materialTypeCategory;
}
