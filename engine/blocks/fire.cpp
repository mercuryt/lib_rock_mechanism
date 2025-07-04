// Record ponters to fire data for use by UI.
#include "blocks.h"
#include "../definitions/materialType.h"
#include "../fire.h"
#include "../area/area.h"
#include "numericTypes/types.h"
#include <algorithm>
void Blocks::fire_maybeIgnite(const BlockIndex& index, const MaterialTypeId& materialType)
{
	if(!fire_existsForMaterialType(index, materialType))
		m_area.m_fires.ignite(index, materialType);
}
void Blocks::fire_setPointer(const BlockIndex& index, SmallMapStable<MaterialTypeId, Fire>* pointer)
{
	assert(!m_fires[index]);
	m_fires[index] = pointer;
}
void Blocks::fire_clearPointer(const BlockIndex& index)
{
	assert(m_fires[index]);
	m_fires[index] = nullptr;
}
bool Blocks::fire_exists(const BlockIndex& index) const
{
	return m_fires[index];
}
bool Blocks::fire_existsForMaterialType(const BlockIndex& index, const MaterialTypeId& materialType) const
{
	return m_fires[index]->contains(materialType);
}
FireStage Blocks::fire_getStage(const BlockIndex& index) const
{
	assert(fire_exists(index));
	FireStage output = FireStage::Smouldering;
	for(auto& pair : *m_fires[index])
	{
		const auto& stage = pair.second->m_stage;
		if(stage > output)
			output = stage;
	}
	return output;
}
Fire& Blocks::fire_get(const BlockIndex& index, const MaterialTypeId& materialType)
{
	return (*m_fires[index])[materialType];
}
