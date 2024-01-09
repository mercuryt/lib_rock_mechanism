#pragma once
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <chrono>
#include "../engine/simulation.h"
#include "../engine/area.h"
#include "../engine/block.h"
#include "../engine/input.h"
#include "gameOverlay.h"
#include "mainMenu.h"
#include "objectivePriorityPanel.h"
#include "load.h"
#include "production.h"
#include "stocksPanel.h"
#include "uniformPanel.h"
#include "actorPanel.h"
class Window final
{
	sf::RenderWindow m_window;
	sf::Font m_font;
	sf::View m_view;
	MainMenuView m_mainMenuView;
	LoadView m_loadView;
	GameOverlay m_gameOverlay;
	ObjectivePriorityView m_objectivePriorityView;
	ProductionView m_productionView;
	UniformListView m_uniformView;
	StocksView m_stocksView;
	ActorView m_actorView;
	std::unique_ptr<Simulation> m_simulation;
	Area* m_area;
	uint32_t m_scale;
	uint32_t m_z;
	//TODO: multi select.
	Cuboid m_selectedBlocks;
	std::unordered_set<Actor*> m_selectedActors;
	std::unordered_set<Item*> m_selectedItems;
	std::unordered_set<Plant*> m_selectedPlants;
	Faction* m_faction;
	std::atomic<bool> m_paused;
	std::chrono::milliseconds m_minimumMillisecondsPerFrame;
	std::chrono::milliseconds m_minimumMillisecondsPerStep;
	std::thread m_simulationThread;
	Block* m_firstCornerOfSelection;
	std::filesystem::path m_path;
	
	void drawView();
	void drawBlock(const Block& block);
	void drawActor(const Actor& actor);
	void povFromJson(const Json& data);
	[[nodiscard]] Json povToJson() const;
	[[nodiscard]] static std::chrono::milliseconds msSinceEpoch();
public:
	tgui::Gui m_gui;
	InputQueue m_inputQueue;
	Window();
	void setArea(Area& area, Block& center, uint32_t scale);
	void startLoop();
	void centerView(const Block& block);
	void setFrameRate(uint32_t);
	// Show panels.
	void hideAllPanels();
	void showMainMenu() { hideAllPanels(); m_mainMenuView.show(); }
	void showGame() { hideAllPanels(); m_gameOverlay.show(); }
	void showLoad() { hideAllPanels(); m_loadView.show(); }
	void showProduction() { hideAllPanels(); m_productionView.show(); }
	void showUniforms() { hideAllPanels(); m_uniformView.show(); }
	void showObjectivePriority(Actor& actor) { hideAllPanels(); m_objectivePriorityView.draw(actor); }
	void showStocks() { hideAllPanels(); m_stocksView.show(); }
	void showActorDetail(Actor& actor) { hideAllPanels(); m_actorView.draw(actor); }
	// Select.
	void selectBlock(Block& block);
	void selectItem(Item& item);
	void selectPlant(Plant& plant);
	void selectActor(Actor& actor);
	Cuboid getSelectedBlocks() { return m_selectedBlocks; }
	std::unordered_set<Item*> getSelectedItems() { return m_selectedItems; }
	std::unordered_set<Plant*> getSelectedPlants() { return m_selectedPlants; }
	std::unordered_set<Actor*> getSelectedActors() { return m_selectedActors; }
	[[nodiscard]] Block& getBlockUnderCursor();
	// Filesystem;
	void save();
	void load(std::filesystem::path path);
	[[nodiscard]] const Faction* getFaction() const { return m_faction;}
	[[nodiscard]] Simulation* getSimulation() { return m_simulation.get(); }
	[[nodiscard]] Area* getArea() { return m_area; }
	// String utilities.
	static std::wstring displayNameForItem(Item& item);
	static std::wstring displayNameForCraftJob(CraftJob& craftJob);
	std::string facingToString(Facing facing);
};
