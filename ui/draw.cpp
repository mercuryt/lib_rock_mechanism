#include "draw.h"
#include "sprite.h"
#include "window.h"
#include "../engine/dataStructures/smallMap.h"
#include "../engine/space/space.h"
#include "../engine/actors/actors.h"
#include "../engine/items/items.h"
#include "../engine/plants.h"
#include "../engine/geometry/cuboidSet.h"
#include "../engine/definitions/plantSpecies.h"
#include "displayData.h"
#include "interpolate.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
void Draw::view()
{
	Space& space = m_window.m_area->getSpace();
	Actors& actors = m_window.m_area->getActors();
	Items& items = m_window.m_area->getItems();
	Plants& plants = m_window.m_area->getPlants();
	m_window.m_gameOverlay.drawZoom();
	m_window.m_gameOverlay.drawTime();
	m_window.m_gameOverlay.drawSelectionDescription();
	//m_gameOverlay.drawWeather();
	// Aquire Area read mutex.
	std::lock_guard lock(m_window.m_simulation->m_uiReadMutex);
	// Render point floors, collect actors into single and multi tile groups.
	SmallSet<Point3D> singleTileActorBlocks;
	SmallSet<ActorIndex> multiTileActors;
	const Cuboid zLevelBlocks = space.getZLevel(m_window.m_z);
	for(const Point3D& point : zLevelBlocks)
	{
		blockFloor(point);
		for(const ActorIndex& actor : space.actor_getAll(point))
		{
			if(Shape::getIsMultiTile(actors.getShape(actor)))
				multiTileActors.maybeInsert(actor);
			else
				singleTileActorBlocks.insert(point);
		}
	}
	// Renger Block floors.
	for(const Point3D& point : zLevelBlocks)
		blockFloor(point);
	// Render point wall corners.
	for(const Point3D& point : zLevelBlocks)
		blockWallCorners(point);
	// Render point walls.
	for(const Point3D& point : zLevelBlocks)
		blockWalls(point);
	// Render point features and fluids.
	for(const Point3D& point : zLevelBlocks)
		pointFeaturesAndFluids(point);
	// Render point plants.
	for(const Point3D& point : zLevelBlocks)
		nonGroundCoverPlant(point);
	// Render items.
	for(const Point3D& point : zLevelBlocks)
		item(point);
	// Render point wall tops.
	for(const Point3D& point : zLevelBlocks)
		blockWallTops(point);
	// Render Actors.
	// Multi tile actors first.
	//TODO: what if multitile actors overlap?
	for(const ActorIndex& actor : multiTileActors)
		multiTileActor(actor);
	// Single tile actors.
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	for(const Point3D& point : singleTileActorBlocks)
	{
		assert(!space.actor_empty(point));
		// Multiple actors, cycle through them over time.
		// Use real time rather then m_step to continue cycling while paused.
		auto& actorsInBlock = space.actor_getAll(point);
		uint32_t count = actorsInBlock.size();
		if(count == 1)
			singleTileActor(actorsInBlock.front());
		else
		{
			uint8_t index = (seconds % count);
			//TODO: hide some actors from player?
			singleTileActor(actorsInBlock[index]);
		}
	}
	// Designated and project progress.
	if(m_window.m_faction.exists())
	{
		if(m_window.m_area->m_spaceDesignations.contains(m_window.m_faction))
		{
			for(const Point3D& point : zLevelBlocks)
				maybeDesignated(point);
		}
		for(const Point3D& point : zLevelBlocks)
		{
			craftLocation(point);
			Percent projectProgress = space.project_getPercentComplete(point, m_window.m_faction);
			if(projectProgress != 0)
				progressBarOnBlock(point, projectProgress);
		}
	}
	// Render item overlays.
	for(const Point3D& point : zLevelBlocks)
		itemOverlay(point);
	// Render actor overlays.
	for(const Point3D& point : singleTileActorBlocks)
		actorOverlay(space.actor_getAll(point).front());

	// Selected.
	if(!m_window.m_selectedBlocks.empty())
	{
		for(const Cuboid& cuboid : m_window.m_selectedBlocks.getCuboids())
			selected(cuboid);
	}
	else if(!m_window.m_selectedActors.empty())
	{
		for(const ActorIndex& actor: m_window.m_selectedActors)
			for(const Cuboid& cuboid : actors.getOccupied(actor))
				for(const Point3D& point : cuboid)
					if(point.z() == m_window.m_z)
						selected(point);
	}
	else if(!m_window.m_selectedItems.empty())
	{
		for(const ItemIndex& item: m_window.m_selectedItems)
			for(const Cuboid& cuboid : items.getOccupied(item))
				for(const Point3D& point : cuboid)
					if(point.z() == m_window.m_z)
						selected(point);
	}
	else if(!m_window.m_selectedPlants.empty())
		for(const PlantIndex& plant: m_window.m_selectedPlants)
			for(const Cuboid& cuboid : plants.getOccupied(plant))
				for(const Point3D& point : cuboid)
					if(point.z() == m_window.m_z)
						selected(point);
	// Selection Box.
	// TODO: Show which space would be added / removed.
	if(m_window.m_firstCornerOfSelection.exists() && sf::Mouse::isButtonPressed(displayData::selectMouseButton))
	{
		auto end = sf::Mouse::getPosition();
		auto start = m_window.m_positionWhereMouseDragBegan;
		uint32_t xSize = std::abs((int32_t)start.x - (int32_t)end.x);
		uint32_t ySize = std::abs((int32_t)start.y - (int32_t)end.y);
		int32_t left = std::min(start.x, end.x);
		int32_t top = std::min(start.y, end.y);
		sf::Vector2f worldPos = m_window.m_window.mapPixelToCoords({left, top});
		sf::RectangleShape rectangle(sf::Vector2f(xSize, ySize));
		rectangle.setFillColor(sf::Color::Transparent);
		rectangle.setOutlineColor(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ? displayData::cancelColor : displayData::selectColor);
		rectangle.setOutlineThickness(3.f);
		rectangle.setPosition(worldPos);
		m_window.m_window.draw(rectangle);
	}
	// Install or move item.
	if(m_window.m_gameOverlay.m_itemBeingInstalled.exists() || m_window.m_gameOverlay.m_itemBeingMoved.exists())
	{
		assert(m_window.m_blockUnderCursor.exists());
		const Point3D& hoverBlock = m_window.m_blockUnderCursor;
		const ItemIndex& item = (m_window.m_gameOverlay.m_itemBeingInstalled.exists() ?
			m_window.m_gameOverlay.m_itemBeingInstalled :
			m_window.m_gameOverlay.m_itemBeingMoved
		);
		auto occupiedBlocks = items.getCuboidsWhichWouldBeOccupiedAtLocationAndFacing(item, hoverBlock, m_window.m_gameOverlay.m_facing);
		bool valid = space.shape_shapeAndMoveTypeCanEnterEverWithFacing(hoverBlock, items.getShape(item), items.getMoveType(item), m_window.m_gameOverlay.m_facing);
		for(const Cuboid& cuboid : occupiedBlocks)
			for(const Point3D& point : cuboid)
				if(!valid)
					invalidOnBlock(point);
				else
					validOnBlock(point);
	}
	// Area Border.
	sf::RectangleShape areaBorder(sf::Vector2f((space.m_sizeX.get() * m_window.m_scale), (space.m_sizeX.get() * m_window.m_scale) ));
	areaBorder.setOutlineColor(sf::Color::White);
	areaBorder.setFillColor(sf::Color::Transparent);
	areaBorder.setOutlineThickness(3.f);
	areaBorder.setPosition(sf::Vector2f(0,0));
	m_window.m_window.draw(areaBorder);
	// Update Info popup.
	//if(m_gameOverlay.infoPopupIsVisible())
		//m_gameOverlay.updateInfoPopup();
}
// Point3D.
void Draw::blockFloor(const Point3D& point)
{
	Space& space = m_window.m_area->getSpace();
	if(space.solid_isAny(point))
	{
		for(const Point3D& adjacent : space.getDirectlyAdjacent(point))
			if(m_window.m_editMode || space.isVisible(adjacent))
			{
				colorOnBlock(point, displayData::materialColors[space.solid_get(point)]);
				break;
			}
	}
	else if(point.z() != 0 && (m_window.m_editMode || space.isVisible(point)))
	{
		const Point3D& below = point.below();
		if(space.solid_isAny(below))
		{
			// Draw floor.
			if(space.plant_exists(point))
			{
				Plants &plants = m_window.m_area->getPlants();
				const PlantSpeciesId& species = plants.getSpecies(space.plant_get(point));
				const PlantSpeciesDisplayData& display = displayData::plantData[species];
				if(display.groundCover)
				{
					//TODO: color.
					imageOnBlock(point, display.image);
					if(m_window.getSelectedPlants().contains(space.plant_get(point)))
						outlineOnBlock(point, displayData::selectColor);
					return;
				}
			}
			// No ground cover plant.
			const sf::Color* color = &displayData::materialColors[space.solid_get(point.below())];
			if(space.isConstructed(below))
			{
				static sf::Sprite sprite = sprites::make("blockFloor").first;
				spriteOnBlock(point, sprite, color);
			}
			else
			{
				static sf::Sprite sprite = sprites::make("roughFloor").first;
				spriteOnBlock(point, sprite, color);
			}
			// Draw stockpiles.
			if(space.stockpile_contains(point, m_window.getFaction()))
				colorOnBlock(point, displayData::stockPileColor);
			// Draw farm fields.
			if(space.farm_contains(point, m_window.getFaction()))
				colorOnBlock(point, displayData::farmFieldColor);
			// Draw cliff edge, ramp, or stairs below floor.
			blockWallsFromNextLevelDown(point);
		}
		else if(space.fluid_getTotalVolume(below) > Config::maxPointVolume * displayData::minimumFluidVolumeToSeeFromAboveLevelRatio)
		{
			const FluidTypeId& fluidType = space.fluid_getTypeWithMostVolume(below);
			const sf::Color color = displayData::fluidColors[fluidType];
			static sf::Sprite sprite = sprites::make("fluidSurface").first;
			// TODO: Give the shore line some feeling of depth somehow.
			spriteOnBlock(point, sprite, &color);
		}
	}
}
void Draw::blockWallCorners(const Point3D& point)
{
	Space& space = m_window.m_area->getSpace();
	if(
		(m_window.m_editMode || space.isVisible(point)) &&
		space.solid_isAny(point) && point.x() != 0 && !space.solid_isAny(point.west()) && point.y() != 0 && !space.solid_isAny(point.south())
	)
	{
		const sf::Color color = displayData::materialColors[space.solid_get(point)];
		float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
		auto pair = sprites::make(space.isConstructed(point) ? "blockWall" : "roughWall");
		auto sprite = pair.first;
		sprite.setPosition(((float)point.x().get() - 0.21f) * (float)m_window.m_scale, ((float)point.y().get() + 0.48f) * (float)m_window.m_scale);
		sprite.setScale(scaleRatio, scaleRatio);
		sprite.setColor(color);
		sprite.setRotation(45);
		m_window.getRenderWindow().draw(sprite);
	}
	else if(point.z() != 0 && point.x() != 0 && point.y() != 0)
	{
		const Point3D& below = point.below();
		const Point3D& belowWest = below.west();
		const Point3D& belowSouth = below.south();
		if(
			(m_window.m_editMode || space.isVisible(below)) &&
			!space.solid_isAny(point) && space.solid_isAny(below) && !space.solid_isAny(belowWest) && !space.solid_isAny(belowSouth)
		)
		{
			const sf::Color color = displayData::materialColors[space.solid_get(below)];
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			auto pair = sprites::make(space.isConstructed(point) ? "blockWall" : "roughWall");
			auto sprite = pair.first;
			sprite.setPosition(((float)point.x().get() - 0.21f) * (float)m_window.m_scale, ((float)point.y().get() + 0.48f) * (float)m_window.m_scale);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			sprite.setRotation(45);
			m_window.getRenderWindow().draw(sprite);
		}
	}
}
void Draw::blockWalls(const Point3D& point)
{
	Space& space = m_window.m_area->getSpace();
	if(space.solid_isAny(point))
	{
		auto adjacentPredicate = [&](const Point3D& adjacent){
			return (m_window.m_editMode || space.isVisible(adjacent)) && !space.solid_isAny(adjacent) && !space.pointFeature_contains(adjacent, PointFeatureTypeId::Stairs) && !space.pointFeature_contains(adjacent, PointFeatureTypeId::Ramp);
		};
		if(point.y() != 0)
		{
			const Point3D& south = point.south();
			if(adjacentPredicate(south))
			{
				const sf::Color color = displayData::materialColors[space.solid_get(point)];
				float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
				auto pair = sprites::make(space.isConstructed(point) ? "blockWall" : "roughWall");
				auto sprite = pair.first;
				sprite.setPosition((float)point.x().get() * m_window.m_scale, (float)(point.y().get() + 1) * m_window.m_scale);
				sprite.setScale(scaleRatio, scaleRatio);
				sprite.setColor(color);
				m_window.getRenderWindow().draw(sprite);
			}
		}
		if(point.y() != 0)
		{
			const Point3D& west = point.west();
			if(adjacentPredicate(west))
			{
				const sf::Color color = displayData::materialColors[space.solid_get(point)];
				float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
				auto pair = sprites::make(space.isConstructed(point) ? "blockWall" : "roughWall");
				auto sprite = pair.first;
				sprite.setPosition((float)point.x().get() * m_window.m_scale, (float)point.y().get() * m_window.m_scale);
				sprite.setScale(scaleRatio, scaleRatio);
				sprite.setColor(color);
				sprite.setRotation(90);
				m_window.getRenderWindow().draw(sprite);
			}
		}
	}
}
void Draw::blockWallTops(const Point3D& point)
{
	Space& space = m_window.m_area->getSpace();
	if(space.solid_isAny(point))
	{
		auto adjacentPredicate = [&](const Point3D& adjacent){
			return (m_window.m_editMode || space.isVisible(adjacent)) && !space.solid_isAny(adjacent) && !space.pointFeature_contains(adjacent, PointFeatureTypeId::Stairs) && !space.pointFeature_contains(adjacent, PointFeatureTypeId::Ramp);
		};
		float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
		const sf::Color color = displayData::materialColors[space.solid_get(point)];
		float offset = displayData::wallTopOffsetRatio * m_window.m_scale;
		const Point3D& north = point.north();
		if(point.x() != space.m_sizeX - 1)
		{
			if(adjacentPredicate(north))
			{
				auto pair = sprites::make(space.isConstructed(point) ? "blockWallTop" : "roughWallTop");
				auto sprite = pair.first;
				sprite.setRotation(180);
				sprite.setPosition((float)(point.x().get() + 1) * m_window.m_scale, ((float)point.y().get() * m_window.m_scale) + offset);
				sprite.setScale(scaleRatio, scaleRatio);
				sprite.setColor(color);
				m_window.getRenderWindow().draw(sprite);
			}
		}
		if(point.y() != space.m_sizeY - 1)
		{
			const Point3D& east = point.east();
			if(adjacentPredicate(east))
			{
				auto pair = sprites::make(space.isConstructed(point) ? "blockWallTop" : "roughWallTop");
				auto sprite = pair.first;
				sprite.setRotation(270);
				sprite.setPosition((((float)point.x().get() + 1) * m_window.m_scale) - offset, ((float)point.y().get() + 1) * m_window.m_scale);
				sprite.setScale(scaleRatio, scaleRatio);
				sprite.setColor(color);
				m_window.getRenderWindow().draw(sprite);
			}
		}
		if(point.x() != 0)
		{
			const Point3D& west = point.west();
			if(adjacentPredicate(west))
			{
				auto pair = sprites::make(space.isConstructed(point) ? "blockWallTop" : "roughWallTop");
				auto sprite = pair.first;
				sprite.setRotation(90);
				sprite.setPosition(((float)point.x().get() * m_window.m_scale) + offset, ((float)point.y().get() * m_window.m_scale));
				sprite.setScale(scaleRatio, scaleRatio);
				sprite.setColor(color);
				m_window.getRenderWindow().draw(sprite);
			}
		}
		if(point.y() != 0)
		{
			const Point3D& south = point.south();
			if(adjacentPredicate(south))
			{
				auto pair = sprites::make(space.isConstructed(point) ? "blockWallTop" : "roughWallTop");
				auto sprite = pair.first;
				sprite.setPosition(((float)point.x().get() * m_window.m_scale), ((float)(point.y().get() + 1) * m_window.m_scale) - offset);
				sprite.setScale(scaleRatio, scaleRatio);
				sprite.setColor(color);
				m_window.getRenderWindow().draw(sprite);
			}
		}
	}
}
void Draw::pointFeaturesAndFluids(const Point3D& point)
{
	// Point3D Features
	//TODO: Draw order.
	Space& space = m_window.m_area->getSpace();
	if(!space.pointFeature_empty(point))
	{
		for(const PointFeature& pointFeature : space.pointFeature_getAll(point))
		{
			sf::Color* color = &displayData::materialColors[pointFeature.materialType];
			if(pointFeature.pointFeatureType == PointFeatureTypeId::Hatch)
			{
				static sf::Sprite hatch = getCenteredSprite("hatch");
				spriteOnBlockCentered(point, hatch, color);
			}
			else if(pointFeature.pointFeatureType == PointFeatureTypeId::FloorGrate)
			{
				static sf::Sprite floorGrate = getCenteredSprite("floorGrate");
				spriteOnBlockCentered(point, floorGrate, color);
			}
			else if(pointFeature.pointFeatureType == PointFeatureTypeId::Stairs)
			{
				static sf::Sprite stairs = getCenteredSprite("stairs");
				Facing4 facing = rampOrStairsFacing(point);
				stairs.setRotation((uint)facing * 90);
				stairs.setOrigin(16,19);
				spriteOnBlockCentered(point, stairs, color);
			}
			else if(pointFeature.pointFeatureType == PointFeatureTypeId::Ramp)
			{
				static sf::Sprite ramp = getCenteredSprite("ramp");
				Facing4 facing = rampOrStairsFacing(point);
				ramp.setRotation((uint)facing * 90);
				ramp.setOrigin(16,19);
				spriteOnBlockCentered(point, ramp, color);
			}
			else if(pointFeature.pointFeatureType == PointFeatureTypeId::FloodGate)
			{
				static sf::Sprite floodGate = getCenteredSprite("floodGate");
				const Point3D& north = point.y() == space.m_sizeY - 1 ? Point3D::null() : point.north();
				const Point3D& south = point.y() == 0 ? Point3D::null() : point.south();
				// Default floodGate image leads north-south, maybe rotate.
				if(north.empty() || space.solid_isAny(north) || south.empty() || space.solid_isAny(south))
					floodGate.setRotation(90);
				else
					floodGate.setRotation(0);
				spriteOnBlockCentered(point, floodGate, color);
			}
			else if(pointFeature.pointFeatureType == PointFeatureTypeId::Fortification)
			{
				static sf::Sprite fortification = getCenteredSprite("fortification");
				const Point3D& north = point.y() == space.m_sizeY - 1 ? Point3D::null() : point.north();
				const Point3D& south = point.y() == 0 ? Point3D::null() : point.south();
				// Default fortification image leads north-south, maybe rotate.
				if(north.empty() || space.solid_isAny(north) || !space.shape_canStandIn(north) || south.empty() || space.solid_isAny(south) || !space.shape_canStandIn(south))
					fortification.setRotation(90);
				else
					fortification.setRotation(0);
				spriteOnBlockCentered(point, fortification, color);
			}
			else if(pointFeature.pointFeatureType == PointFeatureTypeId::Door)
			{
				static sf::Sprite door = getCenteredSprite("door");
				const Point3D& north = point.y() == space.m_sizeY - 1 ? Point3D::null() : point.north();
				const Point3D& south = point.y() == 0 ? Point3D::null() : point.south();
				// Default door image leads north-south, maybe rotate.
				if(north.empty() || space.solid_isAny(north) || !space.shape_canStandIn(north) || south.empty() || space.solid_isAny(south) || !space.shape_canStandIn(south))
					door.setRotation(90);
				else
					door.setRotation(0);
				spriteOnBlockCentered(point, door, color);
			}
			else if(pointFeature.pointFeatureType == PointFeatureTypeId::Flap)
			{
				static sf::Sprite flap = getCenteredSprite("flap");
				const Point3D& north = point.y() == space.m_sizeY - 1 ? Point3D::null() : point.north();
				const Point3D& south = point.y() == 0 ? Point3D::null() : point.south();
				// Default flap image leads north-south, maybe rotate.
				if(north.empty() || space.solid_isAny(north) || !space.shape_canStandIn(north) || south.empty() || space.solid_isAny(south) || !space.shape_canStandIn(south))
					flap.setRotation(90);
				else
					flap.setRotation(0);
				spriteOnBlockCentered(point, flap, color);
			}
		}
	}
	else if(point.z() != 0)
	{
		const Point3D& below = point.below();
		if(!space.solid_isAny(below))
		{
			// Show tops of stairs and ramps from next level down.
			if(space.pointFeature_contains(below, PointFeatureTypeId::Stairs))
			{
				const PointFeature& pointFeature = space.pointFeature_at(below, PointFeatureTypeId::Stairs);
				sf::Color* color = &displayData::materialColors[pointFeature.materialType];
				static sf::Sprite stairs = getCenteredSprite("stairs");
				Facing4 facing = rampOrStairsFacing(below);
				stairs.setRotation((uint)facing * 90);
				stairs.setOrigin(16,19);
				stairs.setTextureRect({0,0,32,16});
				spriteOnBlockCentered(point, stairs, color);
			}
			else if(space.pointFeature_contains(below, PointFeatureTypeId::Ramp))
			{
				const PointFeature& pointFeature = space.pointFeature_at(below, PointFeatureTypeId::Ramp);
				sf::Color* color = &displayData::materialColors[pointFeature.materialType];
				static sf::Sprite ramp = getCenteredSprite("ramp");
				Facing4 facing = rampOrStairsFacing(below);
				ramp.setRotation((uint)facing * 90);
				ramp.setOrigin(16,19);
				ramp.setTextureRect({0,0,32,16});
				spriteOnBlockCentered(point, ramp, color);
			}
		}
	}
	// Fluids
	if(space.fluid_getTotalVolume(point) != 0)
	{
		const FluidTypeId& fluidType = space.fluid_getTypeWithMostVolume(point);
		CollisionVolume volume = space.fluid_volumeOfTypeContains(point, fluidType);
		const sf::Color color = displayData::fluidColors[fluidType];
		colorOnBlock(point, color);
		stringOnBlock(point, std::to_string(volume.get()), sf::Color::Black);
	}
}
void Draw::blockWallsFromNextLevelDown(const Point3D& point)
{
	if(point.z() == 0)
		return;
	Space& space = m_window.m_area->getSpace();
	const Point3D& below = point.below();
	if(!space.solid_isAny(below))
		return;
	if(below.y() != 0)
	{
		const Point3D& belowSouth = below.south();
		if(belowSouth.exists() && !space.solid_isAny(belowSouth))
		{
			sf::Sprite* sprite;
			MaterialTypeId materialType;
			if(space.isConstructed(point))
			{
				static sf::Sprite blockWall = getCenteredSprite("blockWall");
				sprite = &blockWall;
				materialType = space.solid_get(below);
			}
			else
			{
				static sf::Sprite roughWall = getCenteredSprite("roughWall");
				sprite = &roughWall;
				materialType = space.solid_get(below);
			}
			sprite->setTextureRect({0, 0, 32, 18});
			assert(materialType.exists());
			assert(displayData::materialColors.contains(materialType));
			sf::Color color = displayData::materialColors[materialType];
			sf::Vector2f position{(((float)belowSouth.x().get() + 0.5f) * (float)m_window.m_scale), (((float)belowSouth.y().get() + 0.5f - displayData::wallOffset) * (float)m_window.m_scale)};
			spriteAt(*sprite, position, &color);
		}
	}
	if(below.x() != 0)
	{
		const Point3D& belowWest = below.west();
		if(!space.solid_isAny(belowWest))
		{
			static sf::Sprite* sprite;
			MaterialTypeId materialType;
			if(space.isConstructed(point))
			{
				static sf::Sprite blockWall = getCenteredSprite("blockWall");
				sprite = &blockWall;
			}
			else
			{
				static sf::Sprite roughWall = getCenteredSprite("roughWall");
				sprite = &roughWall;
			}
			materialType = space.solid_get(below);
			sprite->setTextureRect({0, 0, 32, 18});
			sprite->setRotation(90);
			assert(displayData::materialColors.contains(materialType));
			sf::Color color = displayData::materialColors[materialType];
			sf::Vector2f position{(((float)belowWest.x().get() + 0.5f + displayData::wallOffset) * (float)m_window.m_scale), (((float)belowWest.y().get() + 0.5f) * (float)m_window.m_scale)};
			spriteAt(*sprite, position, &color);
		}
	}
}
void Draw::validOnBlock(const Point3D& point)
{
	colorOnBlock(point, {0, 255, 0, 122});
}
void Draw::invalidOnBlock(const Point3D& point)
{
	colorOnBlock(point, {255, 0, 0, 122});
}
void Draw::colorOnBlock(const Point3D& point, const sf::Color color)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale, m_window.m_scale));
	square.setFillColor(color);
	square.setPosition((float)point.x().get() * m_window.m_scale, ((float)point.y().get() * m_window.m_scale));
	m_window.getRenderWindow().draw(square);
}
void Draw::maybeDesignated(const Point3D& point)
{
	SpaceDesignation designation = m_window.m_area->m_spaceDesignations.getForFaction(m_window.m_faction).getDisplayDesignation(point);
	if(designation == SpaceDesignation::SPACE_DESIGNATION_MAX)
		return;
	static std::unordered_map<SpaceDesignation, sf::Sprite> designationSprites{
		{SpaceDesignation::Dig, getCenteredSprite("pick")},
		{SpaceDesignation::Construct, getCenteredSprite("mallet")},
		{SpaceDesignation::FluidSource, getCenteredSprite("bucket")},
		{SpaceDesignation::GivePlantFluid, getCenteredSprite("bucket")},
		{SpaceDesignation::SowSeeds, getCenteredSprite("seed")},
		{SpaceDesignation::Harvest, getCenteredSprite("hand")},
		{SpaceDesignation::Rescue, getCenteredSprite("hand")},
		{SpaceDesignation::Sleep, getCenteredSprite("sleep")},
		{SpaceDesignation::WoodCutting, getCenteredSprite("axe")},
		{SpaceDesignation::StockPileHaulFrom, getCenteredSprite("hand")},
		{SpaceDesignation::StockPileHaulTo, getCenteredSprite("open")}
	};
	spriteOnBlockCentered(point, designationSprites.at(designation), &displayData::selectColor);
}
void Draw::craftLocation(const Point3D& point)
{
	const CraftStepTypeCategoryId& craftDisplay = m_window.m_area->m_hasCraftingLocationsAndJobs.getForFaction(m_window.getFaction()).getDisplayStepTypeCategoryForLocation(point);
	if(craftDisplay.exists())
	{
		static sf::Sprite craftSpot = getCenteredSprite("tool");
		spriteOnBlockCentered(point, craftSpot, &displayData::selectColor);
	}
}
sf::Sprite Draw::getCenteredSprite(std::string name)
{
	auto pair = sprites::make(name);
	pair.first.setOrigin(pair.second);
	return pair.first;
}
// Origin is assumed to already be set in sprite on point.
void Draw::spriteOnBlockWithScaleCentered(const Point3D& point, sf::Sprite& sprite, float scale, const sf::Color* color)
{
	float windowScale = m_window.m_scale;
	scale = scale * (windowScale / (float)displayData::defaultScale);
	sprite.setScale(scale, scale);
	sprite.setPosition(((float)point.x().get() + 0.5f) * windowScale, ((float)point.y().get() + 0.5f) * windowScale);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteOnBlockWithScale(const Point3D& point, sf::Sprite& sprite, float scale, const sf::Color* color)
{
	float windowScale = m_window.m_scale;
	scale = scale * (windowScale / (float)displayData::defaultScale);
	sprite.setScale(scale, scale);
	sprite.setPosition((float)point.x().get() * windowScale, (float)point.y().get() * windowScale);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteAt(sf::Sprite& sprite, sf::Vector2f position, const sf::Color* color)
{
	spriteAtWithScale(sprite, position, 1, color);
}
void Draw::spriteAtWithScale(sf::Sprite& sprite, sf::Vector2f position, float scale, const sf::Color* color)
{
	sprite.setPosition(position);
	float scaleRatio = scale * (float)m_window.m_scale / (float)displayData::defaultScale;
	sprite.setScale(scaleRatio, scaleRatio);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteOnBlock(const Point3D& point, sf::Sprite& sprite, const sf::Color* color)
{
	sf::Vector2f position{(float)(point.x().get()* m_window.m_scale), (float)(point.y().get()* m_window.m_scale)};
	spriteAt(sprite, position, color);
}
void Draw::spriteOnBlockCentered(const Point3D& point, sf::Sprite& sprite, const sf::Color* color)
{
	sf::Vector2f position{(((float)point.x().get() + 0.5f) * (float)m_window.m_scale), (((float)point.y().get() + 0.5f) * (float)m_window.m_scale)};
	spriteAt(sprite, position, color);
}
void Draw::imageOnBlock(const Point3D& point, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	spriteOnBlock(point, pair.first, color);
}
// By default images are top aligned.
void Draw::imageOnBlockWestAlign(const Point3D& point, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	pair.first.setRotation(270);
	spriteOnBlock(point, pair.first, color);
}
void Draw::imageOnBlockEastAlign(const Point3D& point, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	pair.first.setRotation(90);
	spriteOnBlock(point, pair.first, color);
}
void Draw::imageOnBlockSouthAlign(const Point3D& point, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	pair.first.setRotation(180);
	spriteOnBlock(point, pair.first, color);
}
void Draw::progressBarOnBlock(const Point3D& point, Percent progress)
{
	float scaledUnit = getScaledUnit();
	sf::RectangleShape outline(sf::Vector2f(m_window.m_scale, scaledUnit * (2 + displayData::progressBarThickness)));
	outline.setFillColor(displayData::progressBarOutlineColor);
	outline.setPosition((float)point.x().get() * m_window.m_scale, (float)point.y().get() * m_window.m_scale);
	m_window.getRenderWindow().draw(outline);
	float progressWidth = util::scaleByPercent(m_window.m_scale, progress) - (scaledUnit * 2);
	sf::RectangleShape rectangle(sf::Vector2f(progressWidth, scaledUnit * displayData::progressBarThickness));
	rectangle.setFillColor(displayData::progressBarColor);
	rectangle.setPosition(((float)point.x().get() * m_window.m_scale) + scaledUnit, ((float)point.y().get() * m_window.m_scale) + scaledUnit);
	m_window.getRenderWindow().draw(rectangle);
}
void Draw::selected(const Point3D& point) { outlineOnBlock(point, displayData::selectColor); }
void Draw::selected(const Cuboid& cuboid)
{
	// Check if cuboid intersects with current z level
	if(cuboid.m_low.z() > m_window.m_z || cuboid.m_high.z() < m_window.m_z)
		return;
	// Set Dimensions.
	const uint xSize = (cuboid.m_high.x() - cuboid.m_low.x()).get() + 1;
	const uint ySize = (cuboid.m_high.y() - cuboid.m_low.y()).get() + 1;
	sf::RectangleShape rectangle(sf::Vector2f(xSize * m_window.m_scale, ySize * m_window.m_scale));
	// Set Color.
	rectangle.setFillColor(displayData::selectColorOverlay);
	// Set Position.
	rectangle.setPosition(((float)cuboid.m_low.x().get() * m_window.m_scale), ((float)cuboid.m_low.y().get() * m_window.m_scale));
	m_window.getRenderWindow().draw(rectangle);
}
void Draw::outlineOnBlock(const Point3D& point, const sf::Color color, float thickness)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale - (thickness*2), m_window.m_scale - (thickness*2)));
	square.setFillColor(sf::Color::Transparent);
	square.setOutlineColor(color);
	square.setOutlineThickness(thickness);
	square.setPosition(((float)point.x().get() * m_window.m_scale) + thickness, ((float)point.y().get() * m_window.m_scale) + thickness);
	m_window.getRenderWindow().draw(square);
}
void Draw::stringOnBlock(const Point3D& point, std::string string, const sf::Color color, float offsetX, float offsetY )
{
	return stringAtPosition(string, {
		((float)point.x().get() + offsetX) * m_window.m_scale,
		((float)point.y().get() + offsetY) * m_window.m_scale
	}, color);

}
void Draw::stringAtPosition(const std::string string, const sf::Vector2f position, const sf::Color color, float offsetX, float offsetY )
{
	// Don't draw text which would be too small to read comfortably.
	if(m_window.m_scale < displayData::defaultScale)
		return;
	sf::Text text;
	text.setFont(m_window.m_font);
	text.setFillColor(color);
	text.setCharacterSize(m_window.m_scale * displayData::ratioOfScaleToFontSize);
	text.setString(string);
	text.setPosition( position.x + offsetX, position.y + offsetY);
	auto center = text.getGlobalBounds().width / 2;
	auto xOrigin = std::round(center);
	text.setOrigin(xOrigin, 0);
	m_window.getRenderWindow().draw(text);
}
// Plant.
void Draw::nonGroundCoverPlant(const Point3D& point)
{
	Space& space = m_window.m_area->getSpace();
	Plants& plants = m_window.m_area->getPlants();
	if(!space.plant_exists(point))
		return;
	const PlantIndex& plant = space.plant_get(point);
	PlantSpeciesDisplayData& display = displayData::plantData[plants.getSpecies(plant)];
	// Ground cover plants are drawn with floors.
	if(display.groundCover)
		return;
	auto& occupiedBlocks = plants.getOccupied(plant);
	if(PlantSpecies::getIsTree(plants.getSpecies(plant)) && occupiedBlocks.size() != 1)
	{
		if(point == plants.getLocation(plant) && occupiedBlocks.size() > 2)
		{
			static sf::Sprite trunk = getCenteredSprite("trunk");
			spriteOnBlockCentered(point, trunk);
		}
		else
		{
			const Point3D& plantLocation = plants.getLocation(plant);
			if(point.x() == plantLocation.x() && point.y() == plantLocation.y())
			{
				const Point3D& above = point.above();
				if(above.exists() && space.plant_exists(above) && space.plant_get(above) == plant)
				{
					static sf::Sprite trunk = getCenteredSprite("trunkWithBranches");
					spriteOnBlockCentered(point, trunk);
					static sf::Sprite trunkLeaves = getCenteredSprite("trunkLeaves");
					spriteOnBlockCentered(point, trunkLeaves);
				}
				else
				{
					static sf::Sprite treeTop = getCenteredSprite(display.image);
					spriteOnBlockCentered(point, treeTop);
				}
			}
			else
			{
				float angle = 45.f * (uint)plantLocation.getFacingTwordsIncludingDiagonal(point);
				static sf::Sprite branch = getCenteredSprite("branch");
				branch.setRotation(angle);
				spriteOnBlockCentered(point, branch);
				static sf::Sprite branchLeaves = getCenteredSprite("branchLeaves");
				branchLeaves.setRotation(angle);
				spriteOnBlockCentered(point, branchLeaves);
			}
		}
	}
	else
	{
		auto pair = sprites::make(display.image);
		//TODO: color
		pair.first.setOrigin(pair.second);
		float scale = plants.getOccupied(plant).size() == 1 ? ((float)plants.getPercentGrown(plant).get() / 100.f) : 1;
		spriteOnBlockWithScaleCentered(point, pair.first, scale);
		if(m_window.getSelectedPlants().contains(plant))
			outlineOnBlock(point, displayData::selectColor);
	}
}
// Item.
void Draw::item(const Point3D& point)
{
	Space& space = m_window.m_area->getSpace();
	if(space.item_empty(point))
		return;
	const ItemIndex& itemToDraw = space.item_getAll(point).front();
	item(itemToDraw, blockToPositionCentered(point));
}
void Draw::item(const ItemIndex& item, sf::Vector2f position)
{
	Items& items = m_window.m_area->getItems();
	ItemTypeDisplayData& display = displayData::itemData[items.getItemType(item)];
	auto [sprite, origin] = sprites::make(display.image);
	sprite.setOrigin(origin);
	sprite.setColor(display.color);
	const sf::Color materialColor = displayData::materialColors[items.getMaterialType(item)];
	sprite.setColor(materialColor);
	spriteAtWithScale(sprite, position, display.scale);
}
void Draw::itemOverlay(const ItemIndex& item, sf::Vector2f position)
{
	Items& items = m_window.m_area->getItems();
	Quantity quantity = items.getQuantity(item);
	if(quantity != 1)
		stringAtPosition(std::to_string(quantity.get()), position, sf::Color::White);
}
void Draw::actorOverlay(const ActorIndex& actor)
{
	Actors& actors = m_window.m_area->getActors();
	sf::Vector2f location = blockToPosition(actors.getLocation(actor));
	location.y -= (float)m_window.m_scale / 3.5f;
	if(!actors.sleep_isAwake(actor))
	{
		sf::Vector2f sleepTextLocation = location;
		sleepTextLocation.y += m_window.m_scale / 3.5f;
		stringAtPosition("zzz", sleepTextLocation, sf::Color::Cyan);
	}
}
void Draw::itemOverlay(const Point3D& point)
{
	Space& space = m_window.m_area->getSpace();
	Items& items = m_window.m_area->getItems();
	if(space.item_empty(point))
		return;
	const ItemIndex& item = space.item_getAll(point).front();
	if(space.item_getAll(point).size() > 1)
		stringOnBlock(point, std::to_string(space.item_getAll(point).size()), sf::Color::Magenta);
	else if(items.getQuantity(item) != 1)
		stringOnBlock(point, std::to_string(items.getQuantity(item).get()), sf::Color::White);
	if(m_window.getSelectedItems().contains(item))
		outlineOnBlock(point, displayData::selectColor);
	if(m_window.getFaction().exists() && !items.stockpile_canBeStockPiled(item, m_window.getFaction()))
		inaccessableSymbol(point);
}
// Actor.
void Draw::singleTileActor(const ActorIndex& actor)
{
	Actors& actors = m_window.m_area->getActors();
	const Point3D& point = actors.getLocation(actor);
	AnimalSpeciesDisplayData& display = displayData::actorData[actors.getSpecies(actor)];
	auto [sprite, origin] = sprites::make(display.image);
	sprite.setOrigin(origin);
	sprite.setColor(display.color);
	spriteOnBlockWithScaleCentered(point, sprite, ((float)actors.getPercentGrown(actor).get() + 10.f) / 110.f);
	if(actors.canPickUp_exists(actor))
	{
		const ActorOrItemIndex& isCarrying = actors.canPickUp_getPolymorphic(actor);
		if(isCarrying.isItem())
		{
			sf::Vector2f position = blockToPositionCentered(point);
			position.y += getScaledUnit() * 4;
			const ItemIndex& itemToDraw = isCarrying.getItem();
			item(itemToDraw, position);
		}
		else
		{
			assert(isCarrying.isActor());
			// TODO: Draw actors being carried.
		}
	}
	if(m_window.getSelectedActors().contains(actor))
		outlineOnBlock(point, displayData::selectColor);
}
void Draw::multiTileActor(const ActorIndex& actor)
{
	Actors& actors = m_window.m_area->getActors();
	AnimalSpeciesDisplayData& display = displayData::actorData[actors.getSpecies(actor)];
	auto [sprite, origin] = sprites::make(display.image);
	const CuboidSet& occupiedBlocks = actors.getOccupied(actor);
	// TODO: move display scale to display data.
	if(Shape::getDisplayScale(actors.getShape(actor)) == 1)
	{
		const Point3D& actorLocation = actors.getLocation(actor);
		const Point3D location = Point3D(actorLocation.x(), actorLocation.y(), m_window.m_z);
		spriteOnBlock(location, sprite, &display.color);
		multiTileBorder(occupiedBlocks, displayData::actorOutlineColor, 1);
	}
	else
	{
		Point3D topLeft;
		for(const Cuboid& cuboid : occupiedBlocks)
			for(const Point3D& point : cuboid)
			{
				if(point.z() == m_window.m_z)
				{
					if(topLeft.empty())
						topLeft = point;
					else
					{
						if(point.x() < topLeft.x() || point.y() < topLeft.y())
							topLeft = point;
					}
				}
			}
		spriteOnBlockWithScale(topLeft, sprite, Shape::getDisplayScale(actors.getShape(actor)), &display.color);
	}
}
void Draw::multiTileBorder(const CuboidSet& blocksOccpuied, sf::Color color, float thickness)
{
	for(const Cuboid& cuboid : blocksOccpuied)
		for(const Point3D& point : cuboid)
		{
			for(const Point3D& adjacent : point.getAllAdjacentIncludingOutOfBounds().sliceAtZ(point.z()))
				if(!blocksOccpuied.contains(adjacent))
				{
					Facing4 facing = point.getFacingTwords(adjacent);
					borderSegmentOnBlock(point, facing, color, thickness);
				}
		}
}
void Draw::borderSegmentOnBlock(const Point3D& point, const Facing4& facing, sf::Color color, float thickness)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale, thickness));
	square.setFillColor(color);
	switch((uint)facing)
	{
		case 0:
			// do nothing
			square.setPosition(
				(float)point.x().get() * m_window.m_scale,
				(float)point.y().get() * m_window.m_scale
			);
			break;
		case 1:
			square.setRotation(90);
			square.setPosition(
				(((float)point.x().get() + 1.f) * m_window.m_scale) - thickness,
				(float)(point.y().get() * m_window.m_scale)
			);
		break;
		case 2:
			square.setPosition(
				(float)point.x().get() * m_window.m_scale,
				(((float)point.y().get() + 1) * m_window.m_scale) - thickness
			);
			break;

		case 3:
			square.setRotation(90);
			square.setPosition(
				(float)point.x().get() * m_window.m_scale,
				(float)point.y().get() * m_window.m_scale
			);
			break;
	}
	m_window.getRenderWindow().draw(square);
}
Facing4 Draw::rampOrStairsFacing(const Point3D& point) const
{
	Space& space = m_window.m_area->getSpace();
	static auto canConnectToAbove = [&](const Point3D& otherPoint) -> bool {
		const Point3D& above = otherPoint.above();
		return above.exists() && !space.pointFeature_contains(otherPoint, PointFeatureTypeId::Stairs) && !space.pointFeature_contains(otherPoint, PointFeatureTypeId::Ramp) && space.shape_canStandIn(above);
	};
	const Point3D& above = point.above();
	if(above.empty())
		return Facing4::North;
	Facing4 backup = Facing4::North;
	const Point3D& north = point.north();
	const Point3D& south = point.south();
	if(north.exists() && canConnectToAbove(north))
	{
		if(!space.solid_isAny(south) && space.shape_canStandIn(south) &&
			!space.pointFeature_contains(south, PointFeatureTypeId::Stairs) &&
			!space.pointFeature_contains(south, PointFeatureTypeId::Stairs)
		)
			return Facing4::North;
	}
	const Point3D& east = point.east();
	const Point3D& west = point.west();
	if(east.exists() && canConnectToAbove(east))
	{
		if(!space.solid_isAny(west) && space.shape_canStandIn(west))
			return Facing4::East;
		else
			backup = Facing4::East;
	}
	if(south.exists() && canConnectToAbove(south))
	{
		if(!space.solid_isAny(north) && space.shape_canStandIn(north) &&
			!space.pointFeature_contains(north, PointFeatureTypeId::Stairs) &&
			!space.pointFeature_contains(north, PointFeatureTypeId::Stairs)
		)
			return Facing4::South;
		else
			backup = Facing4::South;
	}
	if(west.exists() && canConnectToAbove(west))
	{
		if(!space.solid_isAny(east) && space.shape_canStandIn(point))
			return Facing4::West;
		else
			backup = Facing4::West;
	}
	return backup;
}
sf::Vector2f Draw::blockToPosition(const Point3D& point) const
{
	return {((float)point.x().get() * m_window.m_scale), ((float)point.y().get() * m_window.m_scale)};
}
sf::Vector2f Draw::blockToPositionCentered(const Point3D& point) const
{
	return {((float)point.x().get() + 0.5f) * m_window.m_scale, ((float)point.y().get() + 0.5f) * m_window.m_scale};
}
void Draw::accessableSymbol(const Point3D& point)
{
	stringOnBlock(point, "o", sf::Color::Green, 0.1, 0.6);
}
void Draw::inaccessableSymbol(const Point3D& point)
{
	stringOnBlock(point, "x", sf::Color::Red, 0.1, 0.6);
}
float Draw::getScaledUnit() const { return float(m_window.m_scale) / float(displayData::defaultScale); }
