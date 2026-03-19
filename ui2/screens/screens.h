#pragma once
class Window;
#include "../../engine/reference.h"
class StockPile;
class Uniform;
struct ImVec2;
namespace screens
{
	void begin(Window& window, const std::string& title, const ImVec2* windowSize = nullptr);
	void end();
	void mainMenu(Window& window);
	void load(Window& window);
	void gameView(Window& window);
	void actorDetails(Window& window, const ActorReference actorRef);
	void objectivePriorities(Window& window, const ActorReference actor);
	void editStockPile(Window& window, StockPile* stockPile);
	void selectUniformToEdit(Window& window);
	void editUniform(Window& window, Uniform* uniform);
	void production(Window& window);
	// Editor screens.
	void createSimulation(Window& window);
	void editSimulation(Window& window);
	void createArea(Window& window);
	void editFaction(Window& window, const FactionId actor);
	void selectFactionToEdit(Window& window);
	void editActor(Window& window, const ActorReference actorRef);
	void editDrama(Window& window, Area& area);
};