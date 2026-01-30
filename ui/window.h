#pragma once
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <chrono>
#include <memory>
#include <ratio>
#include "../engine/simulation/simulation.h"
#include "../engine/area/area.h"
#include "../engine/input.h"
#include "../engine/geometry/cuboidSet.h"
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
#include "progressBar.h"
#include "backgroundTask.h"
#include "atomicBool.h"
//#include "worldParamatersPanel.h"
struct GameView final
{
	Point3D center;
	int32_t scale;
};
enum class SelectMode{Actors, Items, Plants, Space};
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
	int32_t m_scale = displayData::defaultScale;
	Distance m_z;
	std::atomic<int16_t> m_speed = 1;
	SmallMap<AreaId, GameView> m_lastViewedSpotInArea;
	CuboidSet m_selectedBlocks;
	SmallSet<ActorIndex> m_selectedActors;
	SmallSet<ItemIndex> m_selectedItems;
	SmallSet<PlantIndex> m_selectedPlants;
	WindowHasBackgroundTask m_backgroundTask;
	FactionId m_faction;
	// AtomicBool used instead of std::atomic<bool> for atomic toggle.
	AtomicBool m_paused = true;
	std::chrono::milliseconds m_minimumTimePerFrame;
	std::chrono::milliseconds m_minimumTimePerStep;
	Draw m_draw;
	std::thread m_simulationThread;
	sf::Vector2i m_positionWhereMouseDragBegan = {0,0};
	Point3D m_firstCornerOfSelection;
	Point3D m_blockUnderCursor;
	SelectMode m_selectMode = SelectMode::Actors;
	static constexpr int gameMarginSize = 400;

	void povFromJson(const Json& data);
	void setZ(const int32_t z);
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
	void setSpeed(int16_t speed);
	void setArea(Area& area, GameView* gameView = nullptr);
	void startLoop();
	void centerView(const Point3D& point);
	void setFrameRate(int32_t);
	void setItemToInstall(const ItemIndex& item) { m_gameOverlay.m_itemBeingInstalled = item; }
	void setItemToMove(const ItemIndex& item) { m_gameOverlay.m_itemBeingMoved = item; }
	void close() { m_window.close(); }
	void setCursor(const sf::Cursor::Type& cursor);
	[[nodiscard]] tgui::Gui& getGui() { return m_gui; }
	[[nodiscard]] sf::RenderWindow& getRenderWindow() { return m_window; }
	[[nodiscard]] GameOverlay& getGameOverlay() { return m_gameOverlay; }
	[[nodiscard]] SelectMode getSelectMode() const { return m_selectMode; }
	[[nodiscard]] int32_t getScale() const { return m_scale; }
	// Show panels.
	void hideAllPanels();
	void showMainMenu() { hideAllPanels(); m_mainMenuView.show(); }
	void showGame() { hideAllPanels(); m_gameOverlay.show(); }
	void showLoad() { hideAllPanels(); m_loadView.draw(); }
	void showProduction() { hideAllPanels(); m_productionView.draw(); }
	void showUniforms() { hideAllPanels(); m_uniformView.show(); }
	void showObjectivePriority(const ActorIndex& actor) { hideAllPanels(); m_objectivePriorityView.draw(actor); }
	void showStocks() { hideAllPanels(); m_stocksView.show(); }
	void showActorDetail(const ActorIndex& actor) { hideAllPanels(); m_actorView.draw(actor); }
	//void showCreateWorld() { hideAllPanels(); m_worldParamatersView.show(); }
	void showEditReality() { hideAllPanels(); m_editRealityView.draw(); }
	void showEditActor(const ActorIndex& actor) { hideAllPanels(); m_editActorView.draw(actor); }
	void showEditArea(Area* area = nullptr) { hideAllPanels(); m_editAreaView.draw(area); }
	void showEditFaction(const FactionId& faction = FactionId::null()) { hideAllPanels(); m_editFactionView.draw(faction); }
	void showEditFactions() { hideAllPanels(); m_editFactionsView.draw(); }
	void showEditStockPile(StockPile* stockPile = nullptr) { hideAllPanels(); m_editStockPileView.draw(stockPile); }
	void showEditDrama(Area* area = nullptr) { hideAllPanels(); m_editDramaView.draw(area); }
	// Select.
	void deselectAll();
	void selectBlock(const Point3D& point);
	void selectItem(const ItemIndex& item);
	void selectPlant(const PlantIndex& plant);
	void selectActor(const ActorIndex& actor);
	CuboidSet& getSelectedBlocks() { return m_selectedBlocks; }
	SmallSet<ItemIndex>& getSelectedItems() { return m_selectedItems; }
	SmallSet<PlantIndex>& getSelectedPlants() { return m_selectedPlants; }
	SmallSet<ActorIndex>& getSelectedActors() { return m_selectedActors; }
	[[nodiscard]] Point3D getBlockUnderCursor();
	[[nodiscard]] Point3D getBlockAtPosition(sf::Vector2i pixelPos);
	// Filesystem.
	void threadTask(std::function<void()>&& task, std::function<void()>&& callback);
	void save();
	void load(std::filesystem::path path);
	// Accessors.
	[[nodiscard]] const FactionId& getFaction() const { return m_faction;}
	void setFaction(const FactionId& faction);
	[[nodiscard]] Simulation* getSimulation() { return m_simulation.get(); }
	void setSimulation(std::unique_ptr<Simulation> simulation) { m_simulation = std::move(simulation); }
	[[nodiscard]] Area* getArea() { return m_area; }
	// String utilities.
	std::string displayNameForItem(const ItemIndex& item);
	static std::string displayNameForCraftJob(CraftJob& craftJob);
	static std::string facingToString(Facing4 facing);
	friend class Draw;
};
