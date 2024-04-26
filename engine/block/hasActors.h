#pragma once
#include "../types.h"
#include <vector>

class Block;
class Actor;
class BlockHasActors
{
	Block& m_block;
	std::vector<Actor*> m_actors;
public:
	BlockHasActors(Block& b) : m_block(b) { }
	void enter(Actor& actor);
	void exit(Actor& actor);
	void setTemperature(Temperature temperature);
	[[nodiscard]] bool canStandIn() const;
	[[nodiscard]] bool contains(Actor& actor) const;
	[[nodiscard]] bool empty() const { return m_actors.empty(); }
	[[nodiscard]] Volume volumeOf(Actor& actor) const;
	[[nodiscard]] std::vector<Actor*>& getAll() { return m_actors; }
	[[nodiscard]] const std::vector<Actor*>& getAll() const { return m_actors; }
	[[nodiscard]] const std::vector<Actor*>& getAllConst() const { return m_actors; }
};
