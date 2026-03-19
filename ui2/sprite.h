#pragma once
/*
	Sprite construction causes a hashmap lookup, each source rect should have an associated Sprite constrcuted only once, either durring load or when a method with a static Sprite stack variable.
*/
#include<SDL2/SDL.h>
#include<string>
#include "../engine/numericTypes/types.h"
class Window;
class CuboidSet;
class Cuboid;
class Point3D;
struct Sprite
{
	SDL_Rect source;
	float u0 = 0.0;
	float v0 = 0.0;
	float u1 = 0.0;
	float v1 = 0.0;
	static inline SDL_Texture* sheet;
	static inline float sheetWidth;
	static inline float sheetHeight;
	static void load(Window& window);
	Sprite(const std::string& name);
	void draw(Window& window, const SDL_Rect& destination) const;
	void draw(Window& window, const Point3D point) const;
	void drawTinted(Window& window, const SDL_Rect& destination, const SDL_Color color) const;
	void drawTinted(Window& window, const Point3D point, const SDL_Color color) const;
	void drawTintedAndRightAligned(Window& window, const Point3D point, const SDL_Color color) const;
	void drawTintedAndRightAlignedAndOffsetNorthAndEast(Window& window, const Point3D point, const SDL_Color color, const int offset) const;
	void drawRepeated(Window& window, const Cuboid cuboid) const;
	void drawRepeatedAndTinted(Window& window, const Cuboid cuboid, const SDL_Color color) const;
	void drawRepeatedAndTintedAndOffsetNorth(Window& window, const Cuboid cuboid, const SDL_Color color, const int offset) const;
	void drawRepeatedVerticallyAndTintedAndRightAligned(Window& window, const Cuboid cuboid, const SDL_Color color) const;
	void drawRepeatedVerticallyAndTintedAndRightAlignedAndOffsetEast(Window& window, const Cuboid cuboid, const SDL_Color color, const int offset) const;
	void drawScaled(Window& window, const Cuboid cuboid) const;
	void drawScaledAndTinted(Window& window, const Cuboid cuboid, const SDL_Color color) const;
	void drawScaledAndTinted(Window& window, const Point3D Point3D, const float scale, const SDL_Color color) const;
	void drawRepeated(Window& window, const CuboidSet& cuboids) const;
	void drawRepeatedAndTinted(Window& window, const CuboidSet& cuboids, const SDL_Color color) const;
	void drawRepeatedAndTintedAndOffsetNorth(Window& window, const CuboidSet& cuboids, const SDL_Color color, const int offset) const;
	void drawRepeatedVerticallyAndTintedAndRightAligned(Window& window, const CuboidSet& cuboids, const SDL_Color color) const;
	void drawRepeatedVerticallyAndTintedAndRightAlignedAndOffsetEast(Window& window, const CuboidSet& cuboids, const SDL_Color color, int offset) const;
	void drawScaled(Window& window, const Point3D point, const float scale) const;
	void drawScaled(Window& window, const CuboidSet& cuboids) const;
	void drawScaledAndTinted(Window& window, const CuboidSet& cuboids, const SDL_Color color) const;
	void drawRotated90CWAndTinted(Window& window, const Point3D point, const SDL_Color color) const;
	void drawRotatedAndTinted(Window& window, const Point3D point, const double angle, const SDL_Color color) const;
	void drawFacingAndTinted(Window& window, const Point3D point, const Facing4 facing, const SDL_Color color) const;
	[[nodiscard]] bool initalized() const;
};