#include "actors.h"
#include "../config/social.h"

PsycologyWeight Actors::psycology_actorCausesFear(const ActorIndex& index, const ActorIndex& other) const
{
	if(!isEnemy(index, other))
		return {0};
	const CombatScore& score = combat_getCombatScore(index);
	const CombatScore& enemyScore = combat_getCombatScore(other);
	if(score > enemyScore)
	{
		// Other has a lower score.
		const CombatScore difference = score - enemyScore;
		float ratio = 1.f - (float)difference.get() / (float)score.get();
		// Higher ratio means more threat.
		if(ratio < Config::minimumCombatScoreDifferenceToFeelFear)
			return {0};
		// Adjust fear to scale so that minimumCombatScoreDifferenceToFeelFear is 0.
		ratio -= Config::minimumCombatScoreDifferenceToFeelFear;
		assert(ratio < 1.f);
		return {(float)PsycologyWeight::max().get() * ratio};
	}
	else
	{
		// Other has a higher score.
		const CombatScore difference = enemyScore - score;
		float ratio = (float)difference.get() / (float)score.get();
		// Higher ratio means more threat.
		// ratio == 1 means other has twice the combatScore, which maxes out Fear.
		if(ratio > 1.f)
			ratio = 1.f;
		return {(float)PsycologyWeight::max().get() * ratio};
	}
}
const Psycology& Actors::psycology_getConst(const ActorIndex& index) const { return m_psycology[index]; }
Psycology& Actors::psycology_get(const ActorIndex& index) { return m_psycology[index]; }
void Actors::psycology_event(const ActorIndex& index, const PsycologyEventType& type, const PsycologyAttribute& attribute, const PsycologyWeight& weight, const Step& duration, const Step& cooldown)
{
	PsycologyEvent psycologyEvent(type, attribute, weight);
	m_psycology[index].apply(psycologyEvent, m_area, index, duration, cooldown);
}
void Actors::psycology_event(const ActorIndex& index, const PsycologyEventType& type, const PsycologyData& delta, const Step& duration, const Step& cooldown)
{
	PsycologyEvent psycologyEvent(type, delta);
	m_psycology[index].apply(psycologyEvent, m_area, index, duration, cooldown);
}