#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../draw/area.h"
#include "../draw/map.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/area/area.h"
#include "../../engine/space/space.h"

void screens::gameView(Window& window)
{
	window.m_blockUnderCursor = window.getBlockUnderCursor();
	if(!window.m_backgroundTask.running())
	{
		// Render world to m_gameView.
		SDL_SetRenderTarget(window.m_sdlRenderer, window.m_gameView);
		SDL_SetRenderDrawColor(window.m_sdlRenderer, 20,20,20,255);
		SDL_RenderClear(window.m_sdlRenderer);
		// Lock simulation mutex for vertex generation.
		// TODO: Sprite buffer.
		{
			std::lock_guard lock(window.m_simulation->m_uiReadMutex);
			if(window.m_mapOpen)
				drawMap::main(window);
			else
				drawArea::main(window);
		}
		// Flush buffer.
		window.m_renderBuffer.render(window.m_sdlRenderer);
		// Set render target to screen.
		SDL_SetRenderTarget(window.m_sdlRenderer, NULL);
		int worldWidth = window.m_currentDisplayTargetTotalTilesX.get() * displayData::defaultScale;
		int worldHeight = window.m_currentDisplayTargetTotalTilesY.get() * displayData::defaultScale;
		SDL_Rect gameViewRenderDestination{
			(int)(-window.m_pov->x * window.m_pov->zoom),
			(int)(-window.m_pov->y * window.m_pov->zoom),
			(int)(worldWidth * window.m_pov->zoom),
			(int)(worldHeight * window.m_pov->zoom)
		};
		SDL_RenderCopy(window.m_sdlRenderer, window.m_gameView, NULL, &gameViewRenderDestination);
	}
	// Does not render yet.
	if(window.m_mapOpen)
		window.m_mapOverlay.drawArea(window);
	else
		window.m_areaOverlay.drawArea(window);
}