#pragma once

_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/imgui_stdlib.h"
_Pragma("GCC diagnostic pop")
#include "imguiStdString.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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
#include <functional>
#include <chrono>
#include "backgroundTask.h"
#include "gameOverlay.h"
#include "atomicBool.h"
#include "renderBuffer.h"
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
	ActorDetails,
	ObjectivePriorities,
	EditUniform,
	Production,
	SelectFactionToEdit,
	EditFaction,
	EditActor,
	EditDrama
};
constexpr PanelId overlayPanels[] = {
	PanelId::ActorDetails,
	PanelId::ObjectivePriorities,
	PanelId::EditUniform,
	PanelId::Production,
	PanelId::SelectFactionToEdit,
	PanelId::EditFaction,
	PanelId::EditActor,
	PanelId::EditDrama,
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
class Window final
{
public:
	GameOverlay m_gameOverlay;
	RenderBuffer m_renderBuffer;
	// Stable because we store a pointer to POV in window.
	SmallMapStable<AreaId, POV> m_lastViewedSpotInArea;
	std::thread m_simulationThread;
	SDL_Window* m_window = nullptr;
	SDL_Renderer* m_sdlRenderer = nullptr;
	SDL_Texture* m_gameView = nullptr;
	Area* m_area = nullptr;
	POV* m_pov = nullptr;
	std::chrono::milliseconds m_minimumTimePerStep;
	SDL_Point m_mousePosition;
	std::atomic<float> m_speed = 1.0;
	float m_mainScale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
	AtomicBool m_paused;
	bool m_running = true;
	bool m_fullScreen = true;
	bool m_shiftKey = false;
	bool m_controllKey = false;
	void updateSimulationList();
	std::vector<std::pair<std::string, std::filesystem::path>> m_simulationList;
	BackgroundTask m_backgroundTask;
	ImGuiMouseCursor m_cursor = ImGuiMouseCursor_Arrow;
	PanelId m_panel = PanelId::MainMenu;
	std::unique_ptr<Simulation> m_simulation = nullptr;
	int m_screenWidth;
	int m_screenHeight;
	float m_dpiScaleX;
	float m_dpiScaleY;
	FactionId m_faction;
	bool m_editMode = false;
	std::atomic<bool> m_lockInput = false;
	constexpr static ImGuiWindowFlags m_menuWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground;
	Window();
	void startLoop();
	void render();
	void quit();
	void processEvents();
	void createSimulation(const std::string& name, const DateTime);
	void save(std::function<void()>&& continuation = nullptr);
	void load(const std::filesystem::path& path);
	void showMainMenu();
	void showCreateSimulation();
	void showCreateArea();
	void showLoad();
	void showGame();
	void showEdit();
	void showActorDetails(const ActorReference actor);
	void showObjectivePriorities(const ActorReference actor);
	void showSelectFactionToEdit();
	void showEditFaction(const FactionId faction);
	void showEditUniform(Uniform* uniform);
	void showEditActor(const ActorReference actor);
	void setArea(Area& area);
	void zoom(float zoomDelta);
	void pan(float dx, float dy);
	void clampPOV();
	[[nodiscard]] SDL_FPoint screenToWorld(float sx, float sy) const;
	[[nodiscard]] SDL_FPoint worldToScreen(float wx, float wy) const;
	[[nodiscard]] Area* getArea();
	[[nodiscard]] const Area* getArea() const;
	[[nodiscard]] POV makeDefaultPOV() const;
	[[nodiscard]] Point3D getBlockUnderCursor() const;
	[[nodiscard]] Point3D getBlockAtScreenPosition(const int x, const int y) const;
	[[nodiscard]] Point3D getBlockAtScreenPosition(const SDL_Point point) const;
	[[nodiscard]] Distance invertY(const Distance distance) const;
	[[nodiscard]] static std::chrono::milliseconds msSinceEpoch();
};