bool EquipmentSortByLayer::operator()(Item*& a, Item*& b)
{
	assert(a->m_itemType.wearableType != nullptr);
	assert(b->m_itemType.wearableType != nullptr);
	return a->m_itemType.wearableType.layer > b->m_itemType.wearableType.layer;
}

void EquipmentSet::addEquipment(Item& equipment)
{
	assert(std::ranges::find(m_equipments, &equipment) == m_equipments.end());
	assert(equipment.wearableType != nullptr || equipment.weaponType != nullptr);
	m_mass += equipment.m_equipmentType.mass;
	m_equipments.insert(&equipment);
	if(equipment.wearableType != nullptr)
		m_wearable.insert(equipment);
}
void EquipmentSet::removeEquipment(Item& equipment)
{
	assert(std::ranges::find(m_equipments, &equipment) != m_equipments.end());
	assert(m_mass >= equipment.m_equipmentType.mass);
	m_mass -= equipment.m_equipmentType.mass;
	std::erase(m_equipments, &equipment);
	if(equipment.wearableType != nullptr)
		m_wearable.remove(equipment);
}
void EquipmentSet::modifyImpact(Hit& hit, const MaterialType& hitMaterialType, const BodyPartType& bodyPartType)
{
	// Wearable priority queue is sorted by layers so we start at the outside and pierce inwards.
	for(Item* equipment : m_wearable)
	{
		auto& wearableType = equipment->m_equipmentType.wearableType;
		if(wearableType.bodyPartsCovered.contains(&BodyPartType) && randomUtil::percentChance(wearableType.percentCoverage))
		{
			uint32_t pierceScore = (hit.force / hit.area) * hitMaterialType.hardness * Config::pierceModifier;
			uint32_t defenseScore = wearableType.defenseScore * equipment->m_materialType.hardness;
			if(pierceScore > defenseScore)
			{
				hit.area = wearableType.rigid ? bodyPartType.area : equipment->m_equipmentType.impactSpreadArea;
				hit.force -= wearableType.forceAbsorbedUnpiercedModifier * equipment->m_materialType.hardness * Config::forceAbsorbedUnpiercedModifier;
			}
			else
			{
				equipment->pierced(hit.area);
				hit.force -= wearableType.forceAbsorbedPiercedModifier * equipment->m_materialType.hardness * Config::forceAbsorbedPiercedModifier;
			}

		}
	}
	std::erase_if(m_equipments, [](Equipment& equipment){ return equipment.m_percentWear == 100; });
}
std::vector<Attack*> EquipmentSet::getAttacks()
{
	std::vector<Attack> output;
	for(Item* equipment : m_equipments)
		if(equipment->weaponType != nullptr)
			for(AttackType* attackType :
					equipment->weaponType.attackTypes)
				output.emplace_back(attackType, equipment->m_materialType, equipment);
	return output;
}
