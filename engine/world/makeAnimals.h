#pragma once
#include "../dataStructures/smallSet.h"
#include "../numericTypes/idTypes.h"
#include "../geometry/cuboidSet.h"

struct BuildArea;

struct MakeAnimals
{
	CuboidSet m_terrestrialCandidates;
	CuboidSet m_arialCandidates;
	CuboidSet m_arborialCandidates;
	CuboidSet m_aquaticCandidates;
	BuildArea& m_buildArea;
	int m_targetMass;
	int m_currentMass{0};
	int m_terrestrialTargetMass{0};
	int m_arialTargetMass{0};
	int m_arborialTargetMass{0};
	int m_aquaticTargetMass{0};
	SmallSet<AnimalSpeciesId> m_terrestrialCarnivores;
	SmallSet<AnimalSpeciesId> m_terrestrialHerbivores;
	SmallSet<AnimalSpeciesId> m_arialCarnivores;
	SmallSet<AnimalSpeciesId> m_arialHerbivores;
	SmallSet<AnimalSpeciesId> m_arborialCarnivores;
	SmallSet<AnimalSpeciesId> m_arborialHerbivores;
	SmallSet<AnimalSpeciesId> m_aquaticCarnivores;
	SmallSet<AnimalSpeciesId> m_aquaticHerbivores;
	bool m_terrestrial = false;
	bool m_arial = false;
	bool m_arborial = false;
	bool m_aquatic = false;
	MakeAnimals(BuildArea& buildArea);
	void gatherSpecies();
	void makeCandidates();
	void divideTarget();
	void spawn(int targetMass, SmallSet<AnimalSpeciesId>& species, CuboidSet& candidates);
	void spawnTerrestrial();
	void spawnArial();
	void spawnArborial();
	void spawnAquatic();
};