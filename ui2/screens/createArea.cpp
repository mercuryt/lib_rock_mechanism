#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../imgui/imgui_stdlib.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"

void screens::createArea(Window& window)
{
	static std::string name;
	static int sizeX = 100;
	static int sizeY = 100;
	static int sizeZ = 100;
	ImVec2 windowSize{400, 400};
	begin(window, "Create Area", &windowSize);
	ImGui::InputText("Name", &name);
	ImGui::PushItemWidth(200.f);
	ImGui::InputInt("X", &sizeX);
	ImGui::InputInt("Y", &sizeY);
	ImGui::InputInt("Z", &sizeZ);
	ImGui::PopItemWidth();
	if(sizeX < 1) sizeX = 1;
	if(sizeY < 1) sizeY = 1;
	if(sizeZ < 1) sizeZ = 1;
	constexpr int maxDistance = Distance::max().get();
	if(sizeX > maxDistance) sizeX = maxDistance;
	if(sizeY > maxDistance) sizeY = maxDistance;
	if(sizeZ > maxDistance) sizeZ = maxDistance;
	if(ImGui::Button("Create") && name[0] != '\0')
	{
		window.m_editMode = true;
		auto result = std::make_shared<Area*>();
		std::function<void()> task = [result, &window]{
			static constexpr bool createDrama = true;
			(*result) = &window.m_simulation->m_hasAreas->createArea(sizeX, sizeY, sizeZ, createDrama);
		};
		window.m_backgroundTask.start(window, task, [result, &window] mutable {
			window.setArea(**result);
			window.m_area->m_name = name;
			window.m_pov = &(window.m_lastViewedSpotInArea.insert(window.m_area->m_id, std::make_unique<POV>(window.makeDefaultPOV())));
			window.showGame();
		});
	}
	ImGui::SameLine();
	if(ImGui::Button("Back"))
		window.showMainMenu();
	end();
}