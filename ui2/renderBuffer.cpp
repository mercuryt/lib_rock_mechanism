#include "renderBuffer.h"
#include "sprite.h"
#include "../engine/numericTypes/types.h"
#include <cassert>
RenderBuffer::RenderBuffer()
{
	buildIndices(100'000);
}
void RenderBuffer::buildIndices(int spriteCount)
{
	int vertexIndex = m_vertices.capacity();
	int i = m_indices.size();
	int end = spriteCount * 6;
	assert(end > i);
	m_indices.resize(end);
	for(; i != end; i += 6)
	{
		m_indices[i+0] = vertexIndex + 0;
		m_indices[i+1] = vertexIndex + 1;
		m_indices[i+2] = vertexIndex + 2;

		m_indices[i+3] = vertexIndex + 2;
		m_indices[i+4] = vertexIndex + 3;
		m_indices[i+5] = vertexIndex + 0;
		vertexIndex += 4;
	}
	m_vertices.reserve(m_vertices.capacity() + spriteCount * 4);
}
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
	float x = (float)destination.x;
	float y = (float)destination.y;
	float w = (float)destination.w;
	float h = (float)destination.h;
	float u0 = sprite.u0;
	float v0 = sprite.v0;
	float u1 = sprite.u1;
	float v1 = sprite.v1;
	auto base = m_vertices.size();
	m_vertices.resize(base + 4);
	// Top-left
	m_vertices[base+0] = {
		{ x, y},
		color,
		{ u0, v0 }
	};
	// Top-right
	m_vertices[base+1] = {
		{ x + w, y },
		color,
		{ u1, v0 }
	};
	// Bottom-right
	m_vertices[base+2] = {
		{ x + w, y + h },
		color,
		{ u1, v1 }
	};
	// Bottom-left
	m_vertices[base+3] = {
		{ x, y + h },
		color,
		{ u0, v1 }
	};
	m_indicesUsed += 6;
}
void RenderBuffer::addRotated(const Sprite& sprite, const SDL_Rect& destination, const double angleDegrees, const SDL_Color color)
{
	// Sprite rectangle
	float x = static_cast<float>(destination.x);
	float y = static_cast<float>(destination.y);
	float w = static_cast<float>(destination.w);
	float h = static_cast<float>(destination.h);

	// Texture coordinates
	float u0 = sprite.u0;
	float v0 = sprite.v0;
	float u1 = sprite.u1;
	float v1 = sprite.v1;

	// Center of the rectangle
	float cx = x + w * 0.5f;
	float cy = y + h * 0.5f;

	// Half-width and half-height
	float hw = w * 0.5f;
	float hh = h * 0.5f;

	// Precompute rotation
	float rad = static_cast<float>(angleDegrees * 0.017453292519943295f);
	float cosA = std::cos(rad);
	float sinA = std::sin(rad);

	// Rotated corner vectors
	float rx = cosA * hw;
	float ry = sinA * hw;
	float ux = -sinA * hh;
	float uy = cosA * hh;

	// Compute corners
	SDL_FPoint p0 = { cx - rx - ux, cy - ry - uy }; // top-left
	SDL_FPoint p1 = { cx + rx - ux, cy + ry - uy }; // top-right
	SDL_FPoint p2 = { cx + rx + ux, cy + ry + uy }; // bottom-right
	SDL_FPoint p3 = { cx - rx + ux, cy - ry + uy }; // bottom-left
	// Add vertices
	auto base = m_vertices.size();
	m_vertices.resize(base + 4);
	m_vertices[base + 0] = { p0, color, { u0, v0 } };
	m_vertices[base + 1] = { p1, color, { u1, v0 } };
	m_vertices[base + 2] = { p2, color, { u1, v1 } };
	m_vertices[base + 3] = { p3, color, { u0, v1 } };
	m_indicesUsed += 6;
}
void RenderBuffer::addRotated90CW(const Sprite& sprite, const SDL_Rect& destination, const SDL_Color color)
{
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
	m_vertices.push_back({ tl, color, { u0, v0 } });
	m_vertices.push_back({ tr, color, { u1, v0 } });
	m_vertices.push_back({ br, color, { u1, v1 } });
	m_vertices.push_back({ bl, color, { u0, v1 } });
	m_indicesUsed += 6;
}
void RenderBuffer::addFacing(const Sprite& sprite, const SDL_Rect& destination, const Facing4 facing, const SDL_Color color)
{
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
	m_vertices.push_back({ { x0, y0 }, color, { u0, v0 } });
	m_vertices.push_back({ { x1, y1 }, color, { u1, v0 } });
	m_vertices.push_back({ { x2, y2 }, color, { u1, v1 } });
	m_vertices.push_back({ { x3, y3 }, color, { u0, v1 } });
	m_indicesUsed += 6;
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
	int count = m_indices.size();
	if(count < m_indicesUsed)
		buildIndices(count * 2);
	SDL_RenderGeometry(renderer, Sprite::sheet, &m_vertices.front(), m_vertices.size(), &m_indices.front(), m_indicesUsed);
	m_indicesUsed = 0;
	m_vertices.clear();
}