#pragma once
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <chrono>
#include "../engine/simulation.h"
#include "../engine/area.h"
#include "../engine/block.h"
#include "../engine/input.h"
#include "detailPanel.h"
class Window final
{
	sf::RenderWindow m_window;
	sf::Font m_font;
	sf::View m_view;
	tgui::Group::Ptr m_mainMenuGui;
	tgui::Group::Ptr m_loadGui;
	tgui::Group::Ptr m_gameGui;
	tgui::Group::Ptr m_productionGui;
	tgui::Group::Ptr m_stocksGui;
	tgui::Group::Ptr m_detailActorGui;
	tgui::Group::Ptr m_objectivePriorityGui;
	std::unique_ptr<Simulation> m_simulation;
	Area* m_area;
	uint32_t m_scale;
	uint32_t m_z;
	Block* m_selectedBlock;
	Item* m_selectedItem;
	Plant* m_selectedPlant;
	Actor* m_selectedActor;
	std::atomic<bool> m_paused;
	std::chrono::milliseconds m_minimumMillisecondsPerFrame;
	std::chrono::milliseconds m_minimumMillisecondsPerStep;
	std::thread m_simulationThread;
	
	void drawView();
	void drawBlock(const Block& block);
	void drawActor(const Actor& actor);
	void povFromJson(const Json& data);
	[[nodiscard]] Json povToJson() const;
	[[nodiscard]] static std::chrono::milliseconds msSinceEpoch();
public:
	tgui::Gui m_gui;
	Window();
	void setArea(Area& area, Block& center, uint32_t scale);
	void startLoop();
	void centerView(const Block& block);
	void setFrameRate(uint32_t);
	InputQueue m_inputQueue;
	void showMainMenu() { m_mainMenuGui->setVisible(true); m_gameGui->setVisible(false); m_loadGui->setVisible(false); }
	void showGame() { m_mainMenuGui->setVisible(false); m_gameGui->setVisible(true); m_loadGui->setVisible(false); }
	void showLoad() { m_mainMenuGui->setVisible(false); m_gameGui->setVisible(false); m_loadGui->setVisible(true); }
	void selectBlock(Block& block);
	void selectItem(Item& item);
	void selectPlant(Plant& plant);
	void selectActor(Actor& actor);
	void save(std::filesystem::path path);
	void load(std::filesystem::path path);
	static std::wstring displayNameForItem(Item& item);
};
