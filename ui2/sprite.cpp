#include "sprite.h"
#include "displayData.h"
#include "window.h"
#include "../img/spright.h"
#include <SDL2/SDL_image.h>

void Sprite::load(Window& window)
{
	spriteCoordinates::load();
	// Initialize PNG and JPEG support
	int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
			SDL_Log("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
	}
	const std::filesystem::path path = "img/build/sheet0.png";
	sheet = IMG_LoadTexture(window.m_sdlRenderer, path.c_str());
	if (!sheet)
		SDL_Log("Failed to load texture: %s\n", IMG_GetError());
	SDL_SetTextureBlendMode(sheet, SDL_BLENDMODE_BLEND);
	SDL_SetTextureScaleMode(sheet, SDL_ScaleModeNearest);
	int h, w;
	SDL_QueryTexture(sheet, nullptr, nullptr, &h, &w);
	sheetWidth = w;
	sheetHeight = h;
}

Sprite::Sprite(const std::string& name)
{
	auto coordinates = spriteCoordinates::data[name];
	source.x = coordinates.x;
	source.y = coordinates.y;
	source.w = coordinates.w;
	source.h = coordinates.h;
	u0 = (float)source.x / sheetWidth;
	v0 = (float)source.y / sheetHeight;
	u1 = (float)(source.x + source.w) / sheetWidth;
	v1 = (float)(source.y + source.h) / sheetHeight;
}
void Sprite::draw(Window& window, const SDL_Rect& destination) const
{
	window.m_renderBuffer.add(*this, destination);
}
void Sprite::draw(Window& window, const Point3D point) const
{
	const auto& scale = displayData::defaultScale;
	SDL_Rect destination{point.x().get() * scale, point.y().get() * scale, source.w, source.h};
	draw(window, destination);
}
void Sprite::drawRotated90CWAndTinted(Window& window, const Point3D point, const SDL_Color color) const
{
	const auto& scale = displayData::defaultScale;
	SDL_Rect destination{point.x().get() * scale, point.y().get() * scale, source.w, source.h};
	window.m_renderBuffer.addRotated90CW(*this, destination, color);
}
void Sprite::drawRotatedAndTinted(Window& window, const Point3D point, const double angle, const SDL_Color color) const
{
	const auto& scale = displayData::defaultScale;
	SDL_Rect destination{point.x().get() * scale, point.y().get() * scale, source.w, source.h};
	window.m_renderBuffer.addRotated(*this, destination, angle, color);
}
void Sprite::drawFacingAndTinted(Window& window, const Point3D point, const Facing4 facing, const SDL_Color color) const
{
	const auto& scale = displayData::defaultScale;
	SDL_Rect destination{point.x().get() * scale, point.y().get() * scale, source.w, source.h};
	window.m_renderBuffer.addFacing(*this, destination, facing, color);
}
void Sprite::drawTinted(Window& window, const SDL_Rect& destination, const SDL_Color color) const
{
	window.m_renderBuffer.add(*this, destination, color);
}
void Sprite::drawTinted(Window& window, const Point3D point, const SDL_Color color) const
{
	const auto& scale = displayData::defaultScale;
	SDL_Rect destination{point.x().get() * scale, point.y().get() * scale, source.w, source.h};
	drawTinted(window, destination, color);
}
void Sprite::drawRepeated(Window& window, const Cuboid& cuboid) const
{
	drawRepeatedAndTinted(window, cuboid, SDL_Color{255,255,255,255});
}
void Sprite::drawRepeatedAndTinted(Window& window, const Cuboid& cuboid, const SDL_Color color) const
{
	const auto& scale = displayData::defaultScale;
	int w = cuboid.sizeX().get();
	int h = cuboid.sizeY().get();
	SDL_Rect destination
	{
		cuboid.m_low.x().get() * scale,
		window.invertY(cuboid.m_high.y()).get() * scale,
		w * scale,
		h * scale
	};
	window.m_renderBuffer.addTiled(*this, destination, w, h, color);
}
void Sprite::drawScaled(Window& window, const Cuboid& cuboid) const
{
	drawScaledAndTinted(window, cuboid, SDL_Color{255,255,255,255});
}
void Sprite::drawScaledAndTinted(Window& window, const Cuboid& cuboid, const SDL_Color color) const
{
	SDL_Rect destination
	{
		cuboid.m_low.x().get() * displayData::defaultScale,
		window.invertY(cuboid.m_high.y()).get() * displayData::defaultScale,
		cuboid.sizeX().get() * displayData::defaultScale,
		cuboid.sizeY().get() * displayData::defaultScale
	};
	window.m_renderBuffer.add(*this, destination, color);
}
void Sprite::drawRepeated(Window& window, const CuboidSet& cuboids) const
{
	for(const Cuboid& cuboid : cuboids)
		drawRepeated(window, cuboid);
}
void Sprite::drawRepeatedAndTinted(Window& window, const CuboidSet& cuboids, const SDL_Color color) const
{
	for(const Cuboid& cuboid : cuboids)
		drawRepeatedAndTinted(window, cuboid, color);
}
void Sprite::drawScaled(Window& window, const Point3D point, const float scale) const
{
	drawScaledAndTinted(window, point, scale, SDL_Color{255,255,255,255});
}
void Sprite::drawScaled(Window& window, const CuboidSet& cuboids) const
{
	for(const Cuboid& cuboid : cuboids)
		drawScaled(window, cuboid);
}
void Sprite::drawScaledAndTinted(Window& window, const CuboidSet& cuboids, const SDL_Color color) const
{
	for(const Cuboid& cuboid : cuboids)
		drawScaledAndTinted(window, cuboid, color);
}
void Sprite::drawScaledAndTinted(Window& window, const Point3D point, const float scale, const SDL_Color color) const
{
	const auto& baseScale = displayData::defaultScale;
	int w = (float)baseScale * scale;
	int h = w;
	SDL_Rect destination{point.x().get() * baseScale, point.y().get() * baseScale, w, h};
	destination.x = destination.x + (scale / 2) - (destination.w / 2);
	destination.y = destination.y + (scale / 2) - (destination.h / 2);
	drawTinted(window, destination, color);
}