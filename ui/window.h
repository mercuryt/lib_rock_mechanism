#pragma once
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <chrono>
#include <memory>
#include <ratio>
#include "../engine/simulation.h"
#include "../engine/area.h"
#include "../engine/block.h"
#include "../engine/input.h"
#include "../engine/atomicBool.h"
#include "displayData.h"
#include "draw.h"
#include "editDramaPanel.h"
#include "editRealityPanel.h"
#include "editAreaPanel.h"
#include "editActorPanel.h"
#include "editStockPilePanel.h"
#include "gameOverlay.h"
#include "mainMenu.h"
#include "objectivePriorityPanel.h"
#include "load.h"
#include "production.h"
#include "stocksPanel.h"
#include "uniformPanel.h"
#include "actorPanel.h"
#include "editFactionPanel.h"
#include "dialogueBox.h"
//#include "worldParamatersPanel.h"
struct GameView final 
{
	BlockIndex* center;
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
	DialogueBoxUI m_dialogueBoxUI;
	ObjectivePriorityView m_objectivePriorityView;
	ProductionView m_productionView;
	UniformListView m_uniformView;
	StocksView m_stocksView;
	ActorView m_actorView;
	//WorldParamatersView m_worldParamatersView;
	EditRealityView m_editRealityView;
	EditActorView m_editActorView;
	EditAreaView m_editAreaView;
	EditFactionView m_editFactionView;
	EditFactionsView m_editFactionsView;
	EditStockPileView m_editStockPileView;
	EditDramaView m_editDramaView;
	std::unique_ptr<Simulation> m_simulation;
	Area* m_area = nullptr;
	uint32_t m_scale = displayData::defaultScale;
	uint32_t m_z = 0;
	std::atomic<uint16_t> m_speed = 1;
	std::unordered_map<AreaId, GameView> m_lastViewedSpotInArea;
	//TODO: multi select.
	std::unordered_set<BlockIndex*> m_selectedBlocks;
	std::unordered_set<Actor*> m_selectedActors;
	std::unordered_set<Item*> m_selectedItems;
	std::unordered_set<Plant*> m_selectedPlants;
	Faction* m_faction = nullptr;
	// AtomicBool used instead of std::atomic<bool> for atomic toggle.
	AtomicBool m_paused = true;
	std::chrono::milliseconds m_minimumTimePerFrame;
	std::chrono::milliseconds m_minimumTimePerStep;
	Draw m_draw;
	std::thread m_simulationThread;
	sf::Vector2i m_positionWhereMouseDragBegan = {0,0};
	BlockIndex* m_firstCornerOfSelection = nullptr;
	BlockIndex* m_blockUnderCursor = nullptr;
	static constexpr int gameMarginSize = 400;
	
	void povFromJson(const Json& data);
	void setZ(const uint32_t z);
	void setSpeedDisplay();
	[[nodiscard]] Json povToJson() const;
	[[nodiscard]] static std::chrono::milliseconds msSinceEpoch();
public:
	InputQueue m_inputQueue;
	bool m_editMode;
	std::atomic<bool> m_lockInput = false;
	Window();
	void setPaused(bool paused);
	void togglePaused();
	void setSpeed(uint16_t speed);
	void setArea(Area& area, GameView* gameView = nullptr);
	void startLoop();
	void centerView(const BlockIndex& block);
	void setFrameRate(uint32_t);
	void setItemToInstall(Item& item) { m_gameOverlay.m_itemBeingInstalled = &item; }
	void setItemToMove(Item& item) { m_gameOverlay.m_itemBeingMoved = &item; }
	void close() { m_window.close(); }
	[[nodiscard]] tgui::Gui& getGui() { return m_gui; }
	[[nodiscard]] sf::RenderWindow& getRenderWindow() { return m_window; }
	[[nodiscard]] GameOverlay& getGameOverlay() { return m_gameOverlay; }
	// Show panels.
	void hideAllPanels();
	void showMainMenu() { hideAllPanels(); m_mainMenuView.show(); }
	void showGame() { hideAllPanels(); m_gameOverlay.show(); }
	void showLoad() { hideAllPanels(); m_loadView.draw(); }
	void showProduction() { hideAllPanels(); m_productionView.draw(); }
	void showUniforms() { hideAllPanels(); m_uniformView.show(); }
	void showObjectivePriority(Actor& actor) { hideAllPanels(); m_objectivePriorityView.draw(actor); }
	void showStocks() { hideAllPanels(); m_stocksView.show(); }
	void showActorDetail(Actor& actor) { hideAllPanels(); m_actorView.draw(actor); }
	//void showCreateWorld() { hideAllPanels(); m_worldParamatersView.show(); }
	void showEditReality() { hideAllPanels(); m_editRealityView.draw(); }
	void showEditActor(Actor& actor) { hideAllPanels(); m_editActorView.draw(actor); }
	void showEditArea(Area* area = nullptr) { hideAllPanels(); m_editAreaView.draw(area); }
	void showEditFaction(Faction* faction = nullptr) { hideAllPanels(); m_editFactionView.draw(faction); }
	void showEditFactions() { hideAllPanels(); m_editFactionsView.draw(); }
	void showEditStockPile(StockPile* stockPile = nullptr) { hideAllPanels(); m_editStockPileView.draw(stockPile); }
	void showEditDrama(Area* area = nullptr) { hideAllPanels(); m_editDramaView.draw(area); }
	// Select.
	void deselectAll();
	void selectBlock(BlockIndex& block);
	void selectItem(Item& item);
	void selectPlant(Plant& plant);
	void selectActor(Actor& actor);
	std::unordered_set<BlockIndex*>& getSelectedBlocks() { return m_selectedBlocks; }
	std::unordered_set<Item*>& getSelectedItems() { return m_selectedItems; }
	std::unordered_set<Plant*>& getSelectedPlants() { return m_selectedPlants; }
	std::unordered_set<Actor*>& getSelectedActors() { return m_selectedActors; }
	[[nodiscard]] BlockIndex& getBlockUnderCursor();
	[[nodiscard]] BlockIndex& getBlockAtPosition(sf::Vector2i pixelPos);
	// Filesystem.
	void threadTask(std::function<void()> task);
	void save();
	void load(std::filesystem::path path);
	// Accessors.
	[[nodiscard]] Faction* getFaction() const { return m_faction;}
	void setFaction(Faction& faction) { m_faction = &faction;}
	[[nodiscard]] Simulation* getSimulation() { return m_simulation.get(); }
	void setSimulation(std::unique_ptr<Simulation> simulation) { m_simulation = std::move(simulation); }
	[[nodiscard]] Area* getArea() { return m_area; }
	// String utilities.
	static std::wstring displayNameForItem(const Item& item);
	static std::wstring displayNameForCraftJob(CraftJob& craftJob);
	std::string facingToString(Facing facing);
	friend class Draw;
};
