#include "blocks.h"
#include "../area.h"
bool Blocks::designation_has(BlockIndex index, Faction& faction, BlockDesignation designation) const
{
	return m_area.m_blockDesignations.at(faction).check(index, designation);
}
void Blocks::designation_set(BlockIndex index, Faction& faction, BlockDesignation designation)
{
	m_area.m_blockDesignations.at(faction).set(index, designation);
}
void Blocks::designation_unset(BlockIndex index, Faction& faction, BlockDesignation designation)
{
	m_area.m_blockDesignations.at(faction).unset(index, designation);
}
void Blocks::designation_maybeUnset(BlockIndex index, Faction& faction, BlockDesignation designation)
{
	m_area.m_blockDesignations.at(faction).maybeUnset(index, designation);
}