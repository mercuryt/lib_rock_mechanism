#include "screens.h"
#include "../window.h"
#include "../../engine/uniform.h"
#include "../../engine/simulation/simulation.h"

void screens::selectUniformToEdit(Window& window)
{
	begin(window, "Select Uniform To Edit");
	for(auto& pair : window.m_simulation->m_hasUniforms.getForFaction(window.m_faction).getAll())
	{
		std::string name = pair.first;
		Uniform& uniform = pair.second;
		if(ImGuiButton(name))
			window.showEditUniform(&uniform);
	}
	if(ImGui::Button("back"))
		window.showGame();
	end();
}