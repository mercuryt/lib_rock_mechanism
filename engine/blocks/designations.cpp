#include "blocks.h"
#include "../area.h"
bool Blocks::designation_has(const BlockIndex& index, const FactionId& faction, const BlockDesignation& designation) const
{
	return m_area.m_blockDesignations.getForFaction(faction).check(index, designation);
}
void Blocks::designation_set(const BlockIndex& index, const FactionId& faction, const BlockDesignation& designation)
{
	m_area.m_blockDesignations.getForFaction(faction).set(index, designation);
}
void Blocks::designation_unset(const BlockIndex& index, const FactionId& faction, const BlockDesignation& designation)
{
	m_area.m_blockDesignations.getForFaction(faction).unset(index, designation);
}
void Blocks::designation_maybeUnset(const BlockIndex& index, const FactionId& faction, const BlockDesignation& designation)
{
	m_area.m_blockDesignations.getForFaction(faction).maybeUnset(index, designation);
}
