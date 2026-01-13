#include "psycology.h"
#include "config.h"
#include <fstream>

void Config::Psycology::load()
{
	std::ifstream f("data/config/psycology.json");
	Json data = Json::parse(f);
	data["angerToAddWhenChastised"].get_to(angerToAddWhenChastised);
	data["angerToAddWhenLosingAConfrontation"].get_to(angerToAddWhenLosingAConfrontation);
	coolDownForStandGround = Config::stepsPerMinute * data["coolDownForStandGroundMinutes"].get<int32_t>();
	durationForAccidentalHomicidePrideLoss = Config::stepsPerDay * data["durationForAccidentalHomicidePrideLossDays"].get<int32_t>();
	durationForConfrontationWinPrideGain = Config::stepsPerHour * data["durationForConfrontationWinPrideGainHours"].get<int32_t>();
	durationForAccidentalInjuryPrideLoss = Config::stepsPerHour * data["durationForAccidentalInjuryPrideLossHours"].get<int32_t>();
	durationForConfrontationLossPrideLoss = Config::stepsPerHour * data["durationForConfrontationLossPrideLossHours"].get<int32_t>();
	data["courageToGainOnStandGround"].get_to(courageToGainOnStandGround);
	loseConfrontationDuration = Config::stepsPerDay * data["loseConfrontationDurationDays"].get<int32_t>();
	data["minimumRatioOfDeltaToFriendlyMaliceToTestCourge"].get_to(minimumRatioOfDeltaToFriendlyMaliceToTestCourge);
	data["minimumMaliceDeltaPlusCourageToHoldFirmWithoutTest"].get_to(minimumMaliceDeltaPlusCourageToHoldFirmWithoutTest);
	data["prideToLoseWhenAccidentallyKilling"].get_to(prideToLoseWhenAccidentallyKilling);
	data["prideToLoseWhenAccidentallyInjuring"].get_to(prideToLoseWhenAccidentallyInjuring);
	intervalToCheckIfSoldiersFlee  = Config::stepsPerSecond * data["intervalToCheckIfSoldiersFleeseconds"].get<int32_t>();
}
