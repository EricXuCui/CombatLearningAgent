// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseRole.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AIEnemyForTraining.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	float SanitizeCurriculumScale(float Scale)
	{
		return FMath::Max(0.1f, Scale);
	}

}

// Sets default values
ABaseRole::ABaseRole()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// Initialize Components
	PrimaryActorTick.bCanEverTick = true;
	BaseRoleCameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("TPCameraArm"));
	BaseRoleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TPCamera"));
	WeaponArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("WeaponArrow"));
	BaseRoleCameraArm->SetupAttachment(GetMesh());
	BaseRoleCamera->SetupAttachment(BaseRoleCameraArm);
	WeaponArrowComponent->SetupAttachment(GetMesh());
	// Initialize Variables
	BaseMaxHP = 100.f;
	BaseDamage = 15.f;
	BaseMaxStamina = 100.f;
	CurriculumHPScale = 1.f;
	CurriculumDamageScale = 1.f;
	CurriculumStaminaScale = 1.f;
	CurriculumWarmupEpisodes = 500;
	CurriculumRampEpisodes = 1500;
	WarmupPlayerHPScale = 1.2f;
	WarmupPlayerDamageScale = 1.1f;
	WarmupPlayerStaminaScale = 1.2f;
	WarmupEnemyHPScale = 0.8f;
	WarmupEnemyDamageScale = 0.85f;
	EnemyTargetRefreshInterval = 0.50f;
	bEnableWallObservationCache = true;
	WallObservationCacheInterval = 0.05f;
	TrainingFocusUpdateInterval = 0.04f;
	SpawnProtectionSeconds = 0.35f;
	EnemyTargetRefreshCooldown = 0.0f;
	TrainingFocusCooldown = 0.0f;
	SpawnProtectionUntilTime = 0.0f;
	CachedWallFrontNorm = 1.0f;
	CachedWallRightNorm = 1.0f;
	CachedWallBackNorm = 1.0f;
	CachedWallLeftNorm = 1.0f;
	WallObservationCacheCooldown = 0.0f;
	FailedActionRequestsSinceConsume = 0;
	bAnyActionExecutedSinceConsume = false;
	MaxHP = BaseMaxHP;
	CurrentHP = MaxHP;
	Damage = BaseDamage;
	MaxStamina = BaseMaxStamina;
	CurrentStamina = MaxStamina;
	ForwardVal = 0.f;
	RightVal = 0.f;
	LastForwardVal = 0.f;
	LastRightVal = 0.f;
	MovementJitterAccumulator = 0.f;
	AttackIndex = 0;
	bDead = false;
	bAttacking = false;
	bRunning = false;
	bRolling = false;
	bDefend = false;
	bDoding = false;
	bTrainingMode = false;
	bEquip = false;
	bInjury = false;
	bLock = false;
	GetCharacterMovement()->MaxWalkSpeed = 200.f;
	// Initialize Character Settings
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	BaseRoleCameraArm->bUsePawnControlRotation = true;
	BaseRoleCameraArm->bEnableCameraLag = true;
	BaseRoleCameraArm->TargetArmLength = 400.f;
}

void ABaseRole::RefreshEnemyTarget(bool bForce)
{
	if (IsValid(EnemyTarget))
	{
		EnemyTargetRefreshCooldown = FMath::Max(0.0f, EnemyTargetRefreshCooldown);
		return;
	}
	EnemyTarget = nullptr;

	if (!bForce)
	{
		if (EnemyTargetRefreshCooldown > 0.0f)
		{
			return;
		}
	}

	if (!GetWorld())
	{
		EnemyTargetRefreshCooldown = FMath::Max(0.05f, EnemyTargetRefreshInterval);
		return;
	}

	UClass* EnemyClass = InstanceOfEnemy ? InstanceOfEnemy.Get() : AAIEnemyForTraining::StaticClass();
	if (!EnemyClass)
	{
		EnemyTargetRefreshCooldown = FMath::Max(0.05f, EnemyTargetRefreshInterval);
		return;
	}

	TArray<AActor*> EnemyCandidates;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), EnemyClass, EnemyCandidates);

	AAIEnemyForTraining* BestTarget = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	const FVector SelfLocation = GetActorLocation();
	for (AActor* Candidate : EnemyCandidates)
	{
		AAIEnemyForTraining* EnemyCandidate = Cast<AAIEnemyForTraining>(Candidate);
		if (!IsValid(EnemyCandidate) || EnemyCandidate->bDead)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(SelfLocation, EnemyCandidate->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestTarget = EnemyCandidate;
		}
	}

	EnemyTarget = BestTarget;
	EnemyTargetRefreshCooldown = FMath::Max(0.05f, EnemyTargetRefreshInterval);
}

void ABaseRole::ApplyCurriculumScalars(float InHPScale, float InDamageScale, float InStaminaScale)
{
	CurriculumHPScale = SanitizeCurriculumScale(InHPScale);
	CurriculumDamageScale = SanitizeCurriculumScale(InDamageScale);
	CurriculumStaminaScale = SanitizeCurriculumScale(InStaminaScale);

	MaxHP = BaseMaxHP * CurriculumHPScale;
	Damage = BaseDamage * CurriculumDamageScale;
	MaxStamina = BaseMaxStamina * CurriculumStaminaScale;
	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
	CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);
}

void ABaseRole::ApplyCurriculumForEpisode(int32 EpisodeIndex)
{
	const int32 SafeEpisode = FMath::Max(0, EpisodeIndex);
	const int32 SafeWarmupEpisodes = FMath::Max(0, CurriculumWarmupEpisodes);
	const int32 SafeRampEpisodes = FMath::Max(1, CurriculumRampEpisodes);

	float PlayerHPScale = 1.f;
	float PlayerDamageScale = 1.f;
	float PlayerStaminaScale = 1.f;
	float EnemyHPScale = 1.f;
	float EnemyDamageScale = 1.f;

	if (SafeEpisode < SafeWarmupEpisodes)
	{
		PlayerHPScale = WarmupPlayerHPScale;
		PlayerDamageScale = WarmupPlayerDamageScale;
		PlayerStaminaScale = WarmupPlayerStaminaScale;
		EnemyHPScale = WarmupEnemyHPScale;
		EnemyDamageScale = WarmupEnemyDamageScale;
	}
	else
	{
		const int32 RampElapsed = SafeEpisode - SafeWarmupEpisodes;
		const float Alpha = FMath::Clamp(static_cast<float>(RampElapsed) / static_cast<float>(SafeRampEpisodes), 0.f, 1.f);
		PlayerHPScale = FMath::Lerp(WarmupPlayerHPScale, 1.f, Alpha);
		PlayerDamageScale = FMath::Lerp(WarmupPlayerDamageScale, 1.f, Alpha);
		PlayerStaminaScale = FMath::Lerp(WarmupPlayerStaminaScale, 1.f, Alpha);
		EnemyHPScale = FMath::Lerp(WarmupEnemyHPScale, 1.f, Alpha);
		EnemyDamageScale = FMath::Lerp(WarmupEnemyDamageScale, 1.f, Alpha);
	}

	ApplyCurriculumScalars(PlayerHPScale, PlayerDamageScale, PlayerStaminaScale);
	if (EnemyTarget)
	{
		EnemyTarget->ApplyCurriculumScalars(EnemyHPScale, EnemyDamageScale);
	}
}

void ABaseRole::MoveForward(float Val)
{
	const bool bPrevActive = FMath::Abs(LastForwardVal) > 0.2f;
	const bool bCurrActive = FMath::Abs(Val) > 0.2f;
	if (bPrevActive && bCurrActive && FMath::Sign(LastForwardVal) != FMath::Sign(Val))
	{
		MovementJitterAccumulator = FMath::Min(10.0f, MovementJitterAccumulator + 1.0f);
	}

	ForwardVal = Val;
	LastForwardVal = Val;
	const FRotator LocalRotator = GetControlRotation();
	const FVector LocalInputVector = FRotationMatrix(FRotator(0, LocalRotator.Yaw, 0)).GetUnitAxis(EAxis::X);
	AddMovementInput(LocalInputVector, Val);
}

void ABaseRole::MoveRight(float Val)
{
	const bool bPrevActive = FMath::Abs(LastRightVal) > 0.2f;
	const bool bCurrActive = FMath::Abs(Val) > 0.2f;
	if (bPrevActive && bCurrActive && FMath::Sign(LastRightVal) != FMath::Sign(Val))
	{
		MovementJitterAccumulator = FMath::Min(10.0f, MovementJitterAccumulator + 1.0f);
	}

	RightVal = Val;
	LastRightVal = Val;
	const FRotator LocalRotator = GetControlRotation();
	const FVector LocalInputVector = FRotationMatrix(FRotator(0, LocalRotator.Yaw, 0)).GetUnitAxis(EAxis::Y);
	AddMovementInput(LocalInputVector, Val);
}

void ABaseRole::MouseX(float Val)
{
	if (!bLock)
	{
		APawn::AddControllerYawInput(Val);
	}
}

void ABaseRole::MouseY(float Val)
{
	if (!bLock)
	{
		APawn::AddControllerPitchInput(Val * -1.f);
	}
}

void ABaseRole::Running()
{
	const bool bCanRun = IsCombatActionGateOpen();
	if (bCanRun)
	{
		CameraFovExtend(true);
		if (bDefend)
		{
			StopDefending();
		}
		bRunning = true;
		GetCharacterMovement()->MaxWalkSpeed = 500.f;
		RecordActionResult(true);
	}
	else
	{
		StopRunning();
		RecordActionResult(false);
	}
}

void ABaseRole::StopRunning()
{
	CameraFovExtend(false);
	bRunning = false;
	GetCharacterMovement()->MaxWalkSpeed = 200.f;
}

void ABaseRole::Dodge()
{
	constexpr float DirectionThreshold = 0.35f;
	constexpr float SideThreshold = 0.35f;
	constexpr float DodgeStaminaCost = 10.0f;
	const bool bCanDodge = IsCombatActionGateOpen() && CurrentStamina >= DodgeStaminaCost;
	bool bExecuted = false;

	if (bCanDodge)
	{
		if (GetMesh()->GetAnimInstance())
		{
			CurrentStamina -= DodgeStaminaCost;
			if (bRunning)
			{
				StopRunning();
			}
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			bDoding = true;
			bDefend = false;
			float DodingPlayRate = 1.2f;
			if (ForwardVal > DirectionThreshold)
			{
				//Forward
				AnimInstance->Montage_Play(DodgeAnimMontages[0], DodingPlayRate);

			}
			else if (ForwardVal > -DirectionThreshold)
			{
				if (RightVal > SideThreshold)
				{
					//Right
					AnimInstance->Montage_Play(DodgeAnimMontages[1], DodingPlayRate);
				}
				else if (RightVal < -SideThreshold)
				{
					//Left
					AnimInstance->Montage_Play(DodgeAnimMontages[2], DodingPlayRate);
				}
				else
				{
					//Backward
					AnimInstance->Montage_Play(DodgeAnimMontages[3], DodingPlayRate);
				}
			}
			else
			{
				//Backward
				AnimInstance->Montage_Play(DodgeAnimMontages[3], DodingPlayRate);
			}
			bExecuted = true;

		}

	}
	RecordActionResult(bExecuted);
}

void ABaseRole::Roll()
{
	constexpr float DirectionThreshold = 0.35f;
	constexpr float SideThreshold = 0.35f;
	constexpr float RollStaminaCost = 13.0f;
	const bool bCanRoll = IsCombatActionGateOpen() && CurrentStamina >= RollStaminaCost;
	bool bExecuted = false;

	if (bCanRoll)
	{
		if (GetMesh()->GetAnimInstance())
		{
			if (bRunning)
			{
				StopRunning();
			}
			CurrentStamina -= RollStaminaCost;
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			bRolling = true;
			bDefend = false;
			float RollingPlayRate = 1.2f;
			if (ForwardVal > DirectionThreshold)
			{
					//Forward
					AnimInstance->Montage_Play(RollAnimMontages[0], RollingPlayRate);
			
			}
			else if (ForwardVal > -DirectionThreshold)
			{
				if (RightVal > SideThreshold)
				{
					//Right
					AnimInstance->Montage_Play(RollAnimMontages[1], RollingPlayRate);
				}
				else if (RightVal < -SideThreshold)
				{
					//Left
					AnimInstance->Montage_Play(RollAnimMontages[2], RollingPlayRate);
				}
				else
				{
					//Backward
					AnimInstance->Montage_Play(RollAnimMontages[3], RollingPlayRate);
				}
			}
			else
			{
				//Backward
				AnimInstance->Montage_Play(RollAnimMontages[3], RollingPlayRate);
			}
			bExecuted = true;
			
		}

	}
	RecordActionResult(bExecuted);
}

void ABaseRole::Attack()
{
	constexpr float AttackStaminaCost = 15.0f;
	const bool bCanAttack = IsCombatActionGateOpen() && CurrentStamina >= AttackStaminaCost && !bInjury;
	bool bExecuted = false;
	if (bCanAttack)
	{
		if (GetMesh()->GetAnimInstance())
		{
			if (bRunning)
			{
				StopRunning();
			}
			CurrentStamina -= AttackStaminaCost;
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			int CurrentAttackIndex = UKismetMathLibrary::RandomIntegerInRange(0, 5);
			if (CurrentAttackIndex != AttackIndex)
			{
				AttackIndex = CurrentAttackIndex;
			}
			else
			{
				if (CurrentAttackIndex == 0)
				{
					AttackIndex = ++CurrentAttackIndex;
				}
				else
				{
					AttackIndex = --CurrentAttackIndex;
				}
			}
			bAttacking = true;
			AnimInstance->Montage_Play(AttackAnimMontages[AttackIndex]);
			bExecuted = true;

		}
	}
	RecordActionResult(bExecuted);
}

void ABaseRole::ResetAttacking()
{
	bAttacking = false;
}


void ABaseRole::ResetDoding()
{
	bDoding = false;
	ResetInjury();
}

void ABaseRole::Defend()
{
	const bool bCanDefend = IsCombatActionGateOpen() && !bInjury;
	if (bCanDefend)
	{
		if (bRunning)
		{
			StopRunning();
		}
		bDefend = true;
	}
	RecordActionResult(bCanDefend);
}

void ABaseRole::StopDefending()
{
	bDefend = false;
}
void ABaseRole::AttackTrace()
{
	FVector ArrowLocation = WeaponArrowComponent->GetComponentLocation();
	FVector ArrowForwardLocation = WeaponArrowComponent->GetForwardVector() * 100;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<FHitResult> HitArray;

	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		ArrowLocation,
		ArrowLocation + ArrowForwardLocation,
		60.f,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitArray,
		true
	);

	if (bHit)
	{
		for (const FHitResult& HitTarget : HitArray)
		{
			if (HitTarget.GetActor() && HitTarget.GetActor() != this)
			{
				AAIEnemyForTraining* Target = Cast<AAIEnemyForTraining>(HitTarget.GetActor());
				if (Target)
				{
					Target->ReceiveDamage(Damage);
				}
			}
		}
	}
}



void ABaseRole::ResetInjury()
{
	bInjury = false;
}

void ABaseRole::ResetBaseRoleAgent()
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->StopAllMontages(0.1f);
	}
	ResetAllConditions();
	ApplyCurriculumScalars(CurriculumHPScale, CurriculumDamageScale, CurriculumStaminaScale);
	CurrentHP = MaxHP;
	CurrentStamina = MaxStamina;
	bDead = false;
	bAttacking = false;
	bRunning = false;
	bRolling = false;
	bDefend = false;
	bDoding = false;
	bTrainingMode = true;
	bEquip = false;
	bInjury = false;
	ForwardVal = 0.0f;
	RightVal = 0.0f;
	LastForwardVal = 0.0f;
	LastRightVal = 0.0f;
	MovementJitterAccumulator = 0.0f;
	EnemyTargetRefreshCooldown = 0.0f;
	TrainingFocusCooldown = 0.0f;
	WallObservationCacheCooldown = 0.0f;
	FailedActionRequestsSinceConsume = 0;
	bAnyActionExecutedSinceConsume = false;
	if (bTrainingMode && GetWorld())
	{
		SpawnProtectionUntilTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, SpawnProtectionSeconds);
	}
	else
	{
		SpawnProtectionUntilTime = 0.0f;
	}
	GetCharacterMovement()->MaxWalkSpeed = 200.f;
	if (EnemyTarget)
	{
		EnemyTarget->ResetTarget();
	}
	SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
	DrawSword();
}

void ABaseRole::GetCombatActionAvailability(bool& bCanAttack, bool& bCanDefend, bool& bCanDodge, bool& bCanRoll, bool& bCanRun) const
{
	constexpr float AttackStaminaCost = 15.0f;
	constexpr float DodgeStaminaCost = 10.0f;
	constexpr float RollStaminaCost = 13.0f;

	const bool bGateOpen = IsCombatActionGateOpen();
	bCanAttack = bGateOpen && !bInjury && CurrentStamina >= AttackStaminaCost;
	bCanDefend = bGateOpen && !bInjury;
	bCanDodge = bGateOpen && CurrentStamina >= DodgeStaminaCost;
	bCanRoll = bGateOpen && CurrentStamina >= RollStaminaCost;
	bCanRun = bGateOpen;
}

bool ABaseRole::IsCombatActionGateOpen() const
{
	return !bRolling && !bAttacking && !bDoding && bEquip;
}

float ABaseRole::ConsumeActionFailurePenalty(float FailurePenaltyPerAction, int32 MaxFailuresToCount)
{
	const int32 FailuresToCount = ConsumeFailedActionRequests(MaxFailuresToCount);
	const float SafeFailurePenalty = FMath::Min(0.0f, FailurePenaltyPerAction);
	return SafeFailurePenalty * static_cast<float>(FailuresToCount);
}

int32 ABaseRole::ConsumeFailedActionRequests(int32 MaxFailuresToCount)
{
	const int32 SafeMaxFailures = FMath::Max(0, MaxFailuresToCount);
	const int32 FailuresToCount = FMath::Clamp(FailedActionRequestsSinceConsume, 0, SafeMaxFailures);
	FailedActionRequestsSinceConsume = 0;
	bAnyActionExecutedSinceConsume = false;
	return FailuresToCount;
}

bool ABaseRole::ConsumeAnyActionExecutedSinceLastQuery()
{
	const bool bHadAnyExecutedAction = bAnyActionExecutedSinceConsume;
	bAnyActionExecutedSinceConsume = false;
	return bHadAnyExecutedAction;
}

void ABaseRole::RecordActionResult(bool bExecuted)
{
	if (bExecuted)
	{
		bAnyActionExecutedSinceConsume = true;
	}
	else
	{
		++FailedActionRequestsSinceConsume;
	}
}


void ABaseRole::LockCameraToTarget()
{
	if (bLock)
	{
		bLock = false;
	}
	else
	{
		bLock = true;
	}
}



void ABaseRole::ReceiveDamage(float IDamage, bool bUltimateAttack)
{
	if (!bDead && !bInjury)
	{
		if (bTrainingMode && GetWorld() && GetWorld()->GetTimeSeconds() < SpawnProtectionUntilTime)
		{
			return;
		}

		if (GetMesh()->GetAnimInstance())
		{
			const bool bWasDefending = bDefend;
			const bool bWasEvading = bRolling || bDoding;
			ResetAllConditions();
			bInjury = true;

			if (bWasDefending)
			{
				if (bUltimateAttack)
				{
					GetMesh()->GetAnimInstance()->Montage_Play(DefenseHeavyDamageMontages);
					constexpr float UltimateBlockStaminaCost = 15.0f;
					if (CurrentStamina >= UltimateBlockStaminaCost)
					{
						CurrentStamina -= UltimateBlockStaminaCost;
					}
					else
					{
						CurrentStamina = 0.f;
						CurrentHP -= IDamage * 1.5;
					}
				}
				else
				{
					constexpr float BlockStaminaCost = 5.0f;
					if (CurrentStamina >= BlockStaminaCost)
					{
						CurrentStamina -= BlockStaminaCost;
					}
					else
					{
						CurrentHP -= IDamage;
					}
					GetMesh()->GetAnimInstance()->Montage_Play(DefendAnimMontage);
				}
			}
			else
			{
				if (bUltimateAttack)
				{
					GetMesh()->GetAnimInstance()->Montage_Play(HeavyDamageMontages);
					CurrentStamina = FMath::Max(0.f, CurrentStamina - 15.f);
					CurrentHP -= IDamage * 1.5;
				}
				else
				{
					if (!bWasEvading)
					{
						int CurrentAttackIndex = UKismetMathLibrary::RandomIntegerInRange(0, 2);
						GetMesh()->GetAnimInstance()->Montage_Play(DamageMontages[CurrentAttackIndex]);
						CurrentHP -= IDamage;
						CurrentStamina -= 5.f;
					}
				}
			}
			if (CurrentHP <= 0)
			{
				bDead = true;
				CurrentStamina = 0;
				CurrentHP = 0;
				ExecuteDeath();
			}
			CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);
		}
	}
	
}

void ABaseRole::DrawSword()
{
	if (GetMesh()->GetAnimInstance())
	{
		if (!bEquip)
		{
			UAnimInstance* AnimInstace = GetMesh()->GetAnimInstance();
			bEquip = true;
			AnimInstace->Montage_Play(EquipMontage);
			RefreshEnemyTarget(true);
			if (EnemyTarget)
			{
				EnemyTarget->DrawSword();
			}
		}
	}
}

void ABaseRole::ResetAllConditions()
{
	ResetAttacking();
	ResetDoding();
	ResetRoll();
	StopDefending();
	StopRunning();
}

void ABaseRole::ExecuteDeath()
{
	if (!bTrainingMode)
	{
		DisableInput(Cast<APlayerController>(GetController()));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		GetMesh()->SetSimulatePhysics(true);
		GetCharacterMovement()->DisableMovement();
		if (EnemyTarget)
		{
			EnemyTarget->StopTheGame();
		}
		SetActorTickEnabled(false);
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

}

FRotator ABaseRole::CalculateDesireRotation()
{
	FVector LastVector = GetCharacterMovement()->GetLastInputVector();
	if (LastVector != FVector(0))
	{
		return UKismetMathLibrary::MakeRotFromX(LastVector);
	}
	return DesireRotation;
}

void ABaseRole::RinterpRotation(bool Rinterp)
{
	if (Rinterp && !bTrainingMode && !bLock)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Black, TEXT("Rotating"));
		FRotator RinterpRotation = UKismetMathLibrary::RInterpTo(GetActorRotation(), DesireRotation, GetWorld()->GetDeltaSeconds(), 10);
		SetActorRotation(FRotator(0, RinterpRotation.Yaw,0));
	}
}

void ABaseRole::EnableEquip()
{
	bEquip = true;
}

void ABaseRole::ResetRoll()
{
	bRolling = false;
	ResetInjury();
}

bool ABaseRole::IsTouchingWall(float CheckDistance) const
{
	if (!GetWorld())
	{
		return false;
	}

	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		return false;
	}

	const float TraceDistance = FMath::Max(2.0f, CheckDistance);
	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight() * 0.5f);
	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	const FVector Directions[4] = { Forward, -Forward, Right, -Right };

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(IsTouchingWall), false, this);
	if (EnemyTarget)
	{
		QueryParams.AddIgnoredActor(EnemyTarget);
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	for (const FVector& Direction : Directions)
	{
		FHitResult Hit;
		const FVector End = Start + Direction * TraceDistance;
		if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectQueryParams, QueryParams) && Hit.bBlockingHit)
		{
			return true;
		}
	}

	return false;
}

FVector2D ABaseRole::GetEnemyRelativeLocationLocal2D(float LocationScale) const
{
	if (!EnemyTarget)
	{
		return FVector2D::ZeroVector;
	}

	const float SafeScale = FMath::Max(1.0f, LocationScale);
	const FVector RelativeWorld = EnemyTarget->GetActorLocation() - GetActorLocation();
	const FVector RelativeLocal = GetActorTransform().InverseTransformVectorNoScale(RelativeWorld);

	return FVector2D(
		FMath::Clamp(RelativeLocal.X / SafeScale, -1.0f, 1.0f),
		FMath::Clamp(RelativeLocal.Y / SafeScale, -1.0f, 1.0f));
}

float ABaseRole::GetEnemyDistanceNormalized(float MaxDistance) const
{
	if (!EnemyTarget)
	{
		return 1.0f;
	}

	const float SafeMaxDistance = FMath::Max(1.0f, MaxDistance);
	const float Distance = FVector::Distance(GetActorLocation(), EnemyTarget->GetActorLocation());
	return FMath::Clamp(Distance / SafeMaxDistance, 0.0f, 1.0f);
}

void ABaseRole::GetWallDistanceObservations(float TraceDistance, float& FrontNorm, float& RightNorm, float& BackNorm, float& LeftNorm) const
{
	FrontNorm = CachedWallFrontNorm;
	RightNorm = CachedWallRightNorm;
	BackNorm = CachedWallBackNorm;
	LeftNorm = CachedWallLeftNorm;

	if (!GetWorld())
	{
		return;
	}

	if (bEnableWallObservationCache && bTrainingMode && WallObservationCacheCooldown > 0.0f)
	{
		return;
	}

	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}

	const float SafeTraceDistance = FMath::Max(2.0f, TraceDistance);
	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight() * 0.5f);
	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GetWallDistanceObservations), false, this);
	if (EnemyTarget)
	{
		QueryParams.AddIgnoredActor(EnemyTarget);
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	auto TraceDistanceNormalized = [&](const FVector& Direction) -> float
	{
		FHitResult Hit;
		const FVector End = Start + Direction * SafeTraceDistance;
		if (!GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectQueryParams, QueryParams) || !Hit.bBlockingHit)
		{
			return 1.0f;
		}

		return FMath::Clamp(Hit.Distance / SafeTraceDistance, 0.0f, 1.0f);
	};

	CachedWallFrontNorm = TraceDistanceNormalized(Forward);
	CachedWallRightNorm = TraceDistanceNormalized(Right);
	CachedWallBackNorm = TraceDistanceNormalized(-Forward);
	CachedWallLeftNorm = TraceDistanceNormalized(-Right);
	WallObservationCacheCooldown = FMath::Max(0.01f, WallObservationCacheInterval);

	FrontNorm = CachedWallFrontNorm;
	RightNorm = CachedWallRightNorm;
	BackNorm = CachedWallBackNorm;
	LeftNorm = CachedWallLeftNorm;
}

bool ABaseRole::IsLikelyStuck(float SpeedThreshold, float EnemyDistanceThreshold, float WallCheckDistance) const
{
	if (bDead || !EnemyTarget)
	{
		return false;
	}

	const float SafeSpeedThreshold = FMath::Max(0.0f, SpeedThreshold);
	const float SafeEnemyDistance = FMath::Max(1.0f, EnemyDistanceThreshold);
	const bool bTooSlow = GetVelocity().Size2D() <= SafeSpeedThreshold;
	const float EnemyDistance = FVector::Distance(GetActorLocation(), EnemyTarget->GetActorLocation());
	const bool bNearEnemy = EnemyDistance <= SafeEnemyDistance;
	const bool bNearWall = IsTouchingWall(WallCheckDistance);

	return bTooSlow && bNearEnemy && bNearWall;
}

float ABaseRole::GetMovementJitterScore() const
{
	return FMath::Clamp(MovementJitterAccumulator / 5.0f, 0.0f, 1.0f);
}

// Called when the game starts or when spawned
void ABaseRole::BeginPlay()
{
	Super::BeginPlay();
	RefreshEnemyTarget(true);
	if (bTrainingMode)
	{
		DrawSword();
		DisableInput(Cast<APlayerController>(GetController()));
	}
	CurrentStamina = MaxStamina;
	CurrentHP = MaxHP;
	InitialTransform = GetActorTransform();
	EnemyTargetRefreshCooldown = 0.0f;
	TrainingFocusCooldown = 0.0f;
	WallObservationCacheCooldown = 0.0f;
	SpawnProtectionUntilTime = bTrainingMode && GetWorld()
		? GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, SpawnProtectionSeconds)
		: 0.0f;
}

// Called every frame
void ABaseRole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	MovementJitterAccumulator = FMath::Max(0.0f, MovementJitterAccumulator - (DeltaTime * 1.5f));
	EnemyTargetRefreshCooldown = FMath::Max(0.0f, EnemyTargetRefreshCooldown - DeltaTime);
	TrainingFocusCooldown = FMath::Max(0.0f, TrainingFocusCooldown - DeltaTime);
	WallObservationCacheCooldown = FMath::Max(0.0f, WallObservationCacheCooldown - DeltaTime);
	if (!EnemyTarget)
	{
		RefreshEnemyTarget(false);
	}
	// Typically used to make character movement look smooth:
	if (GetVelocity().Size() > 0)
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
	}
	else
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
	}
	//Calculate desire rotation and execute it while attacking
	DesireRotation = CalculateDesireRotation();
	if (EnemyTarget)
	{
		AController* LocalController = GetController();
		if (!LocalController)
		{
			return;
		}

		FRotator FocusRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), EnemyTarget->GetActorLocation());
		//Optional only used for agent training
		if (bTrainingMode && TrainingFocusCooldown <= 0.0f)
		{
			FRotator RinterpFocusRotation = UKismetMathLibrary::RInterpTo(LocalController->GetControlRotation(), FocusRotation, GetWorld()->GetDeltaSeconds(), 15);
			LocalController->SetControlRotation(FRotator(FMath::Clamp(RinterpFocusRotation.Pitch - 5, -30, 30), RinterpFocusRotation.Yaw, RinterpFocusRotation.Roll));
			SetActorRotation(FRotator(GetActorRotation().Pitch, RinterpFocusRotation.Yaw, GetActorRotation().Roll));
			TrainingFocusCooldown = FMath::Max(0.01f, TrainingFocusUpdateInterval);
		}
		if (bLock)
		{
			FRotator RinterpFocusRotation = UKismetMathLibrary::RInterpTo(LocalController->GetControlRotation(), FocusRotation, GetWorld()->GetDeltaSeconds(), 15);
			LocalController->SetControlRotation(FRotator(FMath::Clamp(RinterpFocusRotation.Pitch-5,-30,30), RinterpFocusRotation.Yaw, RinterpFocusRotation.Roll));
			SetActorRotation(FRotator(GetActorRotation().Pitch,RinterpFocusRotation.Yaw, GetActorRotation().Roll));
		}
	}
	else
	{
		RefreshEnemyTarget(false);
	}
	//Stamina mechanism
	if (CurrentStamina < MaxStamina && !bDefend && !bAttacking)
	{
		if (GetVelocity().Size() <= 200)
		{
			CurrentStamina += 15.f * DeltaTime;
		}
		else
		{
			CurrentStamina += 9.f * DeltaTime;
		}
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);
	}
}

// Called to bind functionality to input
void ABaseRole::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &ABaseRole::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABaseRole::MoveRight);
	PlayerInputComponent->BindAxis("MouseX", this, &ABaseRole::MouseX);
	PlayerInputComponent->BindAxis("MouseY", this, &ABaseRole::MouseY);
	PlayerInputComponent->BindAction("Run", EInputEvent::IE_Pressed, this, &ABaseRole::Running);
	PlayerInputComponent->BindAction("Run", EInputEvent::IE_Released, this, &ABaseRole::StopRunning);
	PlayerInputComponent->BindAction("Roll", EInputEvent::IE_Pressed, this, &ABaseRole::Roll);
	PlayerInputComponent->BindAction("Defend", EInputEvent::IE_Pressed, this, &ABaseRole::Defend);
	PlayerInputComponent->BindAction("Defend", EInputEvent::IE_Released, this, &ABaseRole::StopDefending);
	PlayerInputComponent->BindAction("Attack", EInputEvent::IE_Pressed, this, &ABaseRole::Attack);
	PlayerInputComponent->BindAction("Dodge", EInputEvent::IE_Pressed, this, &ABaseRole::Dodge);
	PlayerInputComponent->BindAction("Lock", EInputEvent::IE_Pressed, this, &ABaseRole::LockCameraToTarget);
	PlayerInputComponent->BindAction("DrawSword", EInputEvent::IE_Pressed, this, &ABaseRole::DrawSword);
}
