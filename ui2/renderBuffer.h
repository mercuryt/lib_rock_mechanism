#pragma once
#include<SDL2/SDL.h>
#include<vector>
#include<cstdint>
#include "../engine/numericTypes/types.h"
struct Sprite;

struct RenderBuffer
{
	std::vector<SDL_Vertex> m_vertices;
	//TODO: use 98'298 int16_t indices and batch UINT16_MAX vertices for 16'383 sprites.
	std::vector<int32_t> m_indices;
	int m_indicesUsed{0};
	RenderBuffer();
	void buildIndices(const int spriteCount);
	void add(const SDL_Rect& destination, const SDL_Color color);
	void addOutline(const SDL_Rect& destination, const SDL_Color color, int thickness);
	void add(const Sprite& sprite, const SDL_Rect& destination, const SDL_Color color = {255, 255, 255, 255});
	void addRotated90CW(const Sprite& sprite, const SDL_Rect& destination, const SDL_Color color = {255, 255, 255, 255});
	void addRotated(const Sprite& sprite, const SDL_Rect& destination, const double angle, const SDL_Color color = {255, 255, 255, 255});
	void addFacing(const Sprite& sprite, const SDL_Rect& destination, const Facing4 facing, const SDL_Color color = {255, 255, 255, 255});
	// repetitionsX and repetitionsY are possed in to avoid extra division, even though they could be derived from sprite.source and destination.
	void addTiled(const Sprite& sprite, const SDL_Rect& destination, int repetitionsX, int repetitionsY, const SDL_Color color = {255, 255, 255, 255});
	void render(SDL_Renderer* renderer);
};