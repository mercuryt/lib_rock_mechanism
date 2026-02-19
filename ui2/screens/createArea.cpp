#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"

void screens::createArea(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	static char name[128];
	static int sizeX = 100;
	static int sizeY = 100;
	static int sizeZ = 100;
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	ImGui::SetNextWindowSize({400,400});
	ImGui::Begin("createArea", &canClose, window.m_menuWindowFlags);
	ImGui::Text("Create Area");
	ImGui::InputText("Name", name, IM_ARRAYSIZE(name));
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
		window.m_backgroundTask.create(std::move(task), [result, &window] mutable {
			window.setArea(**result);
			window.m_area->m_name = name;
			window.m_pov = &(window.m_lastViewedSpotInArea.insert(window.m_area->m_id, std::make_unique<POV>(window.makeDefaultPOV())));
			window.showGame();
		});
	}
	ImGui::SameLine();
	if(ImGui::Button("Back"))
		window.showMainMenu();
	ImGui::PopFont();
	ImGui::End();
}