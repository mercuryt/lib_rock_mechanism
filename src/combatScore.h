#pragma once

template<class Actor>
uint32_t getCombatScore(const Actor& actor)
{
	uint32_t output = actor.m_individualCombatScore;
	// Find adjacent enemies and allies.
	std::vector<Block*> adjacentBlocksContainingAllies;
	std::vector<Block*> adjacentBlocksContainingEnemies;
	for(Block* block : actor.getAdjacentOnSameZLevelOnly())
	{
		bool containsEnemy = false;
		bool containsAlly = false;
		for(Actor* adjacent : block.m_actors)
		{
			if(!containsEnemy && adjacent->isEnemy(actor))
				containsEnemy = true;
			if(!containsAlly && adjacent->isAlly(actor))
				containsAlly = true;
		}
		// Ignore blocks which contain both enemies and allies.
		if(containsEnemy && !containsAlly)
			adjacentBlocksContainingEnemies.push_back(block);
		else if(containsAlly && !containsEnemy)
			adjacentBlocksContainingEnemies.push_back(block);
	}
	// Find the strongest ally in each adjacent block and add their individual combat score modified by config to output.
	for(Block* block : adjacentBlocksContainingAllies)
	{
		uint32_t strongestAllyCombatScore = 0;
		bool diagonalToAdjacentEnemy = block->isDiagonalToAny(adjacentBlocksContainingEnemies);
		for(Actor* ally : block->m_actors)
			if(diagonalToAdjacentEnemy || ally->m_maxAttackRange > 1)
			{
				uint32_t allyCombatScore = ally->m_individualCombatScore;
				if(allyCombatScore > strongestAllyCombatScore)
					strongestAllyCombatScore = allyCombatScore;
			}
		output += allyCombatScore * Config::adjacentAllyCombatScoreBonusModifier;
	}
	// Modify output by the number of adjacent blocks containing enemies.
	if(adjacentBlocksContainingEnemies > 1)
	{
		float flankingModifier = (adjacentBlocksContainingEnemies - 1) * Config::flankingModifier;
		output *= flankingModifier;
	}
	return output;
}
template<class Actor, class Equipment>
void generateCombatScoreAndAttackTable(Actor& actor)
{
	actor.combatScore = 0;
	uint32_t grasps = 0;
	actor.m_equipmentSet.attackTable.clear();
	// Get scores for equipment, includes base score for equipment type, user skill, equipment quality, and equipment maintinence.
	for(const Equipment* equipment : actor.m_equipment.m_equipments)
	{
		if(equipment->m_equipmentType.combatSkill != nullptr)
		{
			uint32_t equipmentTypeCombatScore = equipment->m_equipmentType.combatScore;
			uint32_t equipmentSkill = actor.m_skills.get(equipment->m_equipmentType.combatSkill);
			uint32_t equipmentQuality = equipment->m_quality;
			uint32_t equipmentWear = equipment->m_wear;
			uint32_t score = (
					(equipmentTypeCombatScore * Config::equipmentTypeCombatScoreModifier) + 
					(equipmentSkill * Config::equipmentSkillModifier) + 
					(equipmentQuality * Config::equipmentQualityModifier) + 
					(equipmentWear * Config::equipmentWearModifier) + 
				  );
			// Note: attackTable is a multimap.
			actor.attackTable[combatScore] = equipment;
			actor.combatScore += score;
		}
		assert(grasps >= equipment.m_equipmentType.grasps);
		grasps += equipment.m_equipmentType.grasps;
	}
	assert(actor.m_body.limbs >= grasps);
	actor.combatScore += (actor.m_body.limbs - grasps) * actor.m_skills.get(m_unarmedCombatSkill) * Config::unarmedCombatSkillModifier;

	uint32_t workingScore = actor.combatScore;
	std::vector<uint32_t> newScores;
	for(auto& [score, equipment] : actor.attackTable)
	{
		newScores.push_back(workingScore);
		workingScore -= score;
	}
	assert(
}
