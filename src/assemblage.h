#pragma once
#include "actor.h"
#include "item.h"
#include "publishedEvents.h"
#include <unordered_set>
#include <unordered_map>

class Assemblage
{
	std::unordered_set<Actor*> m_actors;
	Item* m_item;
	EventPublisher m_eventPublisher;
	std::unordered_set<Block*> m_possibleHaulPositions;
	std::unordered_set<Block*> m_possiblePassengerPositions;
	std::unordered_map<Actor*, Block*> m_haulPositions;
	std::unordered_map<Actor*, Block*> m_passengerPositions;
	std::unordered_set<Actor*> m_nonsentientActors;
	uint8_t m_notInPositionCount;
public:
	Assemblage(std::unordered_set<Actor*>& actors, Item* item = nullptr) : m_actors(actors), m_item(item) 
	{ 
		assignPositions();
	}
	Assemblage(Actor& actor, Item* item = nullptr) : m_item(item) 
	{ 
		m_actors.insert(&actor); 
		assignPositions();
	}
	void assignPositions()
	{
		std::vector<Actor*> actorsVector(m_actors.begin(), m_actors.end());
		std::ranges::sort_by(actorsVector, [&](Actor* a, Actor* b){ return a->getCarryMass() > b->getCarryMass(); });
		auto haulPositionsIter = m_possibleHaulPositions.begin();
		auto passengerPositionsIter = m_possiblePassengerPositions.begin();
		for(Actor* actor : actorsVector)
		{
			if(haulPositionsIter != m_possibleHaulPositions.end())
				m_haulPositions[actor] = &*haulPositionsIter++;
			else if(passengerPositionsIter != m_possiblePassengerPositions.end())
				m_passengerPositions[actor] = &*passengerPositionsIter++;
			else
				assert(false);
			if(actor.isSentient())
				command(actor);
			else
				m_nonsentientActors.insert(&actor);
		}
	}
	// Direct actors to assemble.
	void command(Actor& actor)
	{
		bool isHaul = m_haulPositions.contains(actor);
		if(isHaul)
		{
			if(actor.m_location == m_haulPositions.at(actor))
				setInPosition(actor);
			else
				actor.setDestination(m_haulPositions.at(actor));
		}
		else
		{
			assert(m_passengerPositions.contains(actor));
			if(actor.m_location == m_passengerPositions.at(actor))
				setInPosition(actor);
			else
				actor.setDestination(m_passengerPositions.at(actor));
		}
	}
	void setInPosition(Actor& actor)
	{
		--m_notInPositionCount;
		if(m_notInPositionCount == 0)
			m_eventPublisher.publish(PublishedEventType::Complete);
	}
};

