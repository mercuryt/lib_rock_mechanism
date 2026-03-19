#include "window.h"
#include "screens/screens.h"
#include "displayData.h"
#include "../engine/simulation/simulation.h"
#include "../engine/simulation/hasAreas.h"
#include "../engine/area/area.h"
#include "../engine/space/space.h"
#include <stdint.h>
#include <utility>
#include <stdint.h>
#include <utility>
#include <vector>
#include <fstream>
constexpr uint32_t windowFlagsNotFullScreen = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
constexpr uint32_t windowFlags = windowFlagsNotFullScreen | SDL_WINDOW_FULLSCREEN_DESKTOP;
Window::Window() :
	m_simulationThread([&](){
		while(true)
		{
			std::chrono::milliseconds start;
			start = msSinceEpoch();
			if(m_simulation && !m_paused)
				m_simulation->doStep(m_speed);
			std::chrono::milliseconds delta = msSinceEpoch() - start;
			if(delta < m_minimumTimePerStep)
				/* When compiling for webassebly the follwing is required for sleeping background threads:
				 The compiler must be passed the flags "-s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1 -s PTHREAD_POOL_SIZE='navigator.hardwareConcurrency'"
				 The page must be served with the HTTP headers:
					Cross-Origin-Opener-Policy: same-origin
					Cross-Origin-Embedder-Policy: require-corp
				*/
				std::this_thread::sleep_for(m_minimumTimePerStep - delta);
		}
	}),
	m_window(SDL_CreateWindow("Goblin Pit", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 800, windowFlags)),
	m_sdlRenderer(SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)),
	m_minimumTimePerStep((int)(1000.f / (float)Config::stepsPerSecond.get()))
{
	// For high DPI systems.
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // smooth scaling
	SDL_SetRenderDrawBlendMode(m_sdlRenderer, SDL_BLENDMODE_BLEND);
	 // Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	 // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;	// Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(m_mainScale);		// Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = m_mainScale;		// Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
	// Setup ImGui Platform/Renderer backend
	ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_sdlRenderer);
	ImGui_ImplSDLRenderer2_Init(m_sdlRenderer);
	// Get drawable size in real pixels.
	SDL_GetWindowSize(m_window, &m_screenWidth, &m_screenHeight);
	int drawableW, drawableH;
	SDL_GetRendererOutputSize(m_sdlRenderer, &drawableW, &drawableH);
	m_dpiScaleX = (float)drawableW / (float)m_screenWidth;
	m_dpiScaleY = (float)drawableH / (float)m_screenHeight;
	updateSimulationList();
	m_gameOverlay.m_controllsState.initalize();
}
void Window::startLoop()
{
	#ifdef __EMSCRIPTEN__
		// For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
		// You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
		io.IniFilename = nullptr;
		EMSCRIPTEN_MAINLOOP_BEGIN
	#else
		while (m_running)
	#endif
		{
			if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED)
			{
				SDL_Delay(10);
				continue;
			}
			m_backgroundTask.update(*this);
			processEvents();
			// Delta time.
			static Uint64 last = SDL_GetPerformanceCounter();
			Uint64 now = SDL_GetPerformanceCounter();
			float deltaTime = static_cast<float>(
				static_cast<double>(now - last) / SDL_GetPerformanceFrequency());
			last = now;
			ImGuiIO& io = ImGui::GetIO();
			io.DeltaTime = deltaTime;
			render();
		}
	#ifdef __EMSCRIPTEN__
		EMSCRIPTEN_MAINLOOP_END;
	#endif
	if (m_sdlRenderer) SDL_DestroyRenderer(m_sdlRenderer);
	if (m_window) SDL_DestroyWindow(m_window);
	SDL_Quit();
}
void Window::render()
{
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGuiIO& io = ImGui::GetIO();
	// Get SDL logical renderer size.
	int logicalW = 0, logicalH = 0;
	SDL_RenderGetLogicalSize(m_sdlRenderer, &logicalW, &logicalH);
	// If logical size not set, fallback to actual renderer output size
	if (logicalW == 0 || logicalH == 0)
		SDL_GetRendererOutputSize(m_sdlRenderer, &logicalW, &logicalH);
	// Make ImGui work with high dpi
	int winW, winH;
	int drawableW, drawableH;
	SDL_GetWindowSize(m_window, &winW, &winH);
	SDL_GetRendererOutputSize(m_sdlRenderer, &drawableW, &drawableH);
	io.DisplaySize = ImVec2((float)winW, (float)winH);
	io.DisplayFramebufferScale = ImVec2( (float)drawableW / winW, (float)drawableH / winH);
	// Get mouse position and scale to logical size.
	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	float scaleX = (float)logicalW / winW;
	float scaleY = (float)logicalH / winH;
	io.MousePos = ImVec2(mouseX * scaleX, mouseY * scaleY);
	// Record logical position.
	m_mousePosition = {(int)io.MousePos.x, (int)io.MousePos.y};
	ImGui::NewFrame();
	SDL_SetRenderDrawColor(m_sdlRenderer, 0,0,0,255);
	SDL_RenderClear(m_sdlRenderer);
	// Render current panel.
	switch (m_panel)
	{
		// Normal mode.
		case PanelId::MainMenu:				screens::mainMenu(*this); break;
		case PanelId::Load:					screens::load(*this); break;
		case PanelId::GameView:				screens::gameView(*this); break;
		case PanelId::ActorDetails:			screens::actorDetails(*this, m_gameOverlay.m_detailActor); break;
		case PanelId::ObjectivePriorities:	screens::objectivePriorities(*this, m_gameOverlay.m_detailActor); break;
		case PanelId::EditUniform:			screens::editUniform(*this, m_gameOverlay.m_uniformToEdit); break;
		case PanelId::Production:			screens::production(*this); break;
		// Edit mode.
		case PanelId::CreateSimulation:		screens::createSimulation(*this); break;
		case PanelId::CreateArea:			screens::createArea(*this); break;
		case PanelId::Edit:					screens::editSimulation(*this); break;
		case PanelId::SelectFactionToEdit:	screens::selectFactionToEdit(*this); break;
		case PanelId::EditFaction:			screens::editFaction(*this, m_gameOverlay.m_factionToEdit); break;
		case PanelId::EditActor:			screens::editActor(*this, m_gameOverlay.m_detailActor); break;
		case PanelId::EditDrama:			screens::editDrama(*this, *m_area); break;
		default: std::unreachable();
	}
	// Render ImGui.
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_sdlRenderer);
	SDL_RenderPresent(m_sdlRenderer);
}
void Window::quit()
{
	#ifdef __EMSCRIPTEN__
		emscripten_cancel_main_loop()
	#endif
		m_running = false;
}
void Window::processEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if(event.type == SDL_QUIT)
			SDL_Quit();
		if(m_lockInput)
			return;
		ImGui_ImplSDL2_ProcessEvent(&event);
		ImGui::SetMouseCursor(m_cursor);
		// All remaining inputs only apply to game view.
		if(m_panel != PanelId::GameView)
			continue;
		if(
			(
				event.type == SDL_MOUSEBUTTONDOWN ||
				event.type == SDL_MOUSEBUTTONUP ||
				event.type == SDL_MOUSEMOTION ||
				event.type == SDL_MOUSEWHEEL
			) &&
			ImGui::GetIO().WantCaptureMouse
		)
				continue;
		if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
		{
			if(ImGui::GetIO().WantCaptureKeyboard)
				continue;
			m_shiftKey = event.key.keysym.mod & KMOD_SHIFT;
			m_controllKey = event.key.keysym.mod & KMOD_CTRL;
		}
		switch(event.type)
		{
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						if(m_gameOverlay.m_infoPopUp != InfoPopUpId::Null)
							m_gameOverlay.m_infoPopUp = InfoPopUpId::Null;
						else if(std::ranges::find(overlayPanels, m_panel) != std::end(overlayPanels))
							m_panel = PanelId::GameView;
						else
						{
							// Opening the menu pauses the game.
							m_paused = true;
							m_gameOverlay.m_gameMenuIsOpen = !m_gameOverlay.m_gameMenuIsOpen;
						}
						break;
					/*
					case 'f':
						m_fullScreen = !m_fullScreen;
						if (m_fullScreen)
							SDL_SetWindowFullscreen(m_window, windowFlags);
						else
							SDL_SetWindowFullscreen(m_window, windowFlagsNotFullScreen);
						break;
					*/
					case ' ':
						// Don't allow user to unpause while menu is open.
						if(!m_gameOverlay.m_gameMenuIsOpen)
							m_paused.toggle();
						break;
					case '.':
						m_simulation->doStep(1);
						break;
					case 'z':
						if(m_gameOverlay.m_selectMode == SelectMode::Plants)
							m_gameOverlay.m_selectMode = SelectMode::Space;
						else
							m_gameOverlay.m_selectMode = (SelectMode)(((int)m_gameOverlay.m_selectMode + 1) % 4);
						m_gameOverlay.deselectAll();
						break;
					case SDLK_INSERT:
						zoom(1.f + displayData::zoomIncrement);
						break;
					case SDLK_DELETE:
						zoom(1.f - displayData::zoomIncrement);
						break;
					case SDLK_UP:
						pan(0.f, -displayData::panSpeed / m_pov->zoom);
						break;
					case SDLK_DOWN:
						pan(0.f, displayData::panSpeed / m_pov->zoom);
						break;
					case SDLK_LEFT:
						pan(-displayData::panSpeed / m_pov->zoom, 0);
						break;
					case SDLK_RIGHT:
						pan(displayData::panSpeed / m_pov->zoom, 0);
						break;
					case SDLK_PAGEUP:
						if(m_pov->z != m_area->getSpace().m_sizeZ - 1)
							++m_pov->z;
						break;
					case SDLK_PAGEDOWN:
						if(m_pov->z != 0)
							--m_pov->z;
						break;

				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				// Right button is handled by ImGui, maybe left should be as well.
				if(event.button.button == SDL_BUTTON_LEFT)
				{
					m_gameOverlay.m_mouseIsDown = true;
					m_gameOverlay.m_mouseDragStartCoordinates = m_mousePosition;
					m_gameOverlay.m_mouseDragStartPoint = getBlockAtScreenPosition(m_mousePosition);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if(event.button.button == SDL_BUTTON_LEFT)
				{
					m_gameOverlay.m_mouseIsDown = false;
					Cuboid cuboid = Cuboid::create(getBlockAtScreenPosition(m_mousePosition), m_gameOverlay.m_mouseDragStartPoint);
					m_gameOverlay.updateSelect(*this, cuboid);
				}
				break;
			case SDL_QUIT:
				quit();
		}
	}
}
void Window::showMainMenu()
{
	m_panel = PanelId::MainMenu;
}
void Window::showCreateSimulation()
{
	m_panel = PanelId::CreateSimulation;
}
void Window::showCreateArea()
{
	m_panel = PanelId::CreateArea;
}
void Window::showLoad()
{
	m_panel = PanelId::Load;
}
void Window::showGame()
{
	m_panel = PanelId::GameView;
}
void Window::showEdit()
{
	m_panel = PanelId::Edit;
}
void Window::showActorDetails(const ActorReference actor)
{
	m_gameOverlay.m_detailActor = actor;
	m_panel = PanelId::ActorDetails;
}
void Window::showObjectivePriorities(const ActorReference actor)
{
	m_gameOverlay.m_detailActor = actor;
	m_panel = PanelId::ObjectivePriorities;
}
void Window::showSelectFactionToEdit()
{
	m_panel = PanelId::SelectFactionToEdit;
}
void Window::showEditFaction(const FactionId faction)
{
	m_gameOverlay.m_factionToEdit = faction;
	m_panel = PanelId::EditFaction;
}
void Window::showEditUniform(Uniform* uniform)
{
	m_gameOverlay.m_uniformToEdit = uniform;
	m_panel = PanelId::EditUniform;
}
void Window::showEditActor(const ActorReference actor)
{
	m_gameOverlay.m_detailActor = actor;
	m_panel = PanelId::EditActor;
}
void Window::createSimulation(const std::string& name, const DateTime dateTime)
{
	m_simulation = std::make_unique<Simulation>(name, dateTime);
	m_simulation->save();
	updateSimulationList();
}
void Window::updateSimulationList()
{
	std::filesystem::path save{"save"};
	m_simulationList.clear();
	for(const auto &entry : std::filesystem::directory_iterator(save))
	{
		std::ifstream simulationFile(entry.path()/"simulation.json");
		const Json& data = Json::parse(simulationFile);
		std::string name = data["name"].get<std::string>();
		m_simulationList.emplace_back(name, entry.path());
	}
}
void Window::load(const std::filesystem::path& path)
{
	m_gameOverlay.deselectAll();
	auto result = std::make_shared<LoadResult>();
	std::function<void()> task = [path, result] mutable {
		result->simulation = std::make_unique<Simulation>(path);
		std::filesystem::path viewPath = result->simulation->getPath()/"view.json";
		if(std::filesystem::exists(viewPath))
		{
			std::ifstream af(viewPath);
			Json viewData = Json::parse(af);
			result->area = viewData["area"].get<AreaId>();
			if(viewData.contains("faction"))
				result->faction = viewData["faction"].get<FactionId>();
		}
	};
	std::function<void()> callback = [this, result] mutable
	{
		m_paused = true;
		m_speed = 1.0f;
		m_simulation = std::move(result->simulation);
		m_faction = result->faction;
		if(result->area.exists())
		{
			setArea(m_simulation->m_hasAreas->getById(result->area));
			if(!m_lastViewedSpotInArea.contains(result->area))
				m_lastViewedSpotInArea.insert(result->area, std::make_unique<POV>(makeDefaultPOV()));
			m_pov = &m_lastViewedSpotInArea[result->area];
		}
		if(m_editMode)
			showEdit();
		else
		{
			assert(m_pov != nullptr);
			showGame();
		}
	};
	m_backgroundTask.start(*this, task, callback);
}
void Window::save(std::function<void()>&& continuation)
{
	std::cout << "begin save" << std::endl;
	// No need to lock mutex here, as long as the game is paused this is safe.
	assert(m_paused == true);
	assert(m_simulation != nullptr);
	std::function<void()> task = [this]{
		m_simulation->save();
		if(m_area != nullptr)
		{
			Json viewData{{"area", m_area->m_id}};
			if(m_faction.exists())
				viewData["faction"] = m_faction;
			std::ofstream file(m_simulation->getPath()/"view.json" );
			file << viewData;
			file.flush();
		}
	};
	m_backgroundTask.start(*this, std::move(task), std::move(continuation));
}
void Window::setArea(Area& area)
{
	m_area = &area;
	SDL_DestroyTexture(m_gameView);
	const Space& space = m_area->getSpace();
	int width = space.m_sizeX.get() * displayData::defaultScale;
	int height = space.m_sizeY.get() * displayData::defaultScale;
	m_gameView = SDL_CreateTexture(m_sdlRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
}
Area* Window::getArea() { return m_area; }
const Area* Window::getArea() const { return m_area; }
POV Window::makeDefaultPOV() const
{
	Point3D surfaceCenter = m_area->getSpace().getCenterAtGroundLevel();
	return {
		surfaceCenter.x().get() * displayData::defaultScale,
		surfaceCenter.y().get() * displayData::defaultScale,
		surfaceCenter.z(),
		1.f
	};
}
void Window::zoom(float zoom)
{
	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	SDL_FPoint worldBefore = screenToWorld(mouseX, mouseY);
	m_pov->zoom *= zoom;
	m_pov->zoom = std::clamp(m_pov->zoom, 0.1f, 20.0f);
	SDL_FPoint worldAfter = screenToWorld(mouseX, mouseY);
	m_pov->x += (worldBefore.x - worldAfter.x);
	m_pov->y += (worldBefore.y - worldAfter.y);
	clampPOV();
}
void Window::pan(float dx, float dy)
{
	m_pov->x += dx;
	m_pov->y += dy;
	clampPOV();
}
void Window::clampPOV()
{
	int w, h;
	SDL_GetRendererOutputSize(m_sdlRenderer, &w, &h);
	float viewW = w / m_pov->zoom;
	float viewH = h / m_pov->zoom;
	const Space& space = m_area->getSpace();
	float worldW = space.m_sizeX.get() * displayData::defaultScale;
	float worldH = space.m_sizeY.get() * displayData::defaultScale;
	float maxX = worldW - viewW + 100.f;
	float maxY = worldH - viewH + 100.f;
	m_pov->x = std::clamp((float)m_pov->x, -100.f, maxX);
	m_pov->y = std::clamp((float)m_pov->y, -100.f, maxY);
}
SDL_FPoint Window::screenToWorld(float sx, float sy) const
{
	assert(m_area != nullptr);
	Space& space = m_area->getSpace();
	SDL_FPoint p;
	p.x = (sx / m_pov->zoom) + m_pov->x;
	p.y = (sy / m_pov->zoom) + m_pov->y;
	p.x = std::clamp(p.x, 0.f, (float)(space.m_sizeX.get() - 1)* displayData::defaultScale);
	p.y = std::clamp(p.y, 0.f, (float)(space.m_sizeY.get() - 1) * displayData::defaultScale);
	return p;
}
SDL_FPoint Window::worldToScreen(float wx, float wy) const
{
	SDL_FPoint p;
	p.x = (wx - m_pov->x) * m_pov->zoom;
	p.y = (wy - m_pov->y) * m_pov->zoom;
	return p;
}
Point3D Window::getBlockUnderCursor() const
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return getBlockAtScreenPosition(x, y);
}
Point3D Window::getBlockAtScreenPosition(const int x, const int y) const
{
	SDL_FPoint worldPosition = screenToWorld(x, y);
	return {
		Distance::create(worldPosition.x / displayData::defaultScale),
		invertY(Distance::create(worldPosition.y / displayData::defaultScale)),
		m_pov->z
	};
}
Point3D Window::getBlockAtScreenPosition(const SDL_Point point) const
{
	return getBlockAtScreenPosition(point.x, point.y);
}
Distance Window::invertY(const Distance distance) const
{
	assert(m_area != nullptr);
	return (m_area->getSpace().m_sizeY - 1) - distance;
}
std::chrono::milliseconds Window::msSinceEpoch()
{
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}