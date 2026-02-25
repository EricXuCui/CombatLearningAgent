#include "CombatRewardLibrary.h"

#include "Math/UnrealMathUtility.h"

float UCombatRewardLibrary::NormalizeHP(float CurrentHP, float MaxHP)
{
	if (!FMath::IsFinite(CurrentHP) || !FMath::IsFinite(MaxHP))
	{
		return 0.0f;
	}

	if (MaxHP <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f);
}

float UCombatRewardLibrary::NormalizeRewardWithScale(float RawReward, float RewardScale, float MaxAbsReward)
{
	if (!FMath::IsFinite(RawReward) || !FMath::IsFinite(RewardScale) || !FMath::IsFinite(MaxAbsReward))
	{
		return 0.0f;
	}

	const float SafeScale = FMath::Max(0.0f, RewardScale);
	const float ScaledReward = RawReward * SafeScale;
	if (MaxAbsReward <= KINDA_SMALL_NUMBER)
	{
		return FMath::Clamp(ScaledReward, -1.0f, 1.0f);
	}

	const float SafeMaxAbsReward = MaxAbsReward;

	return FMath::Clamp(ScaledReward / SafeMaxAbsReward, -1.0f, 1.0f);
}

float UCombatRewardLibrary::ComputeCombatStepReward(
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
	float WinReward,
	float LoseReward,
	float TargetHPDamageScale,
	float SelfHPDamageScale,
	float TimePenalty,
	float DistanceApproachScale,
	float RetreatPenaltyScale,
	float IdlePenalty,
	float WallTouchPenalty,
	float StuckPenalty,
	float JitterPenaltyScale,
	float StaminaUseThreshold,
	float StaminaUseActionPenalty,
	float IdleSpeedThreshold,
	float MinCombatDistance,
	float MaxCombatDistance,
	bool bNormalizeFinalReward,
	float RewardScale,
	float MaxAbsReward,
	int32 FailedActionRequests,
	float FailedActionPenaltyPerRequest)
{
	const float SelfHPNorm = NormalizeHP(CurrentSelfHP, MaxSelfHP);
	const float TargetHPNorm = NormalizeHP(CurrentTargetHP, MaxTargetHP);
	const float SelfHPDelta = FMath::Clamp(PrevSelfHP - SelfHPNorm, -1.0f, 1.0f);
	const float TargetHPDelta = FMath::Clamp(PrevTargetHP - TargetHPNorm, -1.0f, 1.0f);
	const float SafePrevDistance = FMath::Max(0.0f, FMath::IsFinite(PrevDistanceToTarget) ? PrevDistanceToTarget : 0.0f);
	const float SafeCurrentDistance = FMath::Max(0.0f, FMath::IsFinite(CurrentDistanceToTarget) ? CurrentDistanceToTarget : SafePrevDistance);
	const float EffectiveMinCombatDistance = FMath::Max(0.0f, MinCombatDistance);
	const float EffectiveMaxCombatDistance = FMath::Max(EffectiveMinCombatDistance + 1.0f, MaxCombatDistance);
	const float MaxDistanceDeltaPerStep = FMath::Max(25.0f, EffectiveMaxCombatDistance * 0.15f);
	const float RawDistanceDelta = SafePrevDistance - SafeCurrentDistance;
	const float DistanceDelta = FMath::Clamp(RawDistanceDelta, -MaxDistanceDeltaPerStep, MaxDistanceDeltaPerStep);

	float Reward = TimePenalty;
	Reward += TargetHPDelta * TargetHPDamageScale;
	Reward -= SelfHPDelta * SelfHPDamageScale;

	Reward += FMath::Max(0.0f, DistanceDelta) * DistanceApproachScale;
	Reward -= FMath::Max(0.0f, -DistanceDelta) * RetreatPenaltyScale;

	const float SafeIdleSpeedThreshold = FMath::Max(0.0f, IdleSpeedThreshold);
	const float SafeSelfSpeed = FMath::IsFinite(SelfSpeed) ? FMath::Max(0.0f, SelfSpeed) : 0.0f;
	const bool bInCombatRange = SafeCurrentDistance >= EffectiveMinCombatDistance && SafeCurrentDistance <= EffectiveMaxCombatDistance;
	if (!bCanDealDamage && bInCombatRange && SafeSelfSpeed <= SafeIdleSpeedThreshold)
	{
		Reward += FMath::Min(0.0f, IdlePenalty);
	}

	if (bTouchingWall)
	{
		Reward += WallTouchPenalty;
	}

	if (bStuck)
	{
		Reward += StuckPenalty;
	}

	Reward += FMath::Clamp(MovementJitterScore, 0.0f, 1.0f) * JitterPenaltyScale;

	if (FMath::IsFinite(PrevStamina) && FMath::IsFinite(CurrentStamina))
	{
		const float StaminaSpent = PrevStamina - CurrentStamina;
		if (StaminaSpent > FMath::Max(0.0f, StaminaUseThreshold))
		{
			Reward += StaminaUseActionPenalty;
		}
	}

	if (bTargetDead)
	{
		Reward += WinReward;
	}

	if (bSelfDead)
	{
		Reward += LoseReward;
	}

	const int32 SafeFailedActionRequests = FMath::Max(0, FailedActionRequests);
	const float SafeFailedActionPenaltyPerRequest = FMath::Min(0.0f, FailedActionPenaltyPerRequest);
	Reward += static_cast<float>(SafeFailedActionRequests) * SafeFailedActionPenaltyPerRequest;

	return bNormalizeFinalReward ? NormalizeRewardWithScale(Reward, RewardScale, MaxAbsReward) : Reward;
}

void UCombatRewardLibrary::UpdateRewardHistory(
	float CurrentSelfHP,
	float MaxSelfHP,
	float CurrentTargetHP,
	float MaxTargetHP,
	float CurrentDistanceToTarget,
	float CurrentStamina,
	float& PrevSelfHP,
	float& PrevTargetHP,
	float& PrevDistanceToTarget,
	float& PrevStamina)
{
	PrevSelfHP = NormalizeHP(CurrentSelfHP, MaxSelfHP);
	PrevTargetHP = NormalizeHP(CurrentTargetHP, MaxTargetHP);
	PrevDistanceToTarget = FMath::IsFinite(CurrentDistanceToTarget) ? FMath::Max(0.0f, CurrentDistanceToTarget) : 0.0f;
	PrevStamina = FMath::IsFinite(CurrentStamina) ? FMath::Max(0.0f, CurrentStamina) : 0.0f;
}
