// Record ponters to fire data for use by UI.
#include "blocks.h"
#include "../materialType.h"
#include "../fire.h"
#include <algorithm>
void Blocks::fire_setPointer(BlockIndex index, std::unordered_map<const MaterialType*, Fire>* pointer)
{
	assert(!m_fires.at(index));
	m_fires.at(index) = pointer;
}
void Blocks::fire_clearPointer(BlockIndex index)
{
	assert(m_fires.at(index));
	m_fires.at(index) = nullptr;
}
bool Blocks::fire_exists(BlockIndex index) const
{
	return m_fires.at(index);
}
FireStage Blocks::fire_getStage(BlockIndex index) const
{
	assert(fire_exists(index));
	FireStage output = FireStage::Smouldering;
	for(auto& pair : *m_fires.at(index))
		if(pair.second.m_stage > output)
			output = pair.second.m_stage;
	return output;
}
Fire& Blocks::fire_get(BlockIndex index, const MaterialType& materialType)
{
	return m_fires.at(index)->at(&materialType);
}
