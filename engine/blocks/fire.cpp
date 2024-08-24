// Record ponters to fire data for use by UI.
#include "blocks.h"
#include "../materialType.h"
#include "../fire.h"
#include "types.h"
#include <algorithm>
void Blocks::fire_setPointer(BlockIndex index, std::unordered_map<MaterialTypeId, Fire, MaterialTypeId::Hash>* pointer)
{
	assert(!m_fires[index]);
	m_fires[index] = pointer;
}
void Blocks::fire_clearPointer(BlockIndex index)
{
	assert(m_fires[index]);
	m_fires[index] = nullptr;
}
bool Blocks::fire_exists(BlockIndex index) const
{
	return m_fires[index];
}
FireStage Blocks::fire_getStage(BlockIndex index) const
{
	assert(fire_exists(index));
	FireStage output = FireStage::Smouldering;
	for(auto& pair : *m_fires[index])
		if(pair.second.m_stage > output)
			output = pair.second.m_stage;
	return output;
}
Fire& Blocks::fire_get(BlockIndex index, MaterialTypeId materialType)
{
	return m_fires[index]->at(materialType);
}
