#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class Point3D;
class PlantIndex;
class MaterialTypeId;
struct MaterialType;
struct PointFeatureType;
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
	void draw(const Point3D& point);
	void drawDigControls(const Point3D& point);
	void drawConstructControls(const Point3D& point);
	void construct(const Point3D& point, bool constructed, const MaterialTypeId& materialType, const PointFeatureType* pointFeatureType);
	void drawActorControls(const Point3D& point);
	void drawPlantControls(const Point3D& point);
	void drawItemControls(const Point3D& point);
	void drawFluidControls(const Point3D& point);
	void drawFarmFieldControls(const Point3D& point);
	void drawStockPileControls(const Point3D& point);
	void drawCraftControls(const Point3D& point);
	void drawWoodCuttingControls(const Point3D& point);
	void hide();
	ContextMenuSegment& makeSubmenu(size_t index);
	[[nodiscard]] bool isVisible() const { return m_root.m_panel->isVisible(); }
	[[nodiscard]] float getOriginYForMousePosition() const;
};
