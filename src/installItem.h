#pragma once
#include "project.h"
#include "threadedTask.h"
#include "objective.h"

class InstallItemProject : Project
{
	Item& m_item;
	Block& m_locaton;
	// Max one worker.
	InstallItemProject(Item& i, Block& l) : Project(l, 1), m_item(i) { }
	void onComplete()
	{
		m_location.m_hasItems.add(m_item);
		m_item.installed = true;
	}
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() { return {}; }
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() { return {{m_item,1}}; }
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() { return {}; }
	uint32_t getDelay() const { return Config::installItemDelay; }
};
class InstallItemThreadedTask : ThreadedTask
{
	InstallItemObjective m_installItemObjective;
	std::vector<Block*> m_route;
	InstallItemThreadedTask(InstallItemObjective iio) : m_installItemObjective(iio) { }
	// Search for nearby item that needs installing.
	void readStep()
	{
		auto condition = [&](Block* block)
		{
			return block->m_area->m_hasInstallItemDesignations.contains(block);
		};
		m_route = path::getForActorToPredicate(m_installItemObjective.m_actor, condition);
	}
	void writeStep()
	{
		if(m_route == nullptr)
			m_installItemObjective.m_actor.m_hasObjectives.cannotCompleteObjective();
		else
		{
			auto& hasInstallItemDesignations = m_actor.m_locaton->m_area->m_hasInstallItemDesignations;
			if(!hasInstallItemDesignations.contains(m_route.back()) || hasInstallItemDesignations.at(m_route.back()).m_reservable.isFullyReserved())
				// Canceled or reserved, try again.
				m_installItemObjective.m_threadedTask.create(m_installItemObjective);
			else
				hasInstallItemDesignations.at(m_route.back()).addWorker(m_installItemObjective.m_actor);
		}

	}
	~InstallItemThreadedTask()
	{
		m_installItemObjective.m_threadedTask.clearPointer();
	}
};
class InstallItemObjective : Objective
{
	Actor& m_actor;
	HasThreadedTask<InstallItemThreadedTask> m_threadedTask;
	InstallItemObjective(Actor& a) : m_actor(a) { }
	void execute() { m_threadedTask.create(*this); }
};
class InstallItemObjectiveType : ObjectiveType
{
	static bool canBeAssigned(Actor& actor) const
	{
		return !actor.m_locaton->m_area->m_installItemObjectives.empty();
	}
	static std::unique_ptr<Objective> makeFor(Actor& actor)
	{
		std::unique_ptr<Objective> objective = std::make_unique<InstallItemObjectiveType>(actor);
		return objective;
	}
};
class HasInstallItemDesignations
{
	std::unordered_map<Block*, InstallItemProject> m_designations;
	void add(Block& block, Item& item)
	{
		assert(!m_destinations.contains(&block));
		m_destinations.emplace(item.m_locaton, item, block);
	}
	void remove(Item& item)
	{
		assert(m_designations.contains(item.m_locaton));
		m_designations.remove(item.m_locaton);
	}
	void empty() const { return m_designations.empty(); }
};
