#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatRewardLibrary.generated.h"

UCLASS()
class COMBATLEARNINGAGENT_API UCombatRewardLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Learning|Reward")
	static float NormalizeHP(float CurrentHP, float MaxHP);

	UFUNCTION(BlueprintPure, Category = "Learning|Reward")
	static float NormalizeRewardWithScale(float RawReward, float RewardScale = 1.0f, float MaxAbsReward = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Learning|Reward")
	static float ComputeCombatStepReward(
		float CurrentSelfHP,
		float MaxSelfHP,
		float CurrentTargetHP,
		float MaxTargetHP,
		float PrevSelfHP,
		float PrevTargetHP,
		float CurrentDistanceToTarget,
		float PrevDistanceToTarget,
		float SelfSpeed,
		float MovementJitterScore,
		float CurrentStamina,
		float PrevStamina,
		bool bCanDealDamage,
		bool bTouchingWall,
		bool bStuck,
		bool bTargetDead,
		bool bSelfDead,
		float WinReward = 10.0f,
		float LoseReward = -10.0f,
		float TargetHPDamageScale = 1.0f,
		float SelfHPDamageScale = 1.0f,
		float TimePenalty = -0.001f,
		float DistanceApproachScale = 0.01f,
		float RetreatPenaltyScale = 0.02f,
		float IdlePenalty = 0.0f,
		float WallTouchPenalty = -1.0f,
		float StuckPenalty = -2.0f,
		float JitterPenaltyScale = -0.05f,
		float StaminaUseThreshold = 0.1f,
		float StaminaUseActionPenalty = -0.1f,
		float IdleSpeedThreshold = 0.0f,
		float MinCombatDistance = 150.0f,
		float MaxCombatDistance = 900.0f,
		bool bNormalizeFinalReward = false,
		float RewardScale = 1.0f,
		float MaxAbsReward = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Learning|Reward")
	static void UpdateRewardHistory(
		float CurrentSelfHP,
		float MaxSelfHP,
		float CurrentTargetHP,
		float MaxTargetHP,
		float CurrentDistanceToTarget,
		float CurrentStamina,
		UPARAM(ref) float& PrevSelfHP,
		UPARAM(ref) float& PrevTargetHP,
		UPARAM(ref) float& PrevDistanceToTarget,
		UPARAM(ref) float& PrevStamina);
};
