#pragma once
#include "../numericTypes/types.h"
namespace Config::Psycology
{
	inline PsycologyWeight angerToAddWhenChastised;
	inline PsycologyWeight angerToAddWhenLosingAConfrontation;
	inline float minimumRatioOfDeltaToFriendlyMaliceToTestCourge;
	inline Step durationForAccidentalHomicidePrideLoss;
	inline Step durationForConfrontationWinPrideGain;
	inline Step durationForAccidentalInjuryPrideLoss;
	inline Step durationForConfrontationLossPrideLoss;
	inline PsycologyWeight courageToGainOnStandGround;
	inline Step loseConfrontationDuration;
	inline PsycologyWeight minimumMaliceDeltaPlusCourageToHoldFirmWithoutTest;
	inline PsycologyWeight prideToLoseWhenAccidentallyKilling;
	inline PsycologyWeight prideToLoseWhenAccidentallyInjuring;
	inline Step coolDownForStandGround;
	inline Step intervalToCheckIfSoldiersFlee;
	void load();
};