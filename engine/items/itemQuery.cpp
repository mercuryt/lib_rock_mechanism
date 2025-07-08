#include "itemQuery.h"
#include "items/items.h"
#include "reference.h"
#include "simulation/simulation.h"
#include "simulation/hasItems.h"
#include "definitions/materialType.h"
#include "numericTypes/types.h"
#include "area/area.h"
bool ItemQuery::query(Area& area, const ItemIndex& item) const
{
	if(m_item.exists())
		return item == m_item.getIndex(area.getItems().m_referenceData);
	Items& items = area.getItems();
	if(m_itemType != items.getItemType(item))
		return false;
	if(m_solidCategory.exists())
		return m_solidCategory == MaterialType::getMaterialTypeCategory(items.getMaterialType(item));
	if(m_solid.exists())
		return m_solid == items.getMaterialType(item);
	return true;
}
bool ItemQuery::operator==(const ItemQuery& itemQuery) const
{
	return m_item == itemQuery.m_item && m_itemType == itemQuery.m_itemType &&
		m_solidCategory == itemQuery.m_solidCategory && m_solid == itemQuery.m_solid;
}
void ItemQuery::specalize(Area& area, const ItemIndex& item)
{
	assert(m_itemType.exists() && m_item.empty() && area.getItems().getItemType(item) == m_itemType);
	m_item.setIndex(item, area.getItems().m_referenceData);
	m_itemType.clear();
	m_solid.clear();
	m_solidCategory.clear();
}
void ItemQuery::specalize(const MaterialTypeId& materialType)
{
	assert(m_solidCategory.empty() || MaterialType::getMaterialTypeCategory(materialType) == m_solidCategory);
	assert(m_solid.empty());
	assert(m_item.empty());
	m_solid = materialType;
	m_solidCategory.clear();
}
void ItemQuery::maybeSpecalize(const MaterialTypeId& materialType)
{
	if(m_item.empty() && m_solid.empty())
		specalize(materialType);
}
void ItemQuery::validate() const
{
	if(m_item.exists())
	{
		assert(m_itemType.empty());
		assert(m_solid.empty());
		assert(m_solidCategory.empty());
	}
	if(m_itemType.exists())
		assert(m_item.empty());
	if(m_solid.exists())
	{
		assert(m_solidCategory.empty());
		assert(m_itemType.exists());
	}
	if(m_solidCategory.exists())
	{
		assert(m_solid.empty());
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
	if(m_solid.exists())
		output["materialType"] = m_solid;
	if(m_solidCategory.exists())
		output["materialTypeCategory"] = m_solidCategory;
	return output;
}
void ItemQuery::load(const Json& data, Area& area)
{
	if(data.contains("item"))
		m_item.load(data["item"], area.getItems().m_referenceData);
	if(data.contains("itemType"))
		data["itemType"].get_to(m_itemType);
	if(data.contains("materialType"))
		data["materialType"].get_to(m_solid);
	if(data.contains("materialTypeCategory"))
		data["materialTypeCategory"].get_to(m_solidCategory);
}