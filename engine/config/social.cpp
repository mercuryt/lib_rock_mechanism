#include "social.h"
#include "config.h"
#include <fstream>

void Config::Social::load()
{
	std::ifstream f("data/config/social.json");
	Json data = Json::parse(f);
	conversationDurationSteps = Config::stepsPerMinute * data["conversationDurationMinutes"].get<float>();
	data["insprationalSpeachCourageToAdd"].get_to(insprationalSpeachCourageToAdd);
	data["insprationalSpeachPositivityToAdd"].get_to(insprationalSpeachPositivityToAdd);
	inspirationalSpeachCoolDownDuration = Config::stepsPerHour * data["inspirationalSpeachCoolDownDurationHours"].get<float>();
	inspirationalSpeachEffectDuration = Config::stepsPerMinute * data["inspirationalSpeachEffectDurationMinutes"].get<float>();
	inspirationalSpeachDuration = Config::stepsPerMinute * data["inspirationalSpeachDurationMinutes"].get<float>();
	intervalBetweenChats = Config::stepsPerMinute * data["intervalBetweenChatsMinutes"].get<float>();
	data["maximumDurationToChastiseCycles"].get_to(maximumDurationToChastiseCycles);
	data["minimumAngerForChastisingToBeAngry"].get_to(minimumAngerForChastisingToBeAngry);
	data["minimumCastingScoreForChastiseMistakeAtWork"].get_to(minimumCastingScoreForChastiseMistakeAtWork);
	data["minimumCastingScoreForPraiseSuccessAtWork"].get_to(minimumCastingScoreForPraiseSuccessAtWork);
	data["minimumDurationToChastiseCycles"].get_to(minimumDurationToChastiseCycles);
	data["minimumPrideToAlwaysGetAngryWhenChastised"].get_to(minimumPrideToAlwaysGetAngryWhenChastised);
	data["minimumRatioOfSkillLevelsToChastise"].get_to(minimumRatioOfSkillLevelsToChastise);
	data["multipleForChastiseDurationIfSomeoneWasHurt"].get_to(multipleForChastiseDurationIfSomeoneWasHurt);
	data["multipleForChastiseDurationIfSomeoneWasKilled"].get_to(multipleForChastiseDurationIfSomeoneWasKilled);
	praiseDurationSteps = Config::stepsPerMinute * data["praiseDurationMinutes"].get<float>();
	praisePsycologyEventDuration = Config::stepsPerHour * data["praisePsycologyEventDurationHours"].get<float>();
	data["prideToAddWhenPraised"].get_to(prideToAddWhenPraised);
	data["prideToAddWhenWinningAConfrontation"].get_to(prideToAddWhenWinningAConfrontation);
	data["prideToLoseWhenChastised"].get_to(prideToLoseWhenChastised);
	data["prideToLoseWhenLosingAConfrontation"].get_to(prideToLoseWhenLosingAConfrontation);
	psycologyEventChastisedDuration = Config::stepsPerHour * data["psycologyEventChastisedDurationHours"].get<float>();
	data["randomFuzzToApplyToCourageChecks"].get_to(randomFuzzToApplyToCourageChecks);
	data["socialPriorityHigh"].get_to(socialPriorityHigh);
	data["socialPriorityLow"].get_to(socialPriorityLow);
	data["unitsOfCourageToGainOnPassingFearTest"].get_to(unitsOfCourageToGainOnPassingFearTest);
	data["unitsOfCourageToLoseOnFailingFearTest"].get_to(unitsOfCourageToLoseOnFailingFearTest);
}
