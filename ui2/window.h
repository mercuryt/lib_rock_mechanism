#pragma once

_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
_Pragma("GCC diagnostic pop")
#include <SDL2/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
	#include <SDL_opengles2.h>
#else
#endif
#ifdef _WIN32
	#include <windows.h>        // SetProcessDPIAware()
#endif
#ifdef __EMSCRIPTEN__
	#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif
#include <memory>
#include <fstream>
#include "backgroundTask.h"
#include "gameOverlay.h"
#include "atomicBool.h"
#include "../engine/simulation/simulation.h"
#include "../engine/numericTypes/index.h"
#include "../engine/numericTypes/idTypes.h"
#include "../engine/geometry/cuboidSet.h"

class Area;
class DateTime;
class SDL_Window;
enum class PanelId
{
	MainMenu,
	Load,
	Edit,
	LoadEdit,
	CreateSimulation,
	CreateArea,
	GameView,
};
struct POV final
{
	int x;
	int y;
	Distance z;
	float zoom;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(POV, x, y, z, zoom);
};
struct LoadResult final
{
	std::unique_ptr<Simulation> simulation;
	FactionId faction;
	AreaId area;
};
enum class SelectMode{Actors, Items, Plants, Space};
class Window final
{
public:
	GameOverlay m_gameOverlay;
	// Stable because we store a pointer to POV in window.
	SmallMapStable<AreaId, POV> m_lastViewedSpotInArea;
	CuboidSet m_selectedSpace;
	SmallSet<ActorIndex> m_selectedActors;
	SmallSet<ItemIndex> m_selectedItems;
	SmallSet<PlantIndex> m_selectedPlants;
	SDL_Point m_mousePosition;
	SDL_Window* m_window;
	SDL_Renderer* m_sdlRenderer;
	SDL_Texture* m_gameView;
	Area* m_area = nullptr;
	POV* m_pov;
	Point3D m_blockUnderCursor;
	std::atomic<float> m_speed = 1.0;
	float m_mainScale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
	AtomicBool m_paused;
	bool m_running = true;
	bool m_fullScreen = true;
	void updateSimulationList();
	[[nodiscard]] static std::chrono::milliseconds msSinceEpoch();
	std::vector<std::pair<std::string, std::filesystem::path>> m_simulationList;
	WindowHasBackgroundTask m_backgroundTask;
	ImGuiMouseCursor m_cursor = ImGuiMouseCursor_Arrow;
	PanelId m_panel = PanelId::MainMenu;
	std::unique_ptr<Simulation> m_simulation = nullptr;
	float m_dpiScaleX;
	float m_dpiScaleY;
	FactionId m_faction;
	bool m_editMode = false;
	bool m_lockInput = false;
	constexpr static ImGuiWindowFlags m_menuWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground;
	Window();
	void startLoop();
	void beginFrame();
	void render();
	void endFrame();
	void quit();
	void processEvents();
	void createSimulation(const std::string& name, const DateTime&);
	void save();
	void load(const std::filesystem::path& path);
	void deselectAll();
	void showMainMenu();
	void showCreateSimulation();
	void showCreateArea();
	void showLoad();
	void showGame();
	void showEdit();
	void setArea(Area& area);
	void color(const SDL_Color& color);
	void zoom(float zoomDelta);
	void zoom(float mouseX, float mouseY, float zoomDelta);
	void pan(float dx, float dy);
	[[nodiscard]] SDL_FPoint screenToWorld(float sx, float sy);
	[[nodiscard]] SDL_FPoint worldToScreen(float wx, float wy);
	[[nodiscard]] Area* getArea();
	[[nodiscard]] const Area* getArea() const;
	[[nodiscard]] POV makeDefaultPOV() const;
};