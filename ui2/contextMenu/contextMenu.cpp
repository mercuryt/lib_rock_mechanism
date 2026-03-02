#include "contextMenu.h"
#include "../window.h"
constexpr static ImGuiWindowFlags contextMenuWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration;
void contextMenu::draw(Window& window)
{
	int x = window.m_mousePostition.x;
	int y = window.m_mousePosition.p;
	ImGui::SetCursorPos(x, y);
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	ImGui::Begin("contextMenu", &canClose, contextMenuWindowFlags);
	ContextMenuState& state = window.m_gameOverly.m_contextMenuState;
	switch(state.current)
	{
		case ContextMenuId::Root:
			root(window);
			break;
		case ContextMenuId::Dig:
			submenu::dig(window);
			break;
		case ContextMenuId::Construct:
			submenu::construct(window);
			break;
		case ContextMenuId::Null:
			std::unreachable();
			break;
	}
	if(state.current != ContextMenuId::Root)
		if(ImGui::Button("back"))
			state.current = ContextMenuId::Root;
	ImGui::End();
}
void contextMenu::root(Window& window)
{
	ContextMenuState& state = window.m_gameOverly.m_contextMenuState;
	const Point3D point = window.m_gameOverlay.m_blockUnderCursor;
	const Space& space = window.m_area->getSpace();
	const MaterialTypeId solid = space.solid_get(point);
	const FeatureTypeId feature = space.pointFeature_get(point);
	if(solid.exists() || feature.exists())
		controlls::dig(window);
}