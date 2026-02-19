#include "gameOverlay.h"
#include "window.h"
#include "displayData.h"
void GameOverlay::draw(Window& window)
{
	drawSelectionBox(window);
	drawTopBar(window);
	if(m_gameMenuIsOpen)
		drawMenu(window);
}
void GameOverlay::drawSelectionBox(Window& window)
{
	if(!m_mouseIsDown)
		return;
	SDL_Rect rect;
	rect.x = std::min(m_mouseDragStart.x, window.m_mousePosition.x);
	rect.y = std::min(m_mouseDragStart.y, window.m_mousePosition.y);
	rect.w = std::abs(window.m_mousePosition.x - m_mouseDragStart.x);
	rect.h = std::abs(window.m_mousePosition.y - m_mouseDragStart.y);
	if(rect.w != 0 && rect.h != 0)
	{
		window.color(displayData::selectColor);
		SDL_RenderDrawRect(window.m_sdlRenderer, &rect);
	}
}
void GameOverlay::drawTopBar(Window& window)
{
	float height = ImGui::CalcTextSize("X").y;
	ImGuiIO& io = ImGui::GetIO();
	float windowWidth = io.DisplaySize.x;
	ImGui::SetNextWindowPos(ImVec2(0,0));
	ImGui::SetNextWindowSize(ImVec2(windowWidth, height));
	ImGui::Begin("topBar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs
	);
	const DateTime& now = window.m_simulation->getDateTime();
	// TODO: getting seconds and minutes from hourlyEvent.percentComplete is roundabout and wierd, add them to DateTime instead?
	const float fractionOfCurrentHourElapsed = window.m_simulation->m_hourlyEvent.fractionComplete();
	int seconds = Config::minutesPerHour * Config::secondsPerMinute * fractionOfCurrentHourElapsed;
	const int minutes = seconds / Config::secondsPerMinute;
	seconds -= minutes * Config::secondsPerMinute;
	ImGui::Text(
		"seconds: %i minutes: %i hour: %i day: %i year: %i",
		seconds, minutes, now.hour, now.day, now.year
	);
	ImGui::SameLine();
	const float yPos = ImGui::GetItemRectMin().y;
	ImGui::SetCursorScreenPos({windowWidth / 3.f, yPos});
	ImGui::Text(window.m_paused ? "paused" : "speed: %.2f", window.m_speed.load());
	ImGui::SameLine();
	ImGui::SetCursorScreenPos({windowWidth * 6 / 10.f, yPos});
	ImGui::Text("%i, %i, %i", window.m_blockUnderCursor.x().get(), window.m_blockUnderCursor.y().get(), window.m_blockUnderCursor.z().get());
	ImGui::SameLine();
	// TODO: weather.
	ImGui::End();
}
void GameOverlay::drawMenu(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	// --- Set window size and position in logical space ---
	ImVec2 windowSize = ImVec2(400, 400);
	ImVec2 centerPos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
	// Center window using pivot (0.5,0.5)
	ImGui::SetNextWindowSize(windowSize);
	ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::Begin("overlayMenu", &canClose, window.m_menuWindowFlags);
	auto centerButton = [&](const char* label, auto callback)
	{
			float buttonWidth = ImGui::CalcTextSize(label).x + 20; // add padding
			ImGui::SetCursorPosX((windowSize.x - buttonWidth) * 0.5f);
			if(ImGui::Button(label, ImVec2(buttonWidth, 0)))
					callback();
	};
	centerButton("Menu", [&]{ window.showMainMenu(); });
	centerButton("Save", [&]{ window.save(); });
	centerButton("Save And Quit", [&]{ window.save(); window.quit();});
	centerButton("Close", [&]{ m_gameMenuIsOpen = false; });
	ImGui::PopFont();
	ImGui::End();
}