#include "renderBuffer.h"
#include "sprite.h"
#include "../engine/numericTypes/types.h"
#include <cassert>
void RenderBuffer::add(const SDL_Rect& destination, const SDL_Color color)
{
	static const Sprite pixel("pixel");
	add(pixel, destination, color);
}
void RenderBuffer::addOutline(const SDL_Rect& destination, const SDL_Color color, int thickness)
{
	assert(thickness <= destination.w  / 2);
	assert(thickness <= destination.h  / 2);
	SDL_Rect segment;
	// Top
	segment = {
		destination.x,
		destination.y,
		destination.w,
		thickness};
	add(segment, color);
	// Right
	segment = {
		destination.x + destination.w - thickness,
		destination.y + thickness,
		thickness,
		destination.h - (2 * thickness)
	};
	add(segment, color);
	// Bottom
	segment = {
		destination.x,
		destination.y + destination.h - thickness,
		destination.w,
		thickness
	};
	add(segment, color);
	// Left
	segment = {
		destination.x,
		destination.y + thickness,
		thickness,
		destination.h - (2 * thickness)
	};
	add(segment, color);
}
void RenderBuffer::add(const Sprite& sprite, const SDL_Rect& destination, const SDL_Color color)
{
	int startIndex = static_cast<int32_t>(m_verticies.size());
	float x = (float)destination.x;
	float y = (float)destination.y;
	float w = (float)destination.w;
	float h = (float)destination.h;
	float u0 = sprite.u0;
	float v0 = sprite.v0;
	float u1 = sprite.u1;
	float v1 = sprite.v1;
	// Top-left
	m_verticies.push_back({
		{ x, y},
		color,
		{ u0, v0 }
	});
	// Top-right
	m_verticies.push_back({
		{ x + w, y },
		color,
		{ u1, v0 }
	});
	// Bottom-right
	m_verticies.push_back({
		{ x + w, y + h },
		color,
		{ u1, v1 }
	});
	// Bottom-left
	m_verticies.push_back({
		{ x, y + h },
		color,
		{ u0, v1 }
	});
	// Two triangles
	m_indicies.push_back(startIndex + 0);
	m_indicies.push_back(startIndex + 1);
	m_indicies.push_back(startIndex + 2);

	m_indicies.push_back(startIndex + 2);
	m_indicies.push_back(startIndex + 3);
	m_indicies.push_back(startIndex + 0);
}
void RenderBuffer::addRotated90CW(const Sprite& sprite, const SDL_Rect& destination, const SDL_Color color)
{
	int startIndex = static_cast<int32_t>(m_verticies.size());
	float x = (float)destination.x;
	float y = (float)destination.y;
	float w = (float)destination.w;
	float h = (float)destination.h;
	float u0 = sprite.u0;
	float v0 = sprite.v0;
	float u1 = sprite.u1;
	float v1 = sprite.v1;
	// Center
	float cx = x + w * 0.5f;
	float cy = y + h * 0.5f;
	// Half Extent
	float hx = w * 0.5f;
	float hy = h * 0.5f;
	// 90° clockwise rotation:
	// (localX, localY) -> (localY, -localX)
	SDL_FPoint tl = { cx - hy, cy + hx };
	SDL_FPoint tr = { cx - hy, cy - hx };
	SDL_FPoint br = { cx + hy, cy - hx };
	SDL_FPoint bl = { cx + hy, cy + hx };
	m_verticies.push_back({ tl, color, { u0, v0 } });
	m_verticies.push_back({ tr, color, { u1, v0 } });
	m_verticies.push_back({ br, color, { u1, v1 } });
	m_verticies.push_back({ bl, color, { u0, v1 } });
	// Two triangles
	m_indicies.push_back(startIndex + 0);
	m_indicies.push_back(startIndex + 1);
	m_indicies.push_back(startIndex + 2);

	m_indicies.push_back(startIndex + 2);
	m_indicies.push_back(startIndex + 3);
	m_indicies.push_back(startIndex + 0);
}
void RenderBuffer::addRotated(const Sprite& sprite, const SDL_Rect& destination, const double angleDegrees, const SDL_Color color)
{
	const int startIndex = static_cast<int>(m_verticies.size());
	const float x = (float)destination.x;
	const float y = (float)destination.y;
	const float w = (float)destination.w;
	const float h = (float)destination.h;
	const float u0 = sprite.u0;
	const float v0 = sprite.v0;
	const float u1 = sprite.u1;
	const float v1 = sprite.v1;
	// Center
	const float cx = x + w * 0.5f;
	const float cy = y + h * 0.5f;
	// Half extents
	const float hw = w * 0.5f;
	const float hh = h * 0.5f;
	// Degrees -> radians
	const float radians = angleDegrees * 0.017453292519943295f; // PI / 180
	const float cosA = std::cos(radians);
	const float sinA = std::sin(radians);
	// Pre-rotated axis vectors
	const float dx_cos = hw * cosA;
	const float dx_sin = hw * sinA;
	const float dy_cos = hh * cosA;
	const float dy_sin = hh * sinA;
	// Top-left (-hw, -hh)
	m_verticies.push_back({
		{ cx - dx_cos + dy_sin, cy - dx_sin - dy_cos },
		color,
		{ u0, v0 }
	});
	// Top-right (+hw, -hh)
	m_verticies.push_back({
		{ cx + dx_cos + dy_sin, cy + dx_sin - dy_cos },
		color,
		{ u1, v0 }
	});
	// Bottom-right (+hw, +hh)
	m_verticies.push_back({
		{ cx + dx_cos - dy_sin, cy + dx_sin + dy_cos },
		color,
		{ u1, v1 }
	});
	// Bottom-left (-hw, +hh)
	m_verticies.push_back({
		{ cx - dx_cos - dy_sin, cy - dx_sin + dy_cos },
		color,
		{ u0, v1 }
	});
	// Two triangles
	m_indicies.push_back(startIndex + 0);
	m_indicies.push_back(startIndex + 1);
	m_indicies.push_back(startIndex + 2);

	m_indicies.push_back(startIndex + 2);
	m_indicies.push_back(startIndex + 3);
	m_indicies.push_back(startIndex + 0);
}
void RenderBuffer::addFacing(const Sprite& sprite, const SDL_Rect& destination, const Facing4 facing, const SDL_Color color)
{
	const int startIndex = static_cast<int>(m_verticies.size());
	const float x = (float)destination.x;
	const float y = (float)destination.y;
	const float w = (float)destination.w;
	const float h = (float)destination.h;
	const float u0 = sprite.u0;
	const float v0 = sprite.v0;
	const float u1 = sprite.u1;
	const float v1 = sprite.v1;
	const float cx = x + w * 0.5f;
	const float cy = y + h * 0.5f;
	const float hw = w * 0.5f;
	const float hh = h * 0.5f;
	float x0, y0;
	float x1, y1;
	float x2, y2;
	float x3, y3;
	switch (facing)
	{
		case Facing4::North:
		default:
			x0 = cx - hw; y0 = cy - hh;
			x1 = cx + hw; y1 = cy - hh;
			x2 = cx + hw; y2 = cy + hh;
			x3 = cx - hw; y3 = cy + hh;
			break;
		case Facing4::East:
			x0 = cx + hh; y0 = cy - hw;
			x1 = cx + hh; y1 = cy + hw;
			x2 = cx - hh; y2 = cy + hw;
			x3 = cx - hh; y3 = cy - hw;
			break;
		case Facing4::South:
			x0 = cx + hw; y0 = cy + hh;
			x1 = cx - hw; y1 = cy + hh;
			x2 = cx - hw; y2 = cy - hh;
			x3 = cx + hw; y3 = cy - hh;
			break;
		case Facing4::West:
			x0 = cx - hh; y0 = cy + hw;
			x1 = cx - hh; y1 = cy - hw;
			x2 = cx + hh; y2 = cy - hw;
			x3 = cx + hh; y3 = cy + hw;
			break;
	}
	m_verticies.push_back({ { x0, y0 }, color, { u0, v0 } });
	m_verticies.push_back({ { x1, y1 }, color, { u1, v0 } });
	m_verticies.push_back({ { x2, y2 }, color, { u1, v1 } });
	m_verticies.push_back({ { x3, y3 }, color, { u0, v1 } });
	// Two triangles
	m_indicies.push_back(startIndex + 0);
	m_indicies.push_back(startIndex + 1);
	m_indicies.push_back(startIndex + 2);

	m_indicies.push_back(startIndex + 2);
	m_indicies.push_back(startIndex + 3);
	m_indicies.push_back(startIndex + 0);
}
void RenderBuffer::addTiled(const Sprite& sprite, const SDL_Rect& d, int repetitionsX, int repetitionsY, const SDL_Color color)
{
	float startX = (float)d.x;
	float startY = (float)d.y;
	float tileW = (float)sprite.source.w;
	float tileH = (float)sprite.source.h;
	for(int y = 0; y < repetitionsY; ++y){
		float posY = startY + (y * tileH);
		for(int x = 0; x < repetitionsX; ++x){
			float posX = startX + (x * tileW);
			SDL_Rect tileRect{(int)posX, (int)posY, (int)tileW, (int)tileH};
			add(sprite, tileRect, color);
		}
	}
}
void RenderBuffer::render(SDL_Renderer* renderer)
{
	SDL_RenderGeometry(renderer, Sprite::sheet, &m_verticies.front(), m_verticies.size(), &m_indicies.front(), m_indicies.size());
	m_indicies.clear();
	m_verticies.clear();
}