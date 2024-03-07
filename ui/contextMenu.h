#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class Block;
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
	void draw(Block& block);
	void drawDigControls(Block& block);
	void drawConstructControls(Block& block);
	void construct(Block& block, bool constructed, const MaterialType& materialType, const BlockFeatureType* blockFeatureType);
	void drawActorControls(Block& block);
	void drawPlantControls(Block& block);
	void drawItemControls(Block& block);
	void drawFluidControls(Block& block);
	void drawFarmFieldControls(Block& block);
	void drawStockPileControls(Block& block);
	void drawCraftControls(Block& block);
	void hide();
	ContextMenuSegment& makeSubmenu(size_t index);
	[[nodiscard]] bool isVisible() const { return m_root.m_panel->isVisible(); }
	[[nodiscard]] float getOriginYForMousePosition() const;
};
