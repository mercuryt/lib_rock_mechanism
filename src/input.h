#pragma once

#include "types.h"
#include "onDestroy.h"
#include <memory>
#include <queue>
#include <unordered_set>
#include <mutex>

class Block;
class Actor;
class InputQueue;
class InputAction;
class Objective;
enum class NewObjectiveEmplacementType { Replace, Before, After };

using Queue = std::deque<std::unique_ptr<InputAction>>;
//TODO: for multiplayer, multiple input queues sorted by player id. Serialization.
class InputAction
{
	NewObjectiveEmplacementType m_emplacementType;
	Queue::iterator m_iterator;
protected:
	HasOnDestroySubscriptions m_onDestroySubscriptions;
	std::unordered_set<Actor*> m_actors;
	InputAction(std::unordered_set<Actor*>& m_actors, NewObjectiveEmplacementType m_emplacementType, InputQueue& inputQueue);
	InputAction(InputQueue& inputQueue);
	void insertObjective(std::unique_ptr<Objective> objective, Actor& actor);
public:
	virtual void execute() = 0;
	virtual ~InputAction() = default;
	friend class InputQueue;
};

class InputQueue
{
	Queue m_actions;
	std::mutex m_mutex;
public:
	Queue::iterator insert(std::unique_ptr<InputAction> action);
	void remove(Queue::iterator action);
	void flush();
};
