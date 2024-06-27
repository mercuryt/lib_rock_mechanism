#include "items.h"
#include "../itemType.h"
#include "../area.h"
void Items::cargo_addActor(ItemIndex index, ActorIndex actor)
{
	assert(m_itemType.at(index)->internalVolume);
	assert(m_itemType.at(index)->internalVolume > m_area.m_actors.getVolume(actor));
}
void Items::cargo_addItem(ItemIndex index, ItemIndex item)
{
	m_hasCargo[index].addItem(m_area, item);
}
void Items::cargo_addFluid(ItemIndex index, const FluidType& fluidType, CollisionVolume volume)
{
	m_hasCargo[index].addFluid(fluidType, volume);
}
void Items::cargo_removeActor(ItemIndex index, ActorIndex actor)
{
	auto hasCargo = m_hasCargo.at(index);
	hasCargo.removeActor(m_area, actor);
	if(hasCargo.empty())
		m_hasCargo.erase(index);
}
void Items::cargo_removeItem(ItemIndex index, ItemIndex item)
{
	auto hasCargo = m_hasCargo.at(index);
	hasCargo.removeItem(m_area, item);
	if(hasCargo.empty())
		m_hasCargo.erase(index);
}
bool Items::cargo_exists(ItemIndex index) const { return m_hasCargo.contains(index); }
bool Items::cargo_containsActor(ItemIndex index, ActorIndex actor) const
{
	if(!m_hasCargo.contains(index))
		return false;
	return m_hasCargo.at(index).containsActor(actor);
}
bool Items::cargo_containsItem(ItemIndex index, ItemIndex item) const
{
	if(!m_hasCargo.contains(index))
		return false;
	return m_hasCargo.at(index).containsItem(item);
}
bool Items::cargo_containsAnyFluid(ItemIndex index) const
{
	if(!m_hasCargo.contains(index))
		return false;
	return m_hasCargo.at(index).containsAnyFluid();
}
bool Items::cargo_containsFluidType(ItemIndex index, const FluidType& fluidType) const
{
	if(!m_hasCargo.contains(index))
		return false;
	return m_hasCargo.at(index).containsFluidType(fluidType);
}
Volume Items::cargo_getFluidVolume(ItemIndex index) const
{
	if(!m_hasCargo.contains(index))
		return 0;
	return m_hasCargo.at(index).getFluidVolume();
}
const FluidType& Items::cargo_getFluidType(ItemIndex index) const
{
	return m_hasCargo.at(index).getFluidType();
}
bool Items::cargo_canAddActor(ItemIndex index, ActorIndex actor) const
{
	if(m_hasCargo.contains(index))
		return m_hasCargo.at(index).canAddActor(m_area, actor);
	else
		return m_itemType.at(index)->internalVolume >= m_area.m_actors.getVolume(actor);
}
bool Items::cargo_canAddItem(ItemIndex index, ItemIndex item) const
{
	if(m_hasCargo.contains(index))
		return m_hasCargo.at(index).canAddItem(m_area, item);
	else
		return m_itemType.at(index)->internalVolume >= m_area.m_items.getVolume(item);
}
Mass Items::cargo_getMass(ItemIndex index) const
{
	return m_hasCargo.at(index).getMass();
}
const std::vector<ItemIndex>& Items::cargo_getItems(ItemIndex index) const { return m_hasCargo.at(index).getItems(); }
const std::vector<ActorIndex>& Items::cargo_getActors(ActorIndex index) const { return m_hasCargo.at(index).getActors(); }
