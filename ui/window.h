#pragma once
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <chrono>
#include <memory>
#include "../engine/simulation.h"
#include "../engine/area.h"
#include "../engine/block.h"
#include "../engine/input.h"
#include "draw.h"
#include "editRealityPanel.h"
#include "editAreaPanel.h"
#include "editActorPanel.h"
#include "gameOverlay.h"
#include "mainMenu.h"
#include "objectivePriorityPanel.h"
#include "load.h"
#include "production.h"
#include "stocksPanel.h"
#include "uniformPanel.h"
#include "actorPanel.h"
#include "draw.h"
//#include "worldParamatersPanel.h"
struct GameView final 
{
	Block* center;
	uint32_t scale;
};
class Window final
{
	sf::RenderWindow m_window;
	tgui::Gui m_gui;
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
	//WorldParamatersView m_worldParamatersView;
	EditRealityView m_editRealityView;
	EditActorView m_editActorView;
	EditAreaView m_editAreaView;
	std::unique_ptr<Simulation> m_simulation;
	Area* m_area;
	uint32_t m_scale;
	uint32_t m_z;
	std::unordered_map<AreaId, GameView> m_lastViewedSpotInArea;
	//TODO: multi select.
	Cuboid m_selectedBlocks;
	std::unordered_set<Actor*> m_selectedActors;
	std::unordered_set<Item*> m_selectedItems;
	std::unordered_set<Plant*> m_selectedPlants;
	Faction* m_faction;
	std::atomic<bool> m_paused;
	std::chrono::milliseconds m_minimumMillisecondsPerFrame;
	std::chrono::milliseconds m_minimumMillisecondsPerStep;
	Draw m_draw;
	std::thread m_simulationThread;
	sf::Vector2i m_positionWhereMouseDragBegan;
	Block* m_firstCornerOfSelection;
	static constexpr int gameMarginSize = 400;
	
	void drawView();
	void povFromJson(const Json& data);
	void setZ(const uint32_t z);
	[[nodiscard]] Json povToJson() const;
	[[nodiscard]] static std::chrono::milliseconds msSinceEpoch();
public:
	InputQueue m_inputQueue;
	bool m_editMode;
	std::atomic<bool> m_lockInput = false;
	Window();
	void setPaused(bool paused);
	void setArea(Area& area, GameView* gameView = nullptr);
	void startLoop();
	void centerView(const Block& block);
	void setFrameRate(uint32_t);
	void setItemToInstall(Item& item) { m_gameOverlay.m_itemBeingInstalled = &item; }
	void close() { m_window.close(); }
	[[nodiscard]] tgui::Gui& getGui() { return m_gui; }
	[[nodiscard]] sf::RenderWindow& getRenderWindow() { return m_window; }
	[[nodiscard]] GameOverlay& getGameOverlay() { return m_gameOverlay; }
	// Show panels.
	void hideAllPanels();
	void showMainMenu() { hideAllPanels(); m_mainMenuView.show(); }
	void showGame() { hideAllPanels(); m_gameOverlay.show(); }
	void showLoad() { hideAllPanels(); m_loadView.draw(); }
	void showProduction() { hideAllPanels(); m_productionView.show(); }
	void showUniforms() { hideAllPanels(); m_uniformView.show(); }
	void showObjectivePriority(Actor& actor) { hideAllPanels(); m_objectivePriorityView.draw(actor); }
	void showStocks() { hideAllPanels(); m_stocksView.show(); }
	void showActorDetail(Actor& actor) { hideAllPanels(); m_actorView.draw(actor); }
	//void showCreateWorld() { hideAllPanels(); m_worldParamatersView.show(); }
	void showEditReality() { hideAllPanels(); m_editRealityView.draw(); }
	void showEditActor(Actor& actor) { hideAllPanels(); m_editActorView.draw(actor); }
	void showEditArea(Area* area = nullptr) { hideAllPanels(); m_editAreaView.draw(area); }
	// Select.
	void deselectAll();
	void selectBlock(Block& block);
	void selectItem(Item& item);
	void selectPlant(Plant& plant);
	void selectActor(Actor& actor);
	Cuboid getSelectedBlocks() { return m_selectedBlocks; }
	std::unordered_set<Item*>& getSelectedItems() { return m_selectedItems; }
	std::unordered_set<Plant*>& getSelectedPlants() { return m_selectedPlants; }
	std::unordered_set<Actor*>& getSelectedActors() { return m_selectedActors; }
	[[nodiscard]] Block& getBlockUnderCursor();
	[[nodiscard]] Block& getBlockAtPosition(sf::Vector2i pixelPos);
	// Filesystem.
	void save();
	void load(std::filesystem::path path);
	// Accessors.
	[[nodiscard]] const Faction* getFaction() const { return m_faction;}
	[[nodiscard]] Simulation* getSimulation() { return m_simulation.get(); }
	void setSimulation(std::unique_ptr<Simulation> simulation) { m_simulation = std::move(simulation); }
	[[nodiscard]] Area* getArea() { return m_area; }
	// String utilities.
	static std::wstring displayNameForItem(const Item& item);
	static std::wstring displayNameForCraftJob(CraftJob& craftJob);
	std::string facingToString(Facing facing);
	friend class Draw;
};
