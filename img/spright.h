// automatically generated with spright
// https://github.com/houmain/spright
#include <unordered_map>
#include <string>

namespace spriteCoordinates
{
	struct SpriteCoords {
		int x, y, w, h;
		float pivot_x, pivot_y;
	};
	inline std::unordered_map<std::string, SpriteCoords> data;

	inline void load()
	{
		data.try_emplace("roughFloor", 224, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("roughWall", 30, 172, 32, 19, 16.0, 9.5 );
		data.try_emplace("roughWallTop", 123, 188, 32, 10, 16.0, 5.0 );
		data.try_emplace("door", 192, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("flap", 160, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("hatch", 128, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("stairs", 96, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("ramp", 64, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("floorGrate", 30, 151, 32, 21, 16.0, 10.5 );
		data.try_emplace("floodGate", 0, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("fortification", 0, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("grass", 224, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("bush", 192, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("tree1", 160, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("tree2", 128, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockFloor", 96, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockWall", 193, 173, 32, 17, 16.0, 8.5 );
		data.try_emplace("blockWallTop", 155, 188, 32, 6, 16.0, 3.0 );
		data.try_emplace("sleep", 152, 174, 20, 14, 10.0, 7.0 );
		data.try_emplace("hand", 87, 125, 28, 26, 14.0, 13.0 );
		data.try_emplace("open", 248, 149, 7, 9, 3.5, 4.5 );
		data.try_emplace("trunk", 228, 149, 20, 25, 10.0, 12.5 );
		data.try_emplace("trunkWithBranches", 76, 64, 32, 31, 16.0, 15.5 );
		data.try_emplace("trunkLeaves", 64, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("branch", 59, 125, 28, 26, 14.0, 13.0 );
		data.try_emplace("branchLeaves", 152, 150, 30, 24, 15.0, 12.0 );
		data.try_emplace("fluidSurface", 32, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("bear", 197, 94, 32, 30, 16.0, 15.0 );
		data.try_emplace("fish", 91, 151, 32, 20, 16.0, 10.0 );
		data.try_emplace("deer", 197, 64, 32, 30, 16.0, 15.0 );
		data.try_emplace("equine", 108, 64, 32, 31, 16.0, 15.5 );
		data.try_emplace("raptor", 169, 64, 28, 31, 14.0, 15.5 );
		data.try_emplace("rabbit", 0, 64, 31, 32, 15.5, 16.0 );
		data.try_emplace("fox", 0, 125, 29, 26, 14.5, 13.0 );
		data.try_emplace("goblin", 31, 64, 23, 32, 11.5, 16.0 );
		data.try_emplace("dwarf", 140, 64, 29, 31, 14.5, 15.5 );
		data.try_emplace("troll", 136, 95, 28, 30, 14.0, 15.0 );
		data.try_emplace("axe", 164, 124, 32, 26, 16.0, 13.0 );
		data.try_emplace("fruit", 54, 64, 22, 32, 11.0, 16.0 );
		data.try_emplace("bar", 229, 94, 14, 28, 7.0, 14.0 );
		data.try_emplace("block", 196, 124, 32, 26, 16.0, 13.0 );
		data.try_emplace("board", 0, 151, 30, 23, 15.0, 11.5 );
		data.try_emplace("rock", 32, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("bucket", 106, 95, 30, 30, 15.0, 15.0 );
		data.try_emplace("tool", 76, 95, 30, 30, 15.0, 15.0 );
		data.try_emplace("cart", 193, 150, 32, 23, 16.0, 11.5 );
		data.try_emplace("arrow", 91, 171, 32, 20, 16.0, 10.0 );
		data.try_emplace("crossbow", 62, 151, 29, 21, 14.5, 10.5 );
		data.try_emplace("dagger", 136, 150, 16, 25, 8.0, 12.5 );
		data.try_emplace("pile", 62, 172, 28, 19, 14.0, 9.5 );
		data.try_emplace("log", 115, 125, 21, 26, 10.5, 13.0 );
		data.try_emplace("sword", 0, 96, 29, 29, 14.5, 14.5 );
		data.try_emplace("panniers", 164, 95, 30, 29, 15.0, 14.5 );
		data.try_emplace("cloths", 29, 124, 30, 27, 15.0, 13.5 );
		data.try_emplace("peg", 0, 174, 19, 17, 9.5, 8.5 );
		data.try_emplace("meal", 136, 125, 28, 25, 14.0, 12.5 );
		data.try_emplace("rope", 229, 64, 26, 30, 13.0, 15.0 );
		data.try_emplace("seed", 182, 150, 11, 24, 5.5, 12.0 );
		data.try_emplace("pick", 29, 96, 29, 28, 14.5, 14.0 );
		data.try_emplace("mallet", 229, 122, 27, 27, 13.5, 13.5 );
		
	}
} // namespace
