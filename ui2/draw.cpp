#include "draw.h"
#include "window.h"
#include "displayData.h"
#include "sprite.h"
#include "../engine/area/area.h"
#include "../engine/space/space.h"
#include "../engine/actors/actors.h"
#include "../engine/items/items.h"
#include "../engine/plants.h"
#include "../engine/definitions/plantSpecies.h"
#include<SDL2/SDL.h>
void draw::world(Window& window)
{
	const Space& space = window.m_area->getSpace();
	int w, h;
	SDL_GetRendererOutputSize(window.m_sdlRenderer, &w, &h);
	Cuboid thisLevel = Cuboid::create(window.getBlockAtScreenPosition(0, 0), window.getBlockAtScreenPosition(window.m_screenWidth, window.m_screenHeight));
	static auto roughFloorSprite = Sprite("roughFloor");
	static auto blockFloorSprite = Sprite("blockFloor");
	space.pointFeature_queryForEachWithCuboids(thisLevel, [&](const Cuboid& cuboid, const PointFeature& pointFeature){
		if(pointFeature.pointFeatureType == PointFeatureTypeId::Floor)
		{
			assert(!pointFeature.isHewn());
			blockFloorSprite.drawRepeatedAndTinted(window, cuboid, displayData::materialColors[pointFeature.materialType]);
		}
	});
	static auto roughWallSouthSprite = Sprite("roughWallSouth");
	static auto roughWallWestSprite = Sprite("roughWallWest");
	static auto blockWallSouthSprite = Sprite("blockWallSouth");
	static auto blockWallWestSprite = Sprite("blockWallWest");
	if(window.m_pov->z != 0)
	{
		Cuboid nextLevelDown = thisLevel;
		nextLevelDown.shift(Facing6::Below, {1});
		space.solid_queryForEachWithCuboids(nextLevelDown, [&](const Cuboid& cuboid, const MaterialTypeId& materialType){
			const SDL_Color& color = displayData::materialColors[materialType];
			{
				CuboidSet constructed = space.queryConstructedGetCuboids(cuboid);
				blockFloorSprite.drawRepeatedAndTinted(window, constructed, color);
				CuboidSet rough = CuboidSet::create(cuboid);
				if(!constructed.empty())
					rough.removeAll(constructed);
				roughFloorSprite.drawRepeatedAndTinted(window, rough, color);
			}
			if(cuboid.m_low.y() != 0)
			{
				Cuboid southFace = cuboid.getFaceSouth();
				CuboidSet southResult = CuboidSet::create(southFace);
				CuboidSet constructed = space.queryConstructedGetCuboids(southResult);
				if(!constructed.empty())
				{
					southResult.removeAll(constructed);
					constructed.shiftSouth();
					space.solid_removeOpaque(constructed);
					blockWallSouthSprite.drawRepeatedAndTinted(window, constructed, color);
				}
				if(!southResult.empty())
				{
					southResult.shiftSouth();
					space.solid_removeOpaque(southResult);
					roughWallSouthSprite.drawRepeatedAndTinted(window, southResult, color);
				}
			}
			if(cuboid.m_low.x() != 0)
			{
				Cuboid westFace = cuboid.getFaceWest();
				CuboidSet westResult = CuboidSet::create(westFace);
				CuboidSet constructed = space.queryConstructedGetCuboids(westResult);
				if(!constructed.empty())
				{
					westResult.removeAll(constructed);
					constructed.shiftWest();
					space.solid_removeOpaque(constructed);
					blockWallWestSprite.drawRepeatedVerticallyAndTintedAndRightAligned(window, constructed, color);
				}
				if(!westResult.empty())
				{
					westResult.shiftWest();
					space.solid_removeOpaque(westResult);
					roughWallWestSprite.drawRepeatedVerticallyAndTintedAndRightAligned(window, westResult, color);
				}
			}
			if(cuboid.m_low.x() != 0 && cuboid.m_low.y() != 0)
			{
				Point3D location = cuboid.m_low;
				bool constructed = space.isConstructed(location);
				location = Point3D::create(location.applyOffset(Offset3D(-1, -1, 0)));
				if(!space.solid_isAny(location.north()) && !space.solid_isAny(location.east()) )
				{
					static auto roughWallCorner = Sprite("roughWallCorner");
					static auto blockWallCorner = Sprite("blockWallCorner");
					if(constructed)
						blockWallCorner.drawTintedAndRightAligned(window, location, color);
					else
						roughWallCorner.drawTintedAndRightAligned(window, location, color);
				}
			}
		});
		// FluidBelow
		static auto fluidSprite = Sprite("fluidSurface");
		space.fluid_queryForEachWithCuboids(nextLevelDown, [&](const Cuboid& cuboid, const FluidData& fluidData){
			fluidSprite.drawRepeatedAndTinted(window, cuboid, displayData::fluidColors[fluidData.type]);
		});
		space.fluid_queryTotalsForEachWithCuboids(thisLevel, [&](const Cuboid& cuboid, const CollisionVolume& volume){
			textOnCuboid(window, cuboid, volume.toString(), displayData::fluidVolumeTextColor);
		});
	}
	SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>> featuresOnThisZLevel;
	space.pointFeature_queryForEachWithCuboids(thisLevel, [&](const Cuboid& cuboid, const PointFeature& pointFeature){
		featuresOnThisZLevel.getOrCreate(pointFeature.pointFeatureType).getOrCreate(pointFeature).add(cuboid);
	});
	// Floor type features.
	featureType(window, PointFeatureTypeId::Floor, blockFloorSprite, featuresOnThisZLevel);
	static const Sprite hatch("hatch");
	featureType(window, PointFeatureTypeId::Hatch, hatch, featuresOnThisZLevel);
	static const Sprite floorGrate("floorGrate");
	featureType(window, PointFeatureTypeId::FloorGrate, floorGrate, featuresOnThisZLevel);
	// Solid.
	space.solid_queryForEachWithCuboids(thisLevel, [&](const Cuboid cuboid, const MaterialTypeId materialTypeId) {
		const SDL_Color& color = displayData::materialColors[materialTypeId];
		colorOnCuboid(window, cuboid, color);
		if(cuboid.m_low.y() != 0)
		{
			Cuboid southFace = cuboid.getFaceSouth();
			CuboidSet southResult = CuboidSet::create(southFace);
			CuboidSet constructed = space.queryConstructedGetCuboids(southResult);
			if(!constructed.empty())
			{
				southResult.removeAll(constructed);
				constructed.shiftSouth();
				space.solid_removeOpaque(constructed);
				blockWallSouthSprite.drawRepeatedAndTinted(window, constructed, color);
			}
			if(!southResult.empty())
			{
				southResult.shiftSouth();
				space.solid_removeOpaque(southResult);
				roughWallSouthSprite.drawRepeatedAndTinted(window, southResult, color);
			}
		}
		if(cuboid.m_low.x() != 0)
		{
			Cuboid westFace = cuboid.getFaceWest();
			CuboidSet westResult = CuboidSet::create(westFace);
			CuboidSet constructed = space.queryConstructedGetCuboids(westResult);
			if(!constructed.empty())
			{
				westResult.removeAll(constructed);
				constructed.shiftWest();
				space.solid_removeOpaque(constructed);
				blockWallWestSprite.drawRepeatedVerticallyAndTintedAndRightAligned(window, constructed, color);
			}
			if(!westResult.empty())
			{
				westResult.shiftWest();
				space.solid_removeOpaque(westResult);
				roughWallWestSprite.drawRepeatedVerticallyAndTintedAndRightAligned(window, westResult, color);
			}
		}
		if(cuboid.m_low.x() != 0 && cuboid.m_low.y() != 0)
		{
			Point3D location = cuboid.m_low;
			bool constructed = space.isConstructed(location);
			location = Point3D::create(location.applyOffset(Offset3D(-1, -1, 0)));
			if(!space.solid_isAny(location.north()) && !space.solid_isAny(location.east()) )
			{
				static auto roughWallCorner = Sprite("roughWallCorner");
				static auto blockWallCorner = Sprite("blockWallCorner");
				if(constructed)
					blockWallCorner.drawTintedAndRightAligned(window, location, color);
				else
					roughWallCorner.drawTintedAndRightAligned(window, location, color);
			}
		}
	});
	// Groundcover plants on current level.
	SmallMap<PlantSpeciesId, SmallSet<PlantIndex>> nongroundCoverPlantsOnCurrentLevel;
	const Plants& plants = window.m_area->getPlants();
	space.plant_queryForEachWithCuboids(thisLevel, [&](const Cuboid cuboid, const PlantIndex plant){
		const PlantSpeciesId& species = plants.getSpecies(plant);
		const PlantSpeciesDisplayData& display = displayData::plantData[species];
		if(display.groundCover)
			display.sprite.drawRepeatedAndTinted(window, cuboid, display.color);
		else
			nongroundCoverPlantsOnCurrentLevel.getOrCreate(species).insertNonunique(plant);
	});
	for(auto& [species, plantIndices] : nongroundCoverPlantsOnCurrentLevel)
		plantIndices.makeUnique();
	if(window.m_faction.exists())
	// Stockpiles.
	space.stockpile_queryForEachCuboidForFaction(thisLevel, window.m_faction, [&](const Cuboid cuboid){
		colorOnCuboid(window, cuboid, displayData::stockPileColor);
	});
	// Farm fields.
	space.farm_queryForEachCuboidForFaction(thisLevel, window.m_faction, [&](const Cuboid cuboid){
		colorOnCuboid(window, cuboid, displayData::farmFieldColor);
	});
	// Fluid on current level.
	space.fluid_queryForEachWithCuboids(thisLevel, [&](const Cuboid& cuboid, const FluidData fluidData){
		colorOnCuboid(window, cuboid, displayData::fluidColors[fluidData.type]);
	});
	// TODO: fluid volume totals.
	// Nonfloor type features.
	static Sprite ramp("ramp");
	rampOrStairs(window, PointFeatureTypeId::Ramp, ramp, featuresOnThisZLevel);
	static Sprite stairs("stairs");
	rampOrStairs(window, PointFeatureTypeId::Stairs, stairs, featuresOnThisZLevel);
	static Sprite floodGate("floodGate");
	featureTypeRotated90IfInaccessableNorthAndSouth(window, PointFeatureTypeId::FloodGate, floodGate, featuresOnThisZLevel);
	static Sprite fortification("fortification");
	featureTypeRotated90IfInaccessableNorthAndSouth(window, PointFeatureTypeId::Fortification, fortification, featuresOnThisZLevel);
	static Sprite flap("flap");
	featureTypeRotated90IfInaccessableNorthAndSouth(window, PointFeatureTypeId::Flap, flap, featuresOnThisZLevel);
	static Sprite door("door");
	featureTypeRotated90IfInaccessableNorthAndSouth(window, PointFeatureTypeId::Door, door, featuresOnThisZLevel);
	// Non-Ground cover plants.
	for(const auto& [speciesId, plantIndices] : nongroundCoverPlantsOnCurrentLevel)
	{
		const PlantSpeciesDisplayData& display = displayData::plantData[speciesId];
		for(const auto& plantIndex : plantIndices)
			// TODO: scale by percent grown as well as cuboid.
			nonGroundCoverPlant(window, plantIndex, display);
	}
	// Render items.
	Items& items = window.m_area->getItems();
	CuboidSet containsItems;
	space.item_queryForEachWithCuboid(thisLevel, [&](const Cuboid cuboid, const ItemIndex item){
		containsItems.maybeAdd(cuboid);
		itemAtLocation(window, item);
	});
	for(const Cuboid& cuboid : containsItems)
		for(const Point3D point : cuboid)
			itemOverlay(window, point);
	// Render actors.
	CuboidSet containsActors;
	space.actor_queryForEachWithCuboid(thisLevel, [&](const Cuboid cuboid, const ActorIndex actor) {
		containsActors.add(cuboid);
		actorAtLocation(window, actor);
	});
	for(const Cuboid& cuboid : containsActors)
		for(const Point3D point : cuboid)
			actorOverlay(window, point);
	// Designated and project progress.
	if(window.m_faction.exists())
	{
		window.getArea()->m_spaceDesignations.queryForEachForFactionIfExists(window.m_faction, thisLevel, [&](const Cuboid& cuboid){
			//TODO: This is slower then it should be.
			for(const Point3D point : cuboid)
				maybeDesignated(window, point);
		});
		const static Sprite tool("tool");
		window.m_area->m_hasCraftingLocationsAndJobs.getForFaction(window.m_faction).forEachPoint([&](const Point3D point){
			tool.drawTinted(window, point, displayData::selectColor);
		});
		space.project_queryForEachWithLocation(window.m_faction, thisLevel, [&](const Project& project, const Point3D location){
			Percent progress = project.getPercentComplete();
			if(progress != 0)
				progressBarOnBlock(window, location, progress);
		});
	}
	// Obscure unrevealed.
	// TODO: prevent rendering calls which will be obscured here anyway?
	if(!window.m_editMode)
		space.unrevealed_queryForEach(thisLevel, [&](const Cuboid& cuboid){
			colorOnCuboid(window, cuboid, displayData::unrevealedColor);
		});
	// Draw border around area.
	int worldWidth = window.m_area->getSpace().m_sizeX.get() * displayData::defaultScale;
	int worldHeight = window.m_area->getSpace().m_sizeY.get() * displayData::defaultScale;
	window.m_renderBuffer.addOutline(SDL_Rect{0, 0, worldWidth, worldHeight}, displayData::areaOutlineColor, displayData::areaOutlineWidth);
	// Draw selected boxes which show what is currently selected.
	selectedBoxes(window);
	// Item being installed or targeted hauling.
	if(window.m_gameOverlay.m_itemBeingInstalled.exists() || window.m_gameOverlay.m_itemBeingMoved.exists())
	{
		const Point3D hoverBlock = window.m_gameOverlay.m_blockUnderCursor;
		if(hoverBlock.exists())
		{
			const ItemReference itemRef = (window.m_gameOverlay.m_itemBeingInstalled.exists() ?
				window.m_gameOverlay.m_itemBeingInstalled :
				window.m_gameOverlay.m_itemBeingMoved
			);
			const ItemIndex item = itemRef.getIndex(items.m_referenceData);
			auto occupiedBlocks = items.getCuboidsWhichWouldBeOccupiedAtLocationAndFacing(item, hoverBlock, window.m_gameOverlay.m_facing);
			bool valid = space.shape_shapeAndMoveTypeCanEnterEverWithFacing(hoverBlock, items.getShape(item), items.getMoveType(item), window.m_gameOverlay.m_facing);
			for(const Cuboid& cuboid : occupiedBlocks)
				for(const Point3D point : cuboid)
					if(!valid)
						invalidOnBlock(window, point);
					else
						validOnBlock(window, point);
		}
	}
}
void draw::nonGroundCoverPlant(Window& window, const PlantIndex plant, const PlantSpeciesDisplayData& display)
{
	Space& space = window.m_area->getSpace();
	Plants& plants = window.m_area->getPlants();
	// Ground cover plants are drawn with floors.
	assert(!display.groundCover);
	const Point3D location = plants.getLocation(plant);
	auto& occupied = plants.getOccupied(plant);
	const CuboidSet sliced = occupied.slicedAtZ(window.m_pov->z);
	if(PlantSpecies::getIsTree(plants.getSpecies(plant)) && occupied.volume() != 1)
	{
		for(const Cuboid& cuboid : sliced)
			for(const Point3D point : cuboid)
			{
				if(point == location && occupied.volume() > 2)
				{
					static Sprite trunk("trunk");
					trunk.draw(window, point);
				}
				else
				{
					if(point.x() == location.x() && point.y() == location.y())
					{
						if(point.z() == space.m_sizeZ)
						{
							display.sprite.draw(window, point);
						}
						else
						{
							const Point3D above = point.above();
							if(above.exists() && space.plant_exists(above) && space.plant_get(above) == plant)
							{
								static Sprite trunkWithBranches("trunkWithBranches");
								trunkWithBranches.draw(window, point);
								static Sprite trunkLeaves("trunkLeaves");
								trunkLeaves.draw(window, point);
							}
							else
								display.sprite.draw(window, point);
						}
					}
					else
					{
						float angle = 45.f * (uint)location.getFacingTwordsIncludingDiagonal(point);
						static Sprite branch("branch");
						branch.drawRotatedAndTinted(window, point, angle, display.color);
						static Sprite branchLeaves("branchLeaves");
						branchLeaves.drawRotated90CWAndTinted(window, point, display.color);
					}
				}
			}
	}
	else
	{
		if(plants.getOccupied(plant).size() == 1)
			display.sprite.draw(window, location);
		else
			display.sprite.drawScaled(window, location, ((float)plants.getPercentGrown(plant).get() / 100.f));
	}
}
void draw::colorOnArea(Window& window, const CuboidSet& area, const SDL_Color color)
{
	for(const Cuboid& cuboid : area)
		draw::colorOnCuboid(window, cuboid, color);
}
void draw::colorOnCuboid(Window& window, const Cuboid cuboid, const SDL_Color color)
{
	const auto& scale = displayData::defaultScale;
	int x = cuboid.m_low.x().get() * scale;
	int y = window.invertY(cuboid.m_high.y()).get() * scale;
	int w = cuboid.sizeX().get() * scale;
	int h = cuboid.sizeY().get() * scale;
	SDL_Rect rect{x, y, w, h};
	window.m_renderBuffer.add(rect, color);
}
void draw::colorOutlineArea(Window& window, const CuboidSet& area, const SDL_Color color, const int thickness)
{
	for(const Cuboid& cuboid : area)
		draw::colorOutlineCuboid(window, cuboid, color, thickness);
}
void draw::colorOutlineCuboid(Window& window, const Cuboid cuboid, const SDL_Color color, const int thickness)
{
	const auto& scale = displayData::defaultScale;
	int x = cuboid.m_low.x().get() * scale;
	int y = window.invertY(cuboid.m_high.y()).get() * scale;
	int w = cuboid.sizeX().get() * scale;
	int h = cuboid.sizeY().get() * scale;
	for(int i = 0; i < thickness; i++)
	{
		SDL_Rect rect{x + i, y + i, w - 2 * i, h - 2 * i};
		window.m_renderBuffer.add(rect, color);
	}
}
void draw::colorOutlineCuboids(Window& window, const CuboidSet& cuboids, const SDL_Color color, const int thickness)
{
	// TODO: Don't draw internal faces.
	for(const Cuboid& cuboid : cuboids)
		colorOutlineCuboid(window, cuboid, color, thickness);
}
void draw::textAtCoordinates(Window& window, const int startX, const int startY, const std::string& text, const SDL_Color color, const int size)
{
	const int& charWidth = size;
	const int& charHeight = size;
	const int& spacing = displayData::textSpacing;
	// ---- Uppercase ----
	static const Sprite sA("A"); static const Sprite sB("B");
	static const Sprite sC("C"); static const Sprite sD("D");
	static const Sprite sE("E"); static const Sprite sF("F");
	static const Sprite sG("G"); static const Sprite sH("H");
	static const Sprite sI("I"); static const Sprite sJ("J");
	static const Sprite sK("K"); static const Sprite sL("L");
	static const Sprite sM("M"); static const Sprite sN("N");
	static const Sprite sO("O"); static const Sprite sP("P");
	static const Sprite sQ("Q"); static const Sprite sR("R");
	static const Sprite sS("S"); static const Sprite sT("T");
	static const Sprite sU("U"); static const Sprite sV("V");
	static const Sprite sW("W"); static const Sprite sX("X");
	static const Sprite sY("Y"); static const Sprite sZ("Z");
	// ---- Lowercase ----
	static const Sprite sa("a"); static const Sprite sb("b");
	static const Sprite sc("c"); static const Sprite sd("d");
	static const Sprite se("e"); static const Sprite sf("f");
	static const Sprite sg("g"); static const Sprite sh("h");
	static const Sprite si("i"); static const Sprite sj("j");
	static const Sprite sk("k"); static const Sprite sl("l");
	static const Sprite sm("m"); static const Sprite sn("n");
	static const Sprite so("o"); static const Sprite sp("p");
	static const Sprite sq("q"); static const Sprite sr("r");
	static const Sprite ss("s"); static const Sprite st("t");
	static const Sprite su("u"); static const Sprite sv("v");
	static const Sprite sw("w"); static const Sprite sx("x");
	static const Sprite sy("y"); static const Sprite sz("z");
	// ---- Numbers ----
	static const Sprite s0("0"); static const Sprite s1("1");
	static const Sprite s2("2"); static const Sprite s3("3");
	static const Sprite s4("4"); static const Sprite s5("5");
	static const Sprite s6("6"); static const Sprite s7("7");
	static const Sprite s8("8"); static const Sprite s9("9");

	int x = startX;

	for (char c : text)
	{
		SDL_Rect destination { x, startY, charWidth, charHeight };

		switch (c)
		{
			// Uppercase
			case 'A': sA.drawTinted(window, destination, color); break;
			case 'B': sB.drawTinted(window, destination, color); break;
			case 'C': sC.drawTinted(window, destination, color); break;
			case 'D': sD.drawTinted(window, destination, color); break;
			case 'E': sE.drawTinted(window, destination, color); break;
			case 'F': sF.drawTinted(window, destination, color); break;
			case 'G': sG.drawTinted(window, destination, color); break;
			case 'H': sH.drawTinted(window, destination, color); break;
			case 'I': sI.drawTinted(window, destination, color); break;
			case 'J': sJ.drawTinted(window, destination, color); break;
			case 'K': sK.drawTinted(window, destination, color); break;
			case 'L': sL.drawTinted(window, destination, color); break;
			case 'M': sM.drawTinted(window, destination, color); break;
			case 'N': sN.drawTinted(window, destination, color); break;
			case 'O': sO.drawTinted(window, destination, color); break;
			case 'P': sP.drawTinted(window, destination, color); break;
			case 'Q': sQ.drawTinted(window, destination, color); break;
			case 'R': sR.drawTinted(window, destination, color); break;
			case 'S': sS.drawTinted(window, destination, color); break;
			case 'T': sT.drawTinted(window, destination, color); break;
			case 'U': sU.drawTinted(window, destination, color); break;
			case 'V': sV.drawTinted(window, destination, color); break;
			case 'W': sW.drawTinted(window, destination, color); break;
			case 'X': sX.drawTinted(window, destination, color); break;
			case 'Y': sY.drawTinted(window, destination, color); break;
			case 'Z': sZ.drawTinted(window, destination, color); break;

			// Lowercase
			case 'a': sa.drawTinted(window, destination, color); break;
			case 'b': sb.drawTinted(window, destination, color); break;
			case 'c': sc.drawTinted(window, destination, color); break;
			case 'd': sd.drawTinted(window, destination, color); break;
			case 'e': se.drawTinted(window, destination, color); break;
			case 'f': sf.drawTinted(window, destination, color); break;
			case 'g': sg.drawTinted(window, destination, color); break;
			case 'h': sh.drawTinted(window, destination, color); break;
			case 'i': si.drawTinted(window, destination, color); break;
			case 'j': sj.drawTinted(window, destination, color); break;
			case 'k': sk.drawTinted(window, destination, color); break;
			case 'l': sl.drawTinted(window, destination, color); break;
			case 'm': sm.drawTinted(window, destination, color); break;
			case 'n': sn.drawTinted(window, destination, color); break;
			case 'o': so.drawTinted(window, destination, color); break;
			case 'p': sp.drawTinted(window, destination, color); break;
			case 'q': sq.drawTinted(window, destination, color); break;
			case 'r': sr.drawTinted(window, destination, color); break;
			case 's': ss.drawTinted(window, destination, color); break;
			case 't': st.drawTinted(window, destination, color); break;
			case 'u': su.drawTinted(window, destination, color); break;
			case 'v': sv.drawTinted(window, destination, color); break;
			case 'w': sw.drawTinted(window, destination, color); break;
			case 'x': sx.drawTinted(window, destination, color); break;
			case 'y': sy.drawTinted(window, destination, color); break;
			case 'z': sz.drawTinted(window, destination, color); break;

			// Numbers
			case '0': s0.drawTinted(window, destination, color); break;
			case '1': s1.drawTinted(window, destination, color); break;
			case '2': s2.drawTinted(window, destination, color); break;
			case '3': s3.drawTinted(window, destination, color); break;
			case '4': s4.drawTinted(window, destination, color); break;
			case '5': s5.drawTinted(window, destination, color); break;
			case '6': s6.drawTinted(window, destination, color); break;
			case '7': s7.drawTinted(window, destination, color); break;
			case '8': s8.drawTinted(window, destination, color); break;
			case '9': s9.drawTinted(window, destination, color); break;

			default: break; // ignore unsupported characters
		}

		x += charWidth + spacing;
	}
}
void draw::textAtPoint(Window& window, const Point3D point, const std::string& text, const SDL_Color color, const int size)
{
	const auto& scale = displayData::defaultScale;
	textAtCoordinates(window, point.x().get() * scale, window.invertY(point.y()).get() * scale, text, color, size);
}
void draw::textOnCuboid(Window& window, const Cuboid cuboid, const std::string& text, const SDL_Color color, const int size)
{
	for(const Point3D point : cuboid)
		textAtPoint(window, point, text, color, size);
}
void draw::featureType(Window& window, const PointFeatureTypeId type, const Sprite& sprite, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features)
{
	auto found = features.find(type);
	if(found != features.end())
	{
		for(const auto& [pointFeature, cuboids] : found->second)
		{
			const SDL_Color color = displayData::materialColors[pointFeature.materialType];
			sprite.drawRepeatedAndTinted(window, cuboids, color);
		}
	}
}
void draw::rampOrStairs(Window& window, const PointFeatureTypeId type, const Sprite& sprite, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features)
{
	auto found = features.find(type);
	if(found!= features.end())
	{
		for(const auto& [feature, cuboids] : found->second)
		{
			const SDL_Color color = displayData::materialColors[feature.materialType];
			for(const Cuboid& cuboid : cuboids)
				for(const Point3D point : cuboid)
				{
					Facing4 facing = rampOrStairsFacing(window, point);
					sprite.drawFacingAndTinted(window, point, facing, color);
				}
		}
	}
}
void draw::featureTypeRotated90IfInaccessableNorthAndSouth(Window& window, const PointFeatureTypeId type, const Sprite& sprite, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features)
{
	const Space& space = window.getArea()->getSpace();
	auto found = features.find(type);
	if(found != features.end())
	{
		for(const auto& [feature, cuboids] : found->second)
		{
			const SDL_Color color = displayData::materialColors[feature.materialType];
			for(const Cuboid& cuboid : cuboids)
				for(const Point3D point : cuboid)
				{
					if(point.y() == space.m_sizeY - 1 || point.x() == 0 || (!space.solid_isAny(point.north()) || !space.solid_isAny(point.south())))
						sprite.drawTinted(window, point, color);
					else
						sprite.drawRotated90CWAndTinted(window, point, color);
				}
		}
	}
}
void draw::actorAtLocation(Window& window, const ActorIndex actor)
{
	Actors& actors = window.m_area->getActors();
	AnimalSpeciesDisplayData& display = displayData::actorData[actors.getSpecies(actor)];
	const Point3D location = actors.getLocation(actor);
	display.sprite.draw(window, location);
	if(Shape::getIsMultiTile(actors.getShape(actor)))
		draw::colorOutlineCuboids(window, actors.getOccupied(actor), displayData::actorOutlineColor, displayData::actorOutlineThickness);
	if(actors.canPickUp_exists(actor))
	{
		const ActorOrItemIndex isCarrying = actors.canPickUp_getPolymorphic(actor);
		if(isCarrying.isItem())
			itemBeingCarried(window, isCarrying.getItem(), location);
		else
		{
			assert(isCarrying.isActor());
			// TODO: Draw actors being carried.
		}
	}
}
void draw::itemAtLocation(Window& window, const ItemIndex item)
{
	Items& items = window.m_area->getItems();
	ItemTypeDisplayData& display = displayData::itemData[items.getItemType(item)];
	SDL_Color color = displayData::materialColors[items.getMaterialType(item)];
	display.sprite.drawTinted(window, items.getLocation(item), color);
	if(Shape::getIsMultiTile(items.getShape(item)))
		draw::colorOutlineCuboids(window, items.getOccupied(item), displayData::itemOutlineColor, displayData::itemOutlineThickness);
}
void draw::itemBeingCarried(Window& window, const ItemIndex item, const Point3D point)
{
	Items& items = window.m_area->getItems();
	ItemTypeDisplayData& display = displayData::itemData[items.getItemType(item)];
	display.sprite.draw(window, point);
	// TODO: outline for multi tile?
}
void draw::selectedBoxes(Window& window)
{
	switch(window.m_gameOverlay.m_selectMode)
	{
		case(SelectMode::Actors):
		{
			const Actors& actors = window.m_area->getActors();
			for(const ActorReference& ref : window.m_gameOverlay.m_selectedActors)
			{
				const ActorIndex actor = ref.getIndex(actors.m_referenceData);
				const CuboidSet& occupied = actors.getOccupied(actor);
				colorOutlineCuboids(window, occupied, displayData::selectColor, displayData::selectBoxThickness);
			}
			break;
		}
		case(SelectMode::Items):
		{
			const Items& items = window.m_area->getItems();
			for(const ItemReference& ref : window.m_gameOverlay.m_selectedItems)
			{
				const ItemIndex item = ref.getIndex(items.m_referenceData);
				const CuboidSet& occupied = items.getOccupied(item);
				colorOutlineCuboids(window, occupied, displayData::selectColor, displayData::selectBoxThickness);
			}
			break;
		}
		case(SelectMode::Space):
		{
			for(const Cuboid& cuboid : window.m_gameOverlay.m_selectedArea)
				if(cuboid.m_high.z() >= window.m_pov->z && cuboid.m_low.z() <= window.m_pov->z)
					colorOnCuboid(window, cuboid, displayData::selectColorOverlay);
			break;
		}
		case(SelectMode::Plants):
		{
			window.m_area->getSpace().plant_queryForEachCuboid(window.m_gameOverlay.m_selectedArea, [&](const Cuboid cuboid){
				colorOutlineCuboid(window, cuboid, displayData::selectColor, displayData::selectBoxThickness);
			});
			break;
		}
	}
}
void draw::itemOverlay(Window& window, const Point3D point)
{
	Space& space = window.m_area->getSpace();
	Items& items = window.m_area->getItems();
	assert(!space.item_empty(point));
	auto itemIndices = space.item_getAll(point);
	const ItemIndex item = itemIndices.front();
	if(itemIndices.size() > 1)
		textAtPoint(window, point, std::to_string(space.item_getAll(point).size()), displayData::itemTypeCountColor);
	else if(items.getQuantity(item) != 1)
		textAtPoint(window, point, std::to_string(items.getQuantity(item).get()), displayData::itemQuantityColor);
	if(window.m_faction.exists() && !items.stockpile_canBeStockPiled(item, window.m_faction))
		inaccessableSymbol(window, point);
}
void draw::actorOverlay(Window& window, const Point3D point)
{
	Space& space = window.m_area->getSpace();
	Actors& actors = window.m_area->getActors();
	const ActorIndex actor = space.actor_getAll(point).front();
	if(!actors.sleep_isAwake(actor))
		textAtPoint(window, point, "zzz", displayData::sleepZZZColor);
}
void draw::validOnBlock(Window& window, const Point3D point)
{
	static const Sprite sprite("valid");
	sprite.draw(window, point);
}
void draw::invalidOnBlock(Window& window, const Point3D point)
{
	static const Sprite sprite("invalid");
	sprite.draw(window, point);
}
void draw::accessableSymbol(Window& window, const Point3D point)
{
	static const Sprite sprite("o");
	sprite.drawTinted(window, point, displayData::allowedColor);
}
void draw::inaccessableSymbol(Window& window, const Point3D point)
{
	static const Sprite sprite("x");
	sprite.drawTinted(window, point, displayData::allowedColor);
}
void draw::progressBarOnBlock(Window& window, const Point3D point, const Percent progress)
{
	const auto& scale = displayData::defaultScale;
	int x = point.x().get() * scale;
	int y = point.y().get() * scale;
	SDL_Rect background{x, y, scale, displayData::progressBarThickness};
	window.m_renderBuffer.add(background, displayData::progressBarBackgroundColor);
	SDL_Rect border{x, y, scale, displayData::progressBarThickness};
	window.m_renderBuffer.addOutline(border, displayData::progressBarOutlineColor, displayData::progressBarOutlineThickness);
	int width = util::scaleByPercent(scale, progress);
	SDL_Rect bar{x, y, width, displayData::progressBarThickness};
	window.m_renderBuffer.add(bar, displayData::progressBarColor);
}
void draw::maybeDesignated(Window& window, const Point3D point)
{
	SpaceDesignation designation = window.m_area->m_spaceDesignations.getForFaction(window.m_faction).getDisplayDesignation(point);
	if(designation == SpaceDesignation::SPACE_DESIGNATION_MAX)
		return;
	static SmallMap<SpaceDesignation, Sprite> designationSprites{
		{SpaceDesignation::Dig, {"pick"}},
		{SpaceDesignation::Construct, {"mallet"}},
		{SpaceDesignation::FluidSource, {"bucket"}},
		{SpaceDesignation::GivePlantFluid, {"bucket"}},
		{SpaceDesignation::SowSeeds, {"seed"}},
		{SpaceDesignation::Harvest, {"hand"}},
		{SpaceDesignation::Rescue, {"hand"}},
		{SpaceDesignation::Sleep, {"sleep"}},
		{SpaceDesignation::WoodCutting, {"axe"}},
		{SpaceDesignation::StockPileHaulFrom, {"hand"}},
		{SpaceDesignation::StockPileHaulTo, {"open"}}
	};
	designationSprites[designation].draw(window, point);
}
Facing4 draw::rampOrStairsFacing(Window& window, const Point3D point)
{
	Space& space = window.m_area->getSpace();
	static auto canConnectToAbove = [&](const Point3D otherPoint) -> bool {
		const Point3D above = otherPoint.above();
		return above.exists() && !space.pointFeature_contains(otherPoint, PointFeatureTypeId::Stairs) && !space.pointFeature_contains(otherPoint, PointFeatureTypeId::Ramp) && space.shape_canStandIn(above);
	};
	const Point3D above = point.above();
	if(above.empty())
		return Facing4::North;
	Facing4 backup = Facing4::North;
	const Cuboid boundry = space.boundry();
	if(point.y() != boundry.m_high.y() && canConnectToAbove(point.north()))
	{
		const Point3D south = point.south();
		if(!space.solid_isAny(south) && space.shape_canStandIn(south) &&
			!space.pointFeature_contains(south, PointFeatureTypeId::Stairs) &&
			!space.pointFeature_contains(south, PointFeatureTypeId::Stairs)
		)
			return Facing4::North;
	}
	if(point.x() != boundry.m_high.x() && canConnectToAbove(point.east()))
	{
		const Point3D west = point.west();
		if(!space.solid_isAny(west) && space.shape_canStandIn(west))
			return Facing4::East;
		else
			backup = Facing4::East;
	}
	if(point.y() != 0 && canConnectToAbove(point.south()))
	{
		const Point3D north = point.north();
		if(!space.solid_isAny(north) && space.shape_canStandIn(north) &&
			!space.pointFeature_contains(north, PointFeatureTypeId::Stairs) &&
			!space.pointFeature_contains(north, PointFeatureTypeId::Stairs)
		)
			return Facing4::South;
		else
			backup = Facing4::South;
	}
	if(point.x() != 0 && canConnectToAbove(point.west()))
	{
		const Point3D east = point.east();
		if(!space.solid_isAny(east) && space.shape_canStandIn(point))
			return Facing4::West;
		else
			backup = Facing4::West;
	}
	return backup;
}