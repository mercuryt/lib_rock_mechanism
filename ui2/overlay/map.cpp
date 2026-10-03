#include "map.h"
#include "../../engine/world/world.h"
#include "../../engine/world/climate.h"
#include "../../engine/fluidType.h"
#include "../window.h"
#include "../displayData.h"
 void mapOverlay::drawArea(Window& window)
 {
	World& world = *window.m_simulation->m_world;

 }
 // This is mostly the same as the areaOverlay version.
void mapOverlay::drawTopBar(Window& window)
{
	ImGuiIO& io = ImGui::GetIO();
	float windowWidth = io.DisplaySize.x;
	ImGui::SetNextWindowPos(ImVec2(0,0));
	ImGui::SetNextWindowSize(ImVec2(windowWidth, 0.f));
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
	Point3D location = window.getBlockUnderCursor();
	ImGui::Text("%i, %i, %i", location.x().get(), location.y().get(), location.z().get());
	ImGui::End();
}
void mapOverlay::drawAreaDetailPanel(Window& window)
{
	ImGuiIO& io = ImGui::GetIO();
	float windowWidth = io.DisplaySize.x;
	ImGui::Begin("areaDetailPanel", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs
	);
	World& world = *window.m_simulation->m_world;
	Point3D location = window.getBlockUnderCursor();
	MaterialTypeId solid = world.m_solid.queryGetOne(location);
	if(solid.exists())
		ImGui::Text("%s", MaterialType::getName(solid));
	else
	{
		FluidTypeId fluid = world.m_fluid.queryGetOne(location);
		if(fluid.exists())
		{
			ImGui::Text("%s", FluidType::getName(fluid));
			if(world.m_ocean.query(location))
			{
				ImGui::SameLine();
				ImGui::Text(" ocean");
			}
		}
	}
	// If there is open space above the area block then the top part is open space.
	if(location.z() != world.m_boundry.m_high.z() && !world.m_solid.queryAny(location.above()))
	{
		Quantity treeCount = world.m_trees.queryGetOne(location);
		if(treeCount.exists())
			ImGui::Text("Trees: %i", treeCount);
		if(world.m_smallLakes.query(location))
			ImGui::Text("Lake(s)");
		if(world.m_smallRivers.queryAny(location))
			ImGui::Text("River(s)");
		if(world.m_roads.queryAny(location))
			ImGui::Text("Roads(s)");
		// TODO: Settlements.
	}
	auto [winter, spring, summer, fall] = climate::humidityBySeason(world, location);
	ImGui::Text("Humidity winter: %i, spring: %i, summer: %i, fall: %i", winter.get(), spring.get(), summer.get(), fall.get());
	auto [minTemp, maxTemp] = climate::temperatureMinAndMax(world, location);
	ImGui::Text("Temperature min:%i, max:%i", minTemp.get(), maxTemp.get());
	ImGui::End();
}
// This is identical to area overlay drawMenu.
void mapOverlay::drawMenu(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	// Set window size and position in logical space.
	ImVec2 windowSize = ImVec2(400, 400);
	ImVec2 centerPos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
	// Center window using pivot (0.5,0.5)
	ImGui::SetNextWindowSize(windowSize);
	ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::Begin("overlayMenu", &canClose, window.m_menuWindowFlags);
	auto centerButton = [&](const char label[], auto callback)
	{
			float buttonWidth = ImGui::CalcTextSize(label).x + 20; // add padding
			ImGui::SetCursorPosX((windowSize.x - buttonWidth) * 0.5f);
			if(ImGui::Button(label, ImVec2(buttonWidth, 0)))
					callback();
	};
	centerButton("Menu", [&]{ m_gameMenuIsOpen = false; window.showMainMenu(); });
	ImGui::BeginDisabled(window.m_backgroundTask.running());
	centerButton("Save", [&]{ window.save(); });
	auto quitContinuation = [&]{ window.save([windowPtr = &window]{ windowPtr->quit(); }); };
	centerButton("Save And Quit", std::move(quitContinuation));
	centerButton("Close", [&]{ m_gameMenuIsOpen = false; });
	ImGui::EndDisabled();
	ImGui::PopFont();
	ImGui::End();
}