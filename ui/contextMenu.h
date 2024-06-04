#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class BlockIndex;
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
	void add(tgui::Widget::Ptr widget, std::string id = "");
	~ContextMenuSegment();
};
class ContextMenu final
{
	Window& m_window;
	ContextMenuSegment m_root;
	tgui::Group::Ptr m_gameOverlayGroup;
	std::vector<ContextMenuSegment> m_submenus;
public:
	ContextMenu(Window& window, tgui::Group::Ptr gameOverlayGroup);
	void draw(BlockIndex& block);
	void drawDigControls(BlockIndex& block);
	void drawConstructControls(BlockIndex& block);
	void construct(BlockIndex& block, bool constructed, const MaterialType& materialType, const BlockFeatureType* blockFeatureType);
	void drawActorControls(BlockIndex& block);
	void drawPlantControls(BlockIndex& block);
	void drawItemControls(BlockIndex& block);
	void drawFluidControls(BlockIndex& block);
	void drawFarmFieldControls(BlockIndex& block);
	void drawStockPileControls(BlockIndex& block);
	void drawCraftControls(BlockIndex& block);
	void drawWoodCuttingControls(BlockIndex& block);
	void hide();
	ContextMenuSegment& makeSubmenu(size_t index);
	[[nodiscard]] bool isVisible() const { return m_root.m_panel->isVisible(); }
	[[nodiscard]] float getOriginYForMousePosition() const;
};
