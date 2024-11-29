#include "itemQuery.h"
#include "items/items.h"
#include "reference.h"
#include "simulation.h"
#include "simulation/hasItems.h"
#include "materialType.h"
#include "types.h"
#include "area.h"
bool ItemQuery::query(Area& area, const ItemIndex& item) const
{
	if(m_item.exists())
		return item == m_item.getIndex(area.getItems().m_referenceData);
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
	assert(m_itemType.exists() && m_item.empty() && area.getItems().getItemType(item) == m_itemType);
	m_item.setIndex(item, area.getItems().m_referenceData);
	m_itemType.clear();
	m_materialType.clear();
	m_materialTypeCategory.clear();
}
void ItemQuery::specalize(const MaterialTypeId& materialType)
{
	assert(m_materialTypeCategory.empty() || MaterialType::getMaterialTypeCategory(materialType) == m_materialTypeCategory);
	assert(m_materialType.empty());
	assert(m_item.empty());
	m_materialType = materialType;
	m_materialTypeCategory.clear();
}
void ItemQuery::maybeSpecalize(const MaterialTypeId& materialType)
{
	if(m_item.empty() && m_materialType.empty())
		specalize(materialType);
}
void ItemQuery::validate() const
{
	if(m_item.exists())
	{
		assert(m_itemType.empty());
		assert(m_materialType.empty());
		assert(m_materialTypeCategory.empty());
	}
	if(m_itemType.exists())
		assert(m_item.empty());
	if(m_materialType.exists())
	{
		assert(m_materialTypeCategory.empty());
		assert(m_itemType.exists());
	}
	if(m_materialTypeCategory.exists())
	{
		assert(m_materialType.empty());
		assert(m_itemType.exists());
	}
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
void ItemQuery::load(const Json& data)
{
	if(data.contains("item"))
		data["item"].get_to(m_item);
	if(data.contains("itemType"))
		data["itemType"].get_to(m_itemType);
	if(data.contains("materialType"))
		data["materialType"].get_to(m_materialType);
	if(data.contains("materialTypeCategory"))
		data["materialTypeCategory"].get_to(m_materialTypeCategory);
}