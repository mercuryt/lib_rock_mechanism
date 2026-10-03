#pragma once
#include "../../engine/geometry/point3D.h"
class Window;
struct MapOverlay
{
	ExpeditionId m_selectedExpedition;
	bool m_gameMenuIsOpen = false;
	void drawArea(Window& window);
	void drawTopBar(Window& window);
	void drawAreaDetailPanel(Window& window);
	void drawMenu(Window& window);
};