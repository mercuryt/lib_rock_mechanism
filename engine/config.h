#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include "json.h"
#pragma GCC diagnostic pop
#include "types.h"
#include "index.h"
#include <cmath>
#include <cstdint>
#include <fstream>
namespace Config
{
	//inline uint32_t medicalPriority;
	//inline uint32_t medicalProjectDelaySteps;
	//
	// sort command for vim:
	// 	sort /\t/w* /w* /w* /
	// TODO: interval should be type Step but I don't know how to make it work with constexpr.
	inline constexpr uint actorDoVisionInterval = 3;
	inline constexpr bool fluidPiston = false;
	inline constexpr uint32_t fluidsSeepDiagonalModifier = 100;
	inline constexpr float dataStoreVectorResizeFactor = 1.5;
	inline constexpr float dataStoreVectorInitalSize = 10;
	inline constexpr int maxActorsPerBlock = 4;
	inline constexpr int maxItemsPerBlock = 4;

	// sort command for vim:
	// 	sort /\t/w* /w* /
	inline Step addToStockPileDelaySteps;
	inline float adjacentAllyCombatScoreBonusModifier;
	inline uint32_t attackCoolDownDurationBaseDextarity;
	inline Step attackCoolDownDurationBaseSteps;
	inline float attackSkillCombatModifier;
	inline Quality averageItemQuality;
	inline size_t averageLandHeightBlocks;
	inline float averageNumberOfRiversPerCandidate;
	inline Step baseHealDelaySteps;
	inline MoveCost baseMoveCost;
	inline Step bleedEventBaseFrequency;
	inline Step bleedPassOutDuration;
	inline float bleedToDeathRatio;
	inline float bleedToUnconciousessRatio;
	inline float blocksPerMeter;
	inline uint32_t bludgeonBleedVoumeRateModifier;
	inline uint32_t bludgeonPercentPermanantImparmentModifier;
	inline uint32_t bludgeonPercentTemporaryImparmentModifier;
	inline uint32_t bludgeonStepsTillHealedModifier;
	inline float bodyHardnessModifier;
	inline float chanceToWaitInsteadOfWander;
	inline uint32_t constructObjectivePriority;
	inline float constructSkillModifier;
	inline float constructStrengthModifier;
	inline uint32_t craftObjectivePriority;
	inline uint32_t cutBleedVoumeRateModifier;
	inline uint32_t cutPercentPermanantImparmentModifier;
	inline uint32_t cutPercentTemporaryImparmentModifier;
	inline uint32_t cutStepsTillHealedModifier;
	inline uint16_t daysPerYear;
	inline Temperature deepAmbiantTemperature;
	inline Step digMaxSteps;
	inline uint32_t digObjectivePriority;
	inline float digSkillModifier;
	inline float digStrengthModifier;
	inline uint32_t drinkPriority;
	inline uint32_t eatPriority;
	inline uint32_t equipPriority;
	inline Step exterminateCheckFrequency;
	inline uint32_t exterminatePriority;
	inline float fatPierceForceCost;
	inline float fireRampDownPhaseDurationFraction;
	inline float flankingModifier;
	inline uint16_t fluidGroupsPerThread;
	inline float forceAbsorbedPiercedModifier;
	inline float forceAbsorbedUnpiercedModifier;
	inline float fractionAttackCoolDownReductionPerPointOfDextarity;
	inline uint32_t getIntoAttackPositionMaxRange;
	inline uint32_t getToSafeTemperaturePriority;
	inline Step givePlantsFluidDelaySteps;
	inline uint32_t givePlantsFluidPriority;
	inline uint32_t goToPriority;
	inline MoveCost goUpMoveCost;
	inline Step harvestEventDuration;
	inline uint32_t harvestPriority;
	inline float heatDisipatesAtDistanceExponent;
	inline float heatFractionForBurn;
	inline float heatFractionForSmoulder;
	inline TemperatureDelta heatRadianceMinimum;
	inline uint32_t hitAreaToBodyPartVolumeRatioForFatalStrikeToVitalArea;
	inline uint32_t hitScaleModifier;
	inline uint32_t hoursPerDay;
	inline Volume impassibleItemVolume;
	inline Step installItemDuration;
	inline uint32_t installItemPriority;
	inline float itemQualityCombatModifier;
	inline float itemQualityModifier;
	inline float itemSkillModifier;
	inline float itemTypeCombatModifier;
	inline float itemTypeCombatScoreModifier;
	inline float itemWearCombatModifier;
	inline float itemWearModifier;
	inline uint32_t killPriority;
	inline uint32_t lakeDepthModifier;
	inline uint32_t lakeRadiusModifier;
	inline Step loadDelaySteps;
	inline DistanceInBlocks locationBucketSize;
	inline float massCarryMaximimMovementRatio;
	inline uint32_t maxAnimalInsertLocationSearchRetries;
	inline CollisionVolume maxBlockVolume;
	inline DistanceInBlocks maxBlocksToLookForBetterFood;
	inline DistanceInBlocks maxDistanceToLookForEatingLocation;
	inline float maxDistanceVisionModifier;
	inline Quantity maxNumberOfWorkersForConstructionProject;
	inline Quantity maxNumberOfWorkersForDigProject;
	inline Quantity maxNumberOfWorkersForWoodCuttingProject;
	inline DistanceInBlocks maxRangeToSearchForCraftingRequirements;
	inline DistanceInBlocks maxRangeToSearchForStockPileItems;
	inline DistanceInBlocks maxRangeToSearchForStockPiles;
	inline DistanceInBlocks maxRangeToSearchForDigDesignations;
	inline DistanceInBlocks maxRangeToSearchForConstructionDesignations;
	inline DistanceInBlocks maxRangeToSearchForWoodCuttingDesignations;
	inline DistanceInBlocks maxRangeToSearchForHorticultureDesignations;
	inline DistanceInBlocks maxRangeToSearchForUniformEquipment;
	inline uint32_t maxSkillLevel;
	inline uint32_t maxStaminaPointsBase;
	inline Quantity maxWorkersForStockPileProject;
	inline DistanceInBlocks maxZLevelForDeepAmbiantTemperature;
	inline Step maximumDurationToWaitInsteadOfWander;
	inline float maximumRainIntensityModifier;
	inline float maximumStepsBetweenRainPerPercentHumidity;
	inline float maximumStepsRainPerPercentHumidity;
	inline Meters metersHeightCarvedByRivers;
	inline Meters metersPerUnitElevationLociiIntensity;
	inline Meters minimumAltitudeForHeadwaterFormation;
	inline float minimumAttackCoolDownModifier;
	inline Step minimumDurationToWaitInsteadOfWander;
	inline Speed minimumHaulSpeedInital;
	inline float minimumOverloadRatio;
	inline Percent minimumPercentFoliageForGrow;
	inline Percent minimumPercentGrowthForWoodCutting;
	inline float minimumRainIntensityModifier;
	inline float minimumStepsBetweenRainPerPercentHumidity;
	inline float minimumStepsRainPerPercentHumidity;
	inline CollisionVolume minimumVolumeOfFluidToBreath;
	inline uint32_t minutesPerHour;
	inline uint32_t moveTryAttemptsBeforeDetour;
	inline float musclePierceForceCost;
	inline uint32_t objectivePrioiorityKill;
	inline uint32_t objectivePrioritySleep;
	inline uint8_t pathRequestsPerThread;
	inline float pathHuristicConstant;
	inline Percent percentHeightCarvedByRivers;
	inline uint8_t percentHungerAcceptableDesireModifier;
	inline Percent percentOfPlantMassWhichIsFoliage;
	inline Percent percentPermanantImparmentMinimum;
	inline uint32_t pierceBleedVoumeRateModifier;
	inline float pierceBoneModifier;
	inline float pierceFatModifier;
	inline float pierceModifier;
	inline float pierceMuscelModifier;
	inline uint32_t piercePercentPermanantImparmentModifier;
	inline uint32_t piercePercentTemporaryImparmentModifier;
	inline float pierceSkinModifier;
	inline uint32_t pierceStepsTillHealedModifier;
	inline float pointsOfCombatScorePerUnitOfAgility;
	inline float pointsOfCombatScorePerUnitOfDextarity;
	inline float pointsOfCombatScorePerUnitOfStrength;
	inline uint32_t projectTryToMakeSubprojectRetriesBeforeProjectDelay;
	inline Step projectDelayAfterExauhstingSubprojectRetries;
	inline uint32_t projectileHitChanceFallsOffWithRangeExponent;
	inline float projectileHitPercentPerPointAttackTypeCombatScore;
	inline float projectileHitPercentPerPointDextarity;
	inline float projectileHitPercentPerPointQuality;
	inline float projectileHitPercentPerPointTargetCombatScore;
	inline float projectileHitPercentPerPointWear;
	inline float projectileHitPercentPerSkillPoint;
	inline float projectileHitPercentPerUnitVolume;
	inline Volume projectileMedianTargetVolume;
	inline DistanceInBlocks rainMaximumOffset;
	inline DistanceInBlocks rainMaximumSpacing;
	inline Step rainWriteStepFreqency;
	inline float ratioOfHitAreaToBodyPartVolumeForSever;
	inline float ratioOfTotalBodyVolumeWhichIsBlood;
	inline Step ratioWoundsCloseDelayToBleedVolume;
	inline Step restIntervalSteps;
	inline float rollingMassModifier;
	inline uint32_t scaleOfHumanBody;
	inline uint32_t secondsPerMinute;
	inline float skinPierceForceCost;
	inline uint32_t sleepObjectivePriority;
	inline uint32_t sowSeedsPriority;
	inline Step sowSeedsStepsDuration;
	inline uint32_t staminaPointsPerRestPeriod;
	inline uint32_t stationPriority;
	inline Step stepsFrequencyToLookForHaulSubprojects;
	inline Step stepsPerDay;
	inline Step stepsPerHour;
	inline Step stepsPerMinute;
	inline Step stepsPerSecond;
	inline Step stepsPerYear;
	inline Step stepsTillDiePlantPriorityOveride;
	inline Step stepsToDelayBeforeTryingAgainToCompleteAnObjective;
	inline Step stepsToDelayBeforeTryingAgainToFollowLeader;
	inline Step stepsToDelayBeforeTryingAgainToReserveItemsAndActorsForAProject;
	inline Step stepsToDisableStockPile;
	inline Step stepsToDrink;
	inline Step stepsToEat;
	inline uint32_t stockPilePriority;
	inline uint32_t targetedHaulPriority;
	inline uint8_t threadedTaskBatchSize;
	inline uint32_t unarmedCombatScoreBase;
	inline float unarmedCombatSkillModifier;
	inline Temperature undergroundAmbiantTemperature;
	inline uint32_t unitsBodyMassPerUnitFluidConsumed;
	inline Mass unitsBodyMassPerUnitFoodConsumed;
	inline float unitsOfAttackForcePerUnitOfStrength;
	inline float unitsOfCarryMassPerUnitOfStrength;
	inline float unitsOfMoveSpeedPerUnitOfAgility;
	inline uint32_t unitsOfWoundAreaPerUnitItemScaleFactor;
	inline uint16_t unitsOfVolumePerUnitOfCollisionVolume;
	inline size_t visionFacadeReservationSize;
	inline VisionFacadeIndex visionThreadingBatchSize;
	inline uint32_t wanderMaximumNumberOfBlocks;
	inline uint32_t wanderMinimimNumberOfBlocks;
	inline Step woodCuttingMaxSteps;
	inline uint32_t woodCuttingObjectivePriority;
	inline float woodCuttingSkillModifier;
	inline float woodCuttingStrengthModifier;
	inline Step yokeDelaySteps;

	inline uint32_t convertBodyPartVolumeToArea(Volume volume){ return sqrt(volume.get()); }
	void load();
}
