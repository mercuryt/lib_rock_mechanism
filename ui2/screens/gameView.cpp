#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/area/area.h"
#include "../../engine/space/space.h"

void screens::gameView(Window& window)
{
	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	SDL_FPoint mousePositionWorld = window.screenToWorld(mouseX, mouseY);
	window.m_blockUnderCursor.setX({(DistanceWidth)(mousePositionWorld.x)});
	window.m_blockUnderCursor.setY({(DistanceWidth)(mousePositionWorld.y)});
	window.m_blockUnderCursor.setZ(window.m_pov->z);
	// Render world at full drawable resolution to m_gameView.
	SDL_SetRenderTarget(window.m_sdlRenderer, window.m_gameView);
	SDL_SetRenderDrawColor(window.m_sdlRenderer, 20,20,20,255);
	SDL_RenderClear(window.m_sdlRenderer);
	int worldWidth = window.m_area->getSpace().m_sizeX.get() * displayData::defaultScale;
	int worldHeight = window.m_area->getSpace().m_sizeY.get() * displayData::defaultScale;
	// Draw border around area.
	window.color(displayData::areaOutlineColor);
	for(int i = 0; i < 3; i++)
	{
		SDL_Rect borderSegment{i, i, worldWidth - 2*i, worldHeight - 2*i};
		SDL_RenderDrawRect(window.m_sdlRenderer, &borderSegment);
	}
	// Set render target to screen.
	SDL_SetRenderTarget(window.m_sdlRenderer, NULL);
	SDL_Rect gameViewRenderDestination{
		(int)(-window.m_pov->x * window.m_pov->zoom),
		(int)(-window.m_pov->y * window.m_pov->zoom),
		(int)(worldWidth * window.m_pov->zoom),
		(int)(worldHeight * window.m_pov->zoom)
	};
	SDL_RenderCopy(window.m_sdlRenderer, window.m_gameView, NULL, &gameViewRenderDestination);
	// Does not render yet.
	window.m_gameOverlay.draw(window);
}