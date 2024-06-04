#pragma once


#include <unordered_map>
class HasShapes final
{
	CanLead m_canLead; // 4
	CanFollow m_canFollow; // 4
	OnDestroy m_onDestroy; // 2
	std::unordered_set<BlockIndex> m_blocks;
private:
	Simulation& m_simulation;
protected:
	Faction* m_faction = nullptr;
public:
	const Shape* m_shape = nullptr;
	BlockIndex m_location = BLOCK_INDEX_MAX;
	Area* m_area = nullptr;
	Facing m_facing= 0; 
	//TODO: Adjacent blocks offset cache?
	bool m_isUnderground = false;
protected:
	bool m_static = false;
	HasShape(Simulation& simulation, const Shape& shape, bool isStatic, Facing f = 0u, uint32_t maxReservations = 1u, Faction* faction = nullptr) :
	       	m_reservable(maxReservations), m_canLead(*this), m_canFollow(*this, simulation), m_simulation(simulation), m_faction(faction), 
		m_shape(&shape), m_location(BLOCK_INDEX_MAX), m_facing(f), m_isUnderground(false), m_static(isStatic) { }
	HasShape(const Json& data, DeserializationMemo& deserializationMemo);
};
