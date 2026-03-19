
#include "screens.h"
#include "../window.h"
#include "../../engine/area/area.h"
#include "../../engine/drama/engine.h"

void screens::editDrama(Window& window, Area& area)
{
	assert(window.m_editMode);
	window.m_paused = true;
	begin(window, "Edit Drama");
	DramaEngine* dramaEngine = window.m_simulation->m_dramaEngine.get();
	std::vector<DramaArc*> arcsForArea = dramaEngine->getArcsForArea(area);
	SmallSet<DramaArcType> typesForArea;
	ImGui::BeginTable("arcs", 3);
	for(DramaArc* arc : arcsForArea)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGuiText(arc->name());
		ImGui::TableNextColumn();
		if(ImGui::Button("start"))
		{
			arc->trigger();
			window.showGame();
		}
		ImGui::TableNextColumn();
		if(ImGui::Button("remove"))
			dramaEngine->remove(*arc);
		typesForArea.insert(arc->m_type);
	}
	ImGui::EndTable();
	if(ImGui::BeginCombo("new arc type", "new arc type"))
	{
		for(const DramaArcType type : DramaArc::getTypes())
		{
			if(!typesForArea.contains(type))
				if(ImGuiSelectable(DramaArc::typeToString(type), false))
					window.m_simulation->m_dramaEngine->createArcTypeForArea(type, area);
		}
		ImGui::EndCombo();
	}
	if(ImGui::Button("close"))
		window.showGame();
	end();
}