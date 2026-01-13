#pragma once
#include "../numericTypes/types.h"
namespace Config::Social
{
	inline Step conversationDurationSteps;
	inline PsycologyWeight insprationalSpeachCourageToAdd;
	inline PsycologyWeight insprationalSpeachPositivityToAdd;
	inline Step inspirationalSpeachCoolDownDuration;
	inline Step inspirationalSpeachDuration;
	inline Step inspirationalSpeachEffectDuration;
	inline Step intervalBetweenChats;
	inline int maximumDurationToChastiseCycles;
	inline PsycologyWeight minimumAngerForChastisingToBeAngry;
	inline float minimumCastingScoreForChastiseMistakeAtWork;
	inline float minimumCastingScoreForPraiseSuccessAtWork;
	inline int minimumDurationToChastiseCycles;
	inline PsycologyWeight minimumPrideToAlwaysGetAngryWhenChastised;
	inline float minimumRatioOfSkillLevelsToChastise;
	inline float multipleForChastiseDurationIfSomeoneWasHurt;
	inline float multipleForChastiseDurationIfSomeoneWasKilled;
	inline Step praiseDurationSteps;
	inline Step praisePsycologyEventDuration;
	inline PsycologyWeight prideToAddWhenPraised;
	inline PsycologyWeight prideToAddWhenWinningAConfrontation;
	inline PsycologyWeight prideToLoseWhenChastised;
	inline PsycologyWeight prideToLoseWhenLosingAConfrontation;
	inline Step psycologyEventChastisedDuration;
	inline Percent randomFuzzToApplyToCourageChecks;
	inline Priority socialPriorityHigh;
	inline Priority socialPriorityLow;
	inline PsycologyWeight unitsOfCourageToGainOnPassingFearTest;
	inline PsycologyWeight unitsOfCourageToLoseOnFailingFearTest;
	void load();
};