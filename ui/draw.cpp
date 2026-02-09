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
#include "../engine/pointFeature.h"
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
	// Collect actors into single and multi tile groups.
	SmallSet<Point3D> singleTileActorBlocks;
	SmallSet<ActorIndex> multiTileActors;
	const Cuboid zLevelBlocks = space.getZLevel(m_window.m_z);
	space.actor_queryForEach(zLevelBlocks, [&](const ActorIndex& actor) {
		if(Shape::getIsMultiTile(actors.getShape(actor)))
			multiTileActors.maybeInsert(actor);
		else
			singleTileActorBlocks.insert(actors.getLocation(actor));
	});
	if(m_window.m_z != 0)
	{
		// Render solid blocks on next level down as floors.
		Cuboid belowLevel(Point3D(space.m_sizeX - 1, space.m_sizeY - 1, m_window.m_z - 1), Point3D::create(0, 0, m_window.m_z.get() - 1));
		space.solid_queryForEachWithCuboids(belowLevel, [&](const Cuboid& cuboid, const MaterialTypeId& material){
			const sf::Color* color = &displayData::materialColors[material];
			CuboidSet constructed = space.queryConstructedGetCuboids(cuboid);
			for(const Cuboid& constructedCuboid : constructed)
			{
				sf::Sprite constructedFloor = sprites::makeRepeated("blockFloor", constructedCuboid);
				spriteFill(constructedFloor, constructedCuboid.intersection(constructedCuboid), color);
				blockWalls(constructedCuboid, material, true, false);
			}
			// NotConstructed isn't neccesary, we could remove each constructed from notOccluded and use the remainder instead.
			CuboidSet notConstructed = CuboidSet::create(cuboid);
			notConstructed.maybeRemoveAll(constructed);
			for(const Cuboid& notConstructedCuboid : notConstructed)
			{
				sf::Sprite notConstructedFloor = sprites::makeRepeated("roughFloor", notConstructedCuboid);
				spriteFill(notConstructedFloor, notConstructedCuboid.intersection(notConstructedCuboid), color);
				blockWalls(notConstructedCuboid, material, false, false);
			}
		});
		// Fluids on next level down.
		space.fluid_queryForEachWithCuboids(belowLevel, [&](const Cuboid& cuboid, const FluidData& data){
			// TODO: Give the shore line some feeling of depth somehow.
			std::string volumeString = std::to_string(data.volume.get());
			for(const Point3D& point : cuboid)
				stringOnBlock(point, volumeString, sf::Color::Black);
			static sf::Sprite sprite = sprites::makeRepeated("fluidSurface", cuboid);
			const sf::Color color = displayData::fluidColors[data.type];
			spriteFill(sprite, cuboid, &color);
		});
	}
	// Ground cover plants.
	space.plant_queryForEachWithCuboids(zLevelBlocks, [&](const Cuboid& cuboid, const PlantIndex& plant){
		const PlantSpeciesId& species = plants.getSpecies(plant);
		const PlantSpeciesDisplayData& display = displayData::plantData[species];
		if(display.groundCover)
		{
			//TODO: color.
			assert(cuboid.volume() == 1);
			const Point3D& location = cuboid.m_high;
			imageOnBlock(location, display.image);
			if(m_window.getSelectedPlants().contains(plant))
				outlineOnBlock(location, displayData::selectColor);
			return;
		}
	});
	// Floor type features.
	SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>> featuresOnThisZLevel;
	space.pointFeature_queryForEachWithCuboids(zLevelBlocks, [&](const Cuboid& cuboid, const PointFeature& pointFeature){
		featuresOnThisZLevel.getOrCreate(pointFeature.pointFeatureType).getOrCreate(pointFeature).add(cuboid);
	});
	static const sf::Texture blockFloor = sprites::textures["blockFloor"].first;
	featureType(blockFloor, PointFeatureTypeId::Floor, featuresOnThisZLevel);
	static const sf::Texture hatch = sprites::textures["hatch"].first;
	featureType(hatch, PointFeatureTypeId::Hatch, featuresOnThisZLevel);
	static const sf::Texture floorGrate = sprites::textures["floorGrate"].first;
	featureType(floorGrate, PointFeatureTypeId::FloorGrate, featuresOnThisZLevel);
	// Stockpiles.
	space.stockpile_queryForEachCuboidForFaction(zLevelBlocks, m_window.getFaction(), [&](const Cuboid& cuboid){
		colorOnCuboid(cuboid, displayData::stockPileColor);
	});
	// Farm fields.
	space.farm_queryForEachCuboidForFaction(zLevelBlocks, m_window.getFaction(), [&](const Cuboid& cuboid){
		colorOnCuboid(cuboid, displayData::farmFieldColor);
	});
	// Fluids on current level.
	space.fluid_queryForEachWithCuboids(zLevelBlocks, [&](const Cuboid& cuboid, const FluidData& data){
		// TODO: Give the shore line some feeling of depth somehow.
		std::string volumeString = std::to_string(data.volume.get());
		for(const Point3D& point : cuboid)
			stringOnBlock(point, volumeString, sf::Color::Black);
		const sf::Color color = displayData::fluidColors[data.type];
		colorOnCuboid(cuboid, color);
	});
	// Render Solid walls.
	space.solid_queryForEachWithCuboids(zLevelBlocks, [&](const Cuboid& cuboid, const MaterialTypeId& material) {
		colorOnCuboid(cuboid, displayData::materialColors[material]);
		CuboidSet notOccluded = CuboidSet::create(cuboid);
		CuboidSet constructed = space.queryConstructedGetCuboids(notOccluded);
		for(const Cuboid& constructedCuboid : constructed)
			blockWalls(constructedCuboid, material, true, true);
		CuboidSet notConstructed = std::move(notOccluded);
		notConstructed.maybeRemoveAll(constructed);
		for(const Cuboid& notConstructedCuboid : notConstructed)
			blockWalls(notConstructedCuboid, material, false, true);
	});
	// Nonfloor type features.
	auto foundStairs = featuresOnThisZLevel.find(PointFeatureTypeId::Stairs);
	if(foundStairs != featuresOnThisZLevel.end())
	{
		for(const auto& [feature, cuboids] : foundStairs->second)
		{
			const sf::Color color = displayData::materialColors[feature.materialType];
			for(const Cuboid& cuboid : cuboids)
				for(const Point3D& point : cuboid)
				{
					static sf::Sprite stairs = getCenteredSprite("stairs");
					Facing4 facing = rampOrStairsFacing(point);
					stairs.setRotation((uint)facing * 90);
					stairs.setOrigin(16,19);
					spriteOnBlockCentered(point, stairs, &color);
				}
			}
	}
	auto foundRamp = featuresOnThisZLevel.find(PointFeatureTypeId::Ramp);
	if(foundRamp != featuresOnThisZLevel.end())
	{
		for(const auto& [feature, cuboids] : foundRamp->second)
		{
			const sf::Color color = displayData::materialColors[feature.materialType];
			for(const Cuboid& cuboid : cuboids)
				for(const Point3D& point : cuboid)
				{
					static sf::Sprite ramp = getCenteredSprite("ramp");
					Facing4 facing = rampOrStairsFacing(point);
					ramp.setRotation((uint)facing * 90);
					ramp.setOrigin(16,19);
					spriteOnBlockCentered(point, ramp, &color);
				}
		}
	}
	auto foundFloodGate = featuresOnThisZLevel.find(PointFeatureTypeId::FloodGate);
	if(foundFloodGate != featuresOnThisZLevel.end())
	{
		for(const auto& [feature, cuboids] : foundFloodGate->second)
		{
			const sf::Color color = displayData::materialColors[feature.materialType];
			for(const Cuboid& cuboid : cuboids)
				for(const Point3D& point : cuboid)
				{
					static sf::Sprite floodGate = getCenteredSprite("floodGate");
					if(point.y() == space.m_sizeY - 1 || point.x() == 0 || (!space.solid_isAny(point.north()) || !space.solid_isAny(point.south())))
						floodGate.setRotation(0);
					else
						floodGate.setRotation(90);
					spriteOnBlockCentered(point, floodGate, &color);
				}
		}
	}
	auto foundFortification = featuresOnThisZLevel.find(PointFeatureTypeId::Fortification);
	if(foundFortification != featuresOnThisZLevel.end())
	{
		for(const auto& [feature, cuboids] : foundFortification->second)
		{
			const sf::Color color = displayData::materialColors[feature.materialType];
			for(const Cuboid& cuboid : cuboids)
				for(const Point3D& point : cuboid)
				{
					static sf::Sprite fortification = getCenteredSprite("fortification");
					if(point.y() == space.m_sizeY - 1 || point.x() == 0 || (!space.solid_isAny(point.north()) || !space.solid_isAny(point.south())))
						fortification.setRotation(0);
					else
						fortification.setRotation(90);
					spriteOnBlockCentered(point, fortification, &color);
				}
		}
	}
	auto foundFlap = featuresOnThisZLevel.find(PointFeatureTypeId::Flap);
	if(foundFlap != featuresOnThisZLevel.end())
	{
		for(const auto& [feature, cuboids] : foundFlap->second)
		{
			const sf::Color color = displayData::materialColors[feature.materialType];
			for(const Cuboid& cuboid : cuboids)
				for(const Point3D& point : cuboid)
				{
					static sf::Sprite flap = getCenteredSprite("flap");
					if(point.y() == space.m_sizeY - 1 || point.x() == 0 || (!space.solid_isAny(point.north()) || !space.solid_isAny(point.south())))
						flap.setRotation(0);
					else
						flap.setRotation(90);
					spriteOnBlockCentered(point, flap, &color);
				}
		}
	}
	auto foundDoor = featuresOnThisZLevel.find(PointFeatureTypeId::Door);
	if(foundDoor != featuresOnThisZLevel.end())
	{
		for(const auto& [feature, cuboids] : foundDoor->second)
		{
			const sf::Color color = displayData::materialColors[feature.materialType];
			for(const Cuboid& cuboid : cuboids)
				for(const Point3D& point : cuboid)
				{
					static sf::Sprite door = getCenteredSprite("door");
					if(point.y() == space.m_sizeY - 1 || point.x() == 0 || (!space.solid_isAny(point.north()) || !space.solid_isAny(point.south())))
						door.setRotation(0);
					else
						door.setRotation(90);
					spriteOnBlockCentered(point, door, &color);
				}
		}
	}
	if(m_window.m_z != 0)
	{
		// Render stairs and ramp tops for next level down.
		Cuboid belowLevel(Point3D(space.m_sizeX - 1, space.m_sizeY - 1, m_window.m_z - 1), Point3D::create(0, 0, m_window.m_z.get() - 1));
		space.pointFeature_queryForEachWithCuboids(belowLevel, [&](const Cuboid& cuboid, const PointFeature& pointFeature){
			sf::Color* color = &displayData::materialColors[pointFeature.materialType];
			if(pointFeature.pointFeatureType == PointFeatureTypeId::Stairs)
			{
				for(const Point3D& point : cuboid)
				{
					Facing4 facing = rampOrStairsFacing(point);
					static sf::Sprite stairs = getCenteredSprite("stairs");
					stairs.setRotation((uint)facing * 90);
					stairs.setOrigin(16,19);
					stairs.setTextureRect({0,0,32,16});
					spriteOnBlockCentered(point, stairs, color);
				}
			}
			if(pointFeature.pointFeatureType == PointFeatureTypeId::Ramp)
			{
				for(const Point3D& point : cuboid)
				{
					Facing4 facing = rampOrStairsFacing(point);
					static sf::Sprite ramp = getCenteredSprite("ramp");
					ramp.setRotation((uint)facing * 90);
					ramp.setOrigin(16,19);
					ramp.setTextureRect({0,0,32,16});
					spriteOnBlockCentered(point, ramp, color);
				}
			}
		});
	}
	// Non-Ground cover plants.
	space.plant_queryForEachWithCuboids(zLevelBlocks, [&](const Cuboid& cuboid, const PlantIndex& plant){
		const PlantSpeciesId& species = plants.getSpecies(plant);
		const PlantSpeciesDisplayData& display = displayData::plantData[species];
		if(!display.groundCover)
		{
			//TODO: color.
			assert(cuboid.volume() == 1);
			const Point3D& location = cuboid.m_high;
			imageOnBlock(location, display.image);
			if(m_window.getSelectedPlants().contains(plant))
				outlineOnBlock(location, displayData::selectColor);
			return;
		}
	});
	// Render items.
	space.item_queryForEachCuboid(zLevelBlocks, [&](const Cuboid& cuboid){
		for(const Point3D& point : cuboid)
			item(point);
	});
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
		m_window.getArea()->m_spaceDesignations.queryForEachForFactionIfExists(m_window.m_faction, zLevelBlocks, [&](const Cuboid& cuboid){
			//TODO: This is slower then it should be.
			for(const Point3D& point : cuboid)
				maybeDesignated(point);
		});
		m_window.m_area->m_hasCraftingLocationsAndJobs.getForFaction(m_window.m_faction).forEachPoint([&](const Point3D& point){
			static sf::Sprite craftSpot = getCenteredSprite("tool");
			spriteOnBlockCentered(point, craftSpot, &displayData::selectColor);
		});
		space.project_queryForEachWithLocation(m_window.m_faction, zLevelBlocks, [&](const Project& project, const Point3D& location){
			Percent progress = project.getPercentComplete();
			if(progress != 0)
				progressBarOnBlock(location, progress);
		});
	}
	// Render item overlays.
	space.item_queryForEachCuboid(zLevelBlocks, [&](const Cuboid& cuboid){
		for(const Point3D& point : cuboid)
			itemOverlay(point);
	});
	// Render actor overlays.
	for(const Point3D& point : singleTileActorBlocks)
		actorOverlay(space.actor_getAll(point).front());

	// Selected.
	if(!m_window.m_selectedBlocks.empty())
		colorOnCuboids(m_window.m_selectedBlocks.getCuboids().getVector(), displayData::selectColorOverlay);
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

		uint32_t xSize = std::abs(start.x - end.x);
		uint32_t ySize = std::abs(start.y - end.y);
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
	sf::RectangleShape areaBorder(sf::Vector2f((space.m_sizeX.get() * displayData::defaultScale), (space.m_sizeY.get() * displayData::defaultScale)));
	areaBorder.setOutlineColor(sf::Color::White);
	areaBorder.setFillColor(sf::Color::Transparent);
	areaBorder.setOutlineThickness(3.f);
	areaBorder.setPosition(sf::Vector2f(0,0));
	m_window.m_window.draw(areaBorder);
	// Update Info popup.
	//if(m_gameOverlay.infoPopupIsVisible())
		//m_gameOverlay.updateInfoPopup();
}
void Draw::featureType(const sf::Texture& texture, const PointFeatureTypeId& featureTypeId, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features)
{
	auto found = features.find(featureTypeId);
	if(found != features.end())
	{
		for(const auto& [pointFeature, cuboids] : found->second)
		{
			const sf::Color color = displayData::materialColors[pointFeature.materialType];
			for(const Cuboid& cuboid : cuboids)
				textureFill(texture, cuboid, &color);
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
		auto pair = sprites::make(space.isConstructed(point) ? "blockWall" : "roughWall");
		auto sprite = pair.first;
		sprite.setPosition(((float)point.x().get() - 0.21f) * (float)displayData::defaultScale, ((float)m_window.invertY(point.y().get()) + 0.48f) * (float)displayData::defaultScale);
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
			auto pair = sprites::make(space.isConstructed(point) ? "blockWall" : "roughWall");
			auto sprite = pair.first;
			sprite.setPosition(((float)point.x().get() - 0.21f) * (float)displayData::defaultScale, ((float)m_window.invertY(point.y().get()) + 0.48f) * (float)displayData::defaultScale);
			sprite.setColor(color);
			sprite.setRotation(45);
			m_window.getRenderWindow().draw(sprite);
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
void Draw::blockWalls(const Cuboid& cuboid, const MaterialTypeId& materialType, const bool& constructed, const bool& drawTops)
{
	const Space& space = m_window.getArea()->getSpace();
	const sf::Color color = displayData::materialColors[materialType];
	const Point3D southWest{cuboid.m_low.x(), cuboid.m_low.y(), m_window.m_z};
	blockWallCorners(southWest);
	static const sf::Texture constructedWallTextureSouthTop = sprites::textures["blockWallTop"].first;
	static const sf::Texture constructedWallTextureWestTop = sprites::makeRotatedTexture("blockWallTop", 90);
	static const sf::Texture constructedWallTextureNorthTop = sprites::makeRotatedTexture("blockWallTop", 180);
	static const sf::Texture constructedWallTextureEastTop = sprites::makeRotatedTexture("blockWallTop", 270);
	static const sf::Texture roughWallTextureSouthTop = sprites::textures["roughWallTop"].first;
	static const sf::Texture roughWallTextureWestTop = sprites::makeRotatedTexture("roughWallTop", 90);
	static const sf::Texture roughWallTextureNorthTop = sprites::makeRotatedTexture("roughWallTop", 180);
	static const sf::Texture roughWallTextureEastTop = sprites::makeRotatedTexture("roughWallTop", 270);
	if(cuboid.m_low.y() != 0)
	{
		static const sf::Texture constructedWallTextureSouth = sprites::textures["blockWall"].first;
		static const sf::Texture roughWallTextureSouth = sprites::textures["roughWall"].first;
		const sf::Texture& textureSouth = constructed ? constructedWallTextureSouth : roughWallTextureSouth;
		Cuboid southFace = cuboid.getFace(Facing6::South);
		southFace.shift(Facing6::South, {1});
		CuboidSet south = CuboidSet::create(southFace);
		space.solid_removeOpaque(south);
		const int height = textureSouth.getSize().y;
		for(const Cuboid& southCuboid : south)
		{
			sf::Sprite sprite(textureSouth);
			const float x = southCuboid.m_low.x().get() * displayData::defaultScale;
			const float y = ((float)m_window.invertY(southCuboid.m_high.y().get()) - 0.01f) * displayData::defaultScale;
			sprite.setPosition(x, y);
			sf::IntRect rect{0, 0, southCuboid.sizeX().get() * displayData::defaultScale, height};
			sprite.setTextureRect(rect);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
			if(drawTops)
			{
				const sf::Texture& textureSouthTop = constructed ? constructedWallTextureSouthTop : roughWallTextureSouthTop;
				sf::Sprite topSprite(textureSouthTop);
				const int topHeight = textureSouthTop.getSize().y;
				// render wall tops directly over their solid blocks, not in the adjacent blocks like walls.
				const float topY = ((float)m_window.invertY(cuboid.m_low.y().get()) + 0.85f) * displayData::defaultScale;
				topSprite.setPosition(x, topY);
				topSprite.setTextureRect({0, 0, southCuboid.sizeX().get() * displayData::defaultScale, topHeight});
				topSprite.setColor(color);
				m_window.getRenderWindow().draw(topSprite);
			}
		}
	}
	if(cuboid.m_low.x() != 0)
	{
		static const sf::Texture constructedWallTextureWest = sprites::makeRotatedTexture("blockWall", 90);
		static const sf::Texture roughWallTextureWest = sprites::makeRotatedTexture("roughWall", 90);
		const sf::Texture& textureWest = constructed ? constructedWallTextureWest : roughWallTextureWest;
		Cuboid westFace = cuboid.getFace(Facing6::West);
		westFace.shift(Facing6::West, {1});
		CuboidSet west = CuboidSet::create(westFace);
		space.solid_removeOpaque(west);
		const int width = textureWest.getSize().x;
		for(const Cuboid& westCuboid : west)
		{
			sf::Sprite sprite(textureWest);
			const float x = (((float)westCuboid.m_low.x().get() + 0.25f) * (float)displayData::defaultScale);
			const float y = m_window.invertY(westCuboid.m_high.y().get()) * displayData::defaultScale;
			// TODO: 0.22 is added here because the rotated block wall images are centered rather then aligned to the right.
			sprite.setPosition(x, y);
			sf::IntRect rect{0, 0, width, westCuboid.sizeY().get() * displayData::defaultScale};
			sprite.setTextureRect(rect);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
			if(drawTops)
			{
				const sf::Texture& textureWestTop = constructed ? constructedWallTextureWestTop : roughWallTextureWestTop;
				sf::Sprite topSprite(textureWestTop);
				const int topWidth = textureWestTop.getSize().x;
				// render wall tops directly over their solid blocks, not in the adjacent blocks like walls.
				const float topX = ((float)westCuboid.m_low.x().get() + 0.52f) * displayData::defaultScale;
				topSprite.setPosition(topX, y);
				topSprite.setTextureRect({0, 0, topWidth, westCuboid.sizeY().get() * displayData::defaultScale});
				topSprite.setColor(color);
				m_window.getRenderWindow().draw(topSprite);
			}
		}
	}
	if(drawTops)
	{
		const Cuboid spaceBoundry = m_window.getArea()->getSpace().boundry();
		if(cuboid.m_high.y() != spaceBoundry.m_high.y())
		{
			Cuboid northFace = cuboid.getFace(Facing6::North);
			northFace.shift(Facing6::North, {1});
			CuboidSet north = CuboidSet::create(northFace);
			space.solid_removeOpaque(north);
			for(const Cuboid& northCuboid : north)
			{
				const sf::Texture& textureNorthTop = constructed ? constructedWallTextureNorthTop : roughWallTextureNorthTop;
				sf::Sprite topSprite(textureNorthTop);
				const int topHeight = textureNorthTop.getSize().y;
				// render wall tops directly over their solid blocks, not in the adjacent blocks like walls.
				const float x = (float)northCuboid.m_low.x().get() * displayData::defaultScale;
				const float y = ((float)m_window.invertY(northCuboid.m_high.y().get()) + 0.9f) * displayData::defaultScale;
				topSprite.setPosition(x, y);
				topSprite.setTextureRect({0, 0, northCuboid.sizeX().get() * displayData::defaultScale, topHeight});
				topSprite.setColor(color);
				m_window.getRenderWindow().draw(topSprite);
			}
		}
		if(cuboid.m_high.x() != spaceBoundry.m_high.x())
		{
			Cuboid eastFace = cuboid.getFace(Facing6::East);
			eastFace.shift(Facing6::East, {1});
			CuboidSet east = CuboidSet::create(eastFace);
			space.solid_removeOpaque(east);
			for(const Cuboid& eastCuboid : east)
			{
				const sf::Texture& textureEastTop = constructed ? constructedWallTextureEastTop : roughWallTextureEastTop;
				sf::Sprite topSprite(textureEastTop);
				const int topWidth = textureEastTop.getSize().x;
				// render wall tops directly over their solid blocks, not in the adjacent blocks like walls.
				const float x = ((float)eastCuboid.m_high.x().get() - 0.6f) * displayData::defaultScale;
				const float y = m_window.invertY(eastCuboid.m_high.y().get()) * displayData::defaultScale;
				topSprite.setPosition(x, y);
				topSprite.setTextureRect({0, 0, topWidth, eastCuboid.sizeY().get() * displayData::defaultScale});
				topSprite.setColor(color);
				m_window.getRenderWindow().draw(topSprite);
			}
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
void Draw::colorOnCuboid(const Cuboid& cuboid, const sf::Color& color)
{
	sf::RectangleShape toDraw(sf::Vector2f(displayData::defaultScale * cuboid.sizeX().get(), displayData::defaultScale * (float)cuboid.sizeY().get()));
	toDraw.setFillColor(color);
	// Swap low y and high y to invert draw direction.
	toDraw.setPosition((float)cuboid.m_low.x().get() * displayData::defaultScale, (float)m_window.invertY(cuboid.m_high.y().get()) * displayData::defaultScale);
	m_window.getRenderWindow().draw(toDraw);
}
void Draw::colorOnCuboids(const std::vector<Cuboid>& cuboids, const sf::Color& color)
{
	for(const Cuboid& cuboid : cuboids)
		colorOnCuboid(cuboid, color);
}
void Draw::colorOnBlock(const Point3D& point, const sf::Color& color)
{
	sf::RectangleShape square(sf::Vector2f(displayData::defaultScale, displayData::defaultScale));
	square.setFillColor(color);
	square.setPosition((float)point.x().get() * displayData::defaultScale, ((float)m_window.invertY(point.y().get()) * displayData::defaultScale));
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
sf::Sprite Draw::getCenteredSprite(std::string name)
{
	auto pair = sprites::make(name);
	pair.first.setOrigin(pair.second);
	return pair.first;
}
// Origin is assumed to already be set in sprite on point.
void Draw::spriteOnBlockWithScaleCentered(const Point3D& point, sf::Sprite& sprite, float scale, const sf::Color* color)
{
	float windowScale = displayData::defaultScale;
	scale = scale * (windowScale / (float)displayData::defaultScale);
	sprite.setScale(scale, scale);
	sprite.setPosition(((float)point.x().get() + 0.5f) * windowScale, ((float)m_window.invertY(point.y().get()) + 0.5f) * windowScale);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteOnBlockWithScale(const Point3D& point, sf::Sprite& sprite, float scale, const sf::Color* color)
{
	float windowScale = displayData::defaultScale;
	scale = scale * (windowScale / (float)displayData::defaultScale);
	sprite.setScale(scale, scale);
	sprite.setPosition((float)point.x().get() * windowScale, (float)m_window.invertY(point.y().get()) * windowScale);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteAt(sf::Sprite& sprite, sf::Vector2f position, const sf::Color* color)
{
	spriteAtWithScale(sprite, position, 1, color);
}
void Draw::spriteFill(sf::Sprite& sprite, const Cuboid& cuboid, const sf::Color* color)
{
	spriteFillWithScale(sprite, cuboid, 1.f, color);
}
void Draw::spriteFillWithScale(sf::Sprite& sprite, const Cuboid& cuboid, float scale, const sf::Color* color)
{
	// Invert low and high due to inverted Y axis.
	const float& windowScale = displayData::defaultScale;
	const int worldWidthPx  = (int)(cuboid.sizeX().get()  * windowScale * scale);
	const int worldHeightPx = (int)(cuboid.sizeY().get() * windowScale * scale);
	assert(worldWidthPx > 0 && worldHeightPx > 0);
	const float x = cuboid.m_low.x().get() * windowScale;
	const float y = m_window.invertY(cuboid.m_high.y().get()) * windowScale;
	assert(x >= 0 && y >= 0);
	sprite.setPosition(x, y);
	sprite.setTextureRect({0, 0, std::max(1, worldWidthPx), std::max(1, worldHeightPx)});
	if(color)
		sprite.setColor(*color);
	assert(sprite.getTexture()->isRepeated());
	m_window.getRenderWindow().draw(sprite);
}
void Draw::textureFill(const sf::Texture& texture, const Cuboid& cuboid, const sf::Color* color)
{
	assert(texture.isRepeated());
	sf::Sprite sprite(texture);
	spriteFill(sprite, cuboid, color);
}
void Draw::spriteFillCentered(sf::Sprite& sprite, const Cuboid& cuboid, const sf::Color* color)
{
	spriteFillCenteredWithScale(sprite, cuboid, 1.f, color);
}
void Draw::spriteFillCenteredWithScale(sf::Sprite& sprite, const Cuboid& cuboid, float scale, const sf::Color* color)
{
	// Invert low and high due to inverted Y axis.
	const float& windowScale = displayData::defaultScale;
	const int worldWidthPx  = (int)(cuboid.sizeX().get()  * windowScale * scale);
	const int worldHeightPx = (int)(cuboid.sizeY().get() * windowScale * scale);
	assert(worldWidthPx > 0 && worldHeightPx > 0);
	const float x = (cuboid.m_low.x().get() * windowScale) + worldWidthPx / 2;
	const float y = (m_window.invertY(cuboid.m_high.y().get()) * windowScale) + worldHeightPx / 2;
	assert(x >= 0 && y >= 0);
	sprite.setPosition(x, y);
	sprite.setTextureRect({0, 0, std::max(1, worldWidthPx), std::max(1, worldHeightPx)});
	sprite.setScale(1.f, 1.f);
	if(color)
		sprite.setColor(*color);
	assert(sprite.getTexture()->isRepeated());
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteAtWithScale(sf::Sprite& sprite, sf::Vector2f position, float scale, const sf::Color* color)
{
	sprite.setPosition(position.x, position.y);
	sprite.setScale(scale, scale);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteOnBlock(const Point3D& point, sf::Sprite& sprite, const sf::Color* color)
{
	sf::Vector2f position{(float)(point.x().get()* displayData::defaultScale), (float)(m_window.invertY(point.y().get()) * displayData::defaultScale)};
	spriteAt(sprite, position, color);
}
void Draw::spriteOnBlockCentered(const Point3D& point, sf::Sprite& sprite, const sf::Color* color)
{
	sf::Vector2f position{(((float)point.x().get() + 0.5f) * (float)displayData::defaultScale), (((float)m_window.invertY(point.y().get()) + 0.5f) * (float)displayData::defaultScale)};
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
	sf::RectangleShape outline(sf::Vector2f(displayData::defaultScale, (2 + displayData::progressBarThickness)));
	outline.setFillColor(displayData::progressBarOutlineColor);
	outline.setPosition((float)point.x().get() * displayData::defaultScale, (float)m_window.invertY(point.y().get()) * displayData::defaultScale);
	m_window.getRenderWindow().draw(outline);
	float progressWidth = util::scaleByPercent(displayData::defaultScale, progress) - 2.f;
	sf::RectangleShape rectangle(sf::Vector2f(progressWidth, displayData::progressBarThickness));
	rectangle.setFillColor(displayData::progressBarColor);
	rectangle.setPosition(((float)point.x().get() * displayData::defaultScale), ((float)m_window.invertY(point.y().get()) * displayData::defaultScale));
	m_window.getRenderWindow().draw(rectangle);
}
void Draw::selected(const Point3D& point) { outlineOnBlock(point, displayData::selectColor); }
void Draw::outlineOnBlock(const Point3D& point, const sf::Color color, float thickness)
{
	sf::RectangleShape square(sf::Vector2f(displayData::defaultScale - (thickness*2), displayData::defaultScale - (thickness*2)));
	square.setFillColor(sf::Color::Transparent);
	square.setOutlineColor(color);
	square.setOutlineThickness(thickness);
	square.setPosition(((float)point.x().get() * displayData::defaultScale) + thickness, ((float)m_window.invertY(point.y().get()) * displayData::defaultScale) + thickness);
	m_window.getRenderWindow().draw(square);
}
void Draw::stringOnBlock(const Point3D& point, std::string string, const sf::Color color, float offsetX, float offsetY )
{
	return stringAtPosition(string, {
		((float)point.x().get() + offsetX) * displayData::defaultScale,
		((float)m_window.invertY(point.y().get()) + offsetY) * displayData::defaultScale
	}, color);

}
void Draw::stringAtPosition(const std::string string, const sf::Vector2f position, const sf::Color color, float offsetX, float offsetY )
{
	// Don't draw text which would be too small to read comfortably.
	if(displayData::defaultScale < displayData::defaultScale)
		return;
	sf::Text text;
	text.setFont(m_window.m_font);
	text.setFillColor(color);
	text.setCharacterSize(displayData::defaultScale * displayData::ratioOfScaleToFontSize);
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
	location.y -= (float)displayData::defaultScale / 3.5f;
	if(!actors.sleep_isAwake(actor))
	{
		sf::Vector2f sleepTextLocation = location;
		sleepTextLocation.y += displayData::defaultScale / 3.5f;
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
			position.y += 4;
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
	sf::RectangleShape square(sf::Vector2f(displayData::defaultScale, thickness));
	square.setFillColor(color);
	switch((uint)facing)
	{
		case 0:
			// do nothing
			square.setPosition(
				(float)point.x().get() * displayData::defaultScale,
				(float)m_window.invertY(point.y().get()) * displayData::defaultScale
			);
			break;
		case 1:
			square.setRotation(90);
			square.setPosition(
				(((float)point.x().get() + 1.f) * displayData::defaultScale) - thickness,
				(float)m_window.invertY(point.y().get() * displayData::defaultScale)
			);
		break;
		case 2:
			square.setPosition(
				(float)point.x().get() * displayData::defaultScale,
				(((float)m_window.invertY(point.y().get()) + 1) * displayData::defaultScale) - thickness
			);
			break;

		case 3:
			square.setRotation(90);
			square.setPosition(
				(float)point.x().get() * displayData::defaultScale,
				(float)m_window.invertY(point.y().get()) * displayData::defaultScale
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
	const Cuboid boundry = space.boundry();
	if(point.y() != boundry.m_high.y() && canConnectToAbove(point.north()))
	{
		const Point3D& south = point.south();
		if(!space.solid_isAny(south) && space.shape_canStandIn(south) &&
			!space.pointFeature_contains(south, PointFeatureTypeId::Stairs) &&
			!space.pointFeature_contains(south, PointFeatureTypeId::Stairs)
		)
			return Facing4::North;
	}
	if(point.x() != boundry.m_high.x() && canConnectToAbove(point.east()))
	{
		const Point3D& west = point.west();
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
		const Point3D& east = point.east();
		if(!space.solid_isAny(east) && space.shape_canStandIn(point))
			return Facing4::West;
		else
			backup = Facing4::West;
	}
	return backup;
}
sf::Vector2f Draw::blockToPosition(const Point3D& point) const
{
	return {((float)point.x().get() * displayData::defaultScale), ((float)m_window.invertY(point.y().get()) * displayData::defaultScale)};
}
sf::Vector2f Draw::blockToPositionCentered(const Point3D& point) const
{
	return {((float)point.x().get() + 0.5f) * displayData::defaultScale, ((float)m_window.invertY(point.y().get()) + 0.5f) * displayData::defaultScale};
}
void Draw::accessableSymbol(const Point3D& point)
{
	stringOnBlock(point, "o", sf::Color::Green, 0.1, 0.6);
}
void Draw::inaccessableSymbol(const Point3D& point)
{
	stringOnBlock(point, "x", sf::Color::Red, 0.1, 0.6);
}
