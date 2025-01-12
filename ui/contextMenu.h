#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class BlockIndex;
class PlantIndex;
class MaterialTypeId;
struct MaterialType;
struct BlockFeatureType;
struct ContextMenuSegment final
{
	int m_count = 1;
	tgui::Group::Ptr m_gameOverlayGroup;
	static constexpr int width = 120;
	static constexpr int height = 20;
public:
	ContextMenuSegment(tgui::Group::Ptr overlayGroup);
	tgui::Panel::Ptr m_panel;
	tgui::Grid::Ptr m_grid;
	void add(tgui::Widget::Ptr widget, std::wstring id = L"");
	~ContextMenuSegment();
};
class ContextMenu final
{
	Window& m_window;
	ContextMenuSegment m_root;
	tgui::Group::Ptr m_gameOverlayGroup;
	std::vector<ContextMenuSegment> m_submenus;
	[[nodiscard]] bool plantIsValid(const PlantIndex& plant) const;
public:
	ContextMenu(Window& window, tgui::Group::Ptr gameOverlayGroup);
	void draw(const BlockIndex& block);
	void drawDigControls(const BlockIndex& block);
	void drawConstructControls(const BlockIndex& block);
	void construct(const BlockIndex& block, bool constructed, const MaterialTypeId& materialType, const BlockFeatureType* blockFeatureType);
	void drawActorControls(const BlockIndex& block);
	void drawPlantControls(const BlockIndex& block);
	void drawItemControls(const BlockIndex& block);
	void drawFluidControls(const BlockIndex& block);
	void drawFarmFieldControls(const BlockIndex& block);
	void drawStockPileControls(const BlockIndex& block);
	void drawCraftControls(const BlockIndex& block);
	void drawWoodCuttingControls(const BlockIndex& block);
	void hide();
	ContextMenuSegment& makeSubmenu(size_t index);
	[[nodiscard]] bool isVisible() const { return m_root.m_panel->isVisible(); }
	[[nodiscard]] float getOriginYForMousePosition() const;
};
