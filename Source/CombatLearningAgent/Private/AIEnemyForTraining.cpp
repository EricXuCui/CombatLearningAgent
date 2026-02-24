                              // Fill out your copyright notice in the Description page of Project Settings.


#include "AIEnemyForTraining.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BaseRole.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"

namespace
{
	float SanitizeCurriculumScale(float Scale)
	{
		return FMath::Max(0.1f, Scale);
	}

	void SetBlackboardBoolSafe(AAIEnemyController* Controller, const TCHAR* KeyName, bool bValue)
	{
		if (Controller && Controller->GetBlackboardComponent())
		{
			Controller->GetBlackboardComponent()->SetValueAsBool(KeyName, bValue);
		}
	}
}
// Sets default values
AAIEnemyForTraining::AAIEnemyForTraining()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	WeaponArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("WeaponArrow"));
	WeaponArrowComponent->SetupAttachment(GetMesh());
	// Initialize Variables
	BaseMaxHP = 200.f;
	BaseDamage = 15.f;
	CurriculumHPScale = 1.f;
	CurriculumDamageScale = 1.f;
	MaxHP = BaseMaxHP;
	Damage = BaseDamage;
	bDead = false;
	bAttacking = false;
	bRolling = false;
	bTrainingMode = false;
	bEquip = false;
	RandomStrafeValue = 1;
	bUltimateAttacking = false;
	bUltimateSkillLoop = false;
	AttackIndex = 0;
	StrafeIntervalSeconds = 3.0f;
	StrafeInitialDelaySeconds = 1.0f;
	UltimateIntervalSeconds = 6.0f;
	UltimateInitialDelaySeconds = 2.0f;
	EnemyTargetRefreshInterval = 0.5f;
	SpawnProtectionSeconds = 0.35f;
	EnemyTargetRefreshCooldown = 0.0f;
	SpawnProtectionUntilTime = 0.0f;
	CurrentHP = MaxHP;
	GetCharacterMovement()->MaxWalkSpeed = 235.f;
}

void AAIEnemyForTraining::RefreshEnemyTarget(bool bForce)
{
	if (IsValid(EnemyTarget))
	{
		return;
	}
	EnemyTarget = nullptr;

	if (!bForce && EnemyTargetRefreshCooldown > 0.0f)
	{
		return;
	}

	if (!GetWorld())
	{
		EnemyTargetRefreshCooldown = FMath::Max(0.05f, EnemyTargetRefreshInterval);
		return;
	}

	UClass* EnemyClass = InstanceOfEnemy ? InstanceOfEnemy.Get() : ABaseRole::StaticClass();
	if (!EnemyClass)
	{
		EnemyTargetRefreshCooldown = FMath::Max(0.05f, EnemyTargetRefreshInterval);
		return;
	}

	TArray<AActor*> EnemyCandidates;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), EnemyClass, EnemyCandidates);

	ABaseRole* BestTarget = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	const FVector SelfLocation = GetActorLocation();
	for (AActor* Candidate : EnemyCandidates)
	{
		ABaseRole* RoleCandidate = Cast<ABaseRole>(Candidate);
		if (!IsValid(RoleCandidate) || RoleCandidate->bDead)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(SelfLocation, RoleCandidate->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestTarget = RoleCandidate;
		}
	}

	EnemyTarget = BestTarget;
	EnemyTargetRefreshCooldown = FMath::Max(0.05f, EnemyTargetRefreshInterval);

	if (AIController && AIController->GetBlackboardComponent())
	{
		AIController->GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), EnemyTarget);
	}
}

void AAIEnemyForTraining::ApplyCurriculumScalars(float InHPScale, float InDamageScale)
{
	CurriculumHPScale = SanitizeCurriculumScale(InHPScale);
	CurriculumDamageScale = SanitizeCurriculumScale(InDamageScale);
	MaxHP = BaseMaxHP * CurriculumHPScale;
	Damage = BaseDamage * CurriculumDamageScale;
	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
}

void AAIEnemyForTraining::DrawSword()
{
	if (GetMesh()->GetAnimInstance())
	{
		if (!bEquip)
		{
			RefreshEnemyTarget(true);
			UAnimInstance* AnimInstace = GetMesh()->GetAnimInstance();
			AnimInstace->Montage_Play(EquipMontage);
			bEquip = true;
			if (AIController)
			{
				SetBlackboardBoolSafe(AIController, TEXT("Equip"), true);
				const float SafeStrafeInterval = FMath::Max(0.25f, StrafeIntervalSeconds);
				const float SafeStrafeDelay = FMath::Max(0.0f, StrafeInitialDelaySeconds);
				const float SafeUltimateInterval = FMath::Max(0.5f, UltimateIntervalSeconds);
				const float SafeUltimateDelay = FMath::Max(0.0f, UltimateInitialDelaySeconds);
				GetWorld()->GetTimerManager().SetTimer(_DelayRandomStrafeMovement, this, &AAIEnemyForTraining::EnableStrafe, SafeStrafeInterval, true, SafeStrafeDelay);
				GetWorld()->GetTimerManager().SetTimer(_DelayRandomUltimateAttack, this, &AAIEnemyForTraining::UltimateAttack, SafeUltimateInterval, true, SafeUltimateDelay);
			}
		}
	}
}

void AAIEnemyForTraining::Attack()
{
	if(!bRolling && !bAttacking && !bDoding && bEquip && !bInjury && !bUltimateAttacking)
	{
		if (GetMesh()->GetAnimInstance())
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (0 == UKismetMathLibrary::RandomIntegerInRange(0, 4))
			{
				bDoding = true;
				bRolling = true;
				int RollingAndDodgeIndex = UKismetMathLibrary::RandomIntegerInRange(0, 5);
				AnimInstance->Montage_Play(RolllAndDodgeAnimMontages[RollingAndDodgeIndex],1.2f);
			}
			else
			{
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
			}

		}
	}
}

void AAIEnemyForTraining::RunningMovement(bool Run)
{
	if (Run)
	{
		GetCharacterMovement()->MaxWalkSpeed = 450.f;
		SetBlackboardBoolSafe(AIController, TEXT("Strafe"), false);
		SetActorTickEnabled(false);
		SetBlackboardBoolSafe(AIController, TEXT("StrafeDoOnce"), false);
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = 235.f;
	}
}

void AAIEnemyForTraining::EnableStrafe()
{
	if (bEquip)
	{
		SetActorTickEnabled(true);
		SetBlackboardBoolSafe(AIController, TEXT("Strafe"), true);
		RunningMovement(false);
		SetBlackboardBoolSafe(AIController, TEXT("Run"), false);
		if (UKismetMathLibrary::RandomBool())
		{
			RandomStrafeValue = 1;
		}
		else
		{
			RandomStrafeValue = -1;
		}
	}

}

void AAIEnemyForTraining::StrafeMovement(int RandomStrafeDirections)
{
	const FRotator LocalRotator = GetActorRotation();
	const FVector LocalInputVector = FRotationMatrix(FRotator(0, LocalRotator.Yaw, 0)).GetUnitAxis(EAxis::Y);
	AddMovementInput(LocalInputVector, RandomStrafeDirections);
}

void AAIEnemyForTraining::UltimateAttack()
{
	if (GetMesh()->GetAnimInstance() && bEquip)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		ResetAllConditions();
		SetBlackboardBoolSafe(AIController, TEXT("Strafe"), false);
		SetBlackboardBoolSafe(AIController, TEXT("StrafeDoOnce"), false);
		SetBlackboardBoolSafe(AIController, TEXT("Attack"), false);
		SetBlackboardBoolSafe(AIController, TEXT("Run"), false);
		bUltimateAttacking = true;
		RunningMovement(false);
		SetActorTickEnabled(false);
		GetWorld()->GetTimerManager().SetTimer(_DelayUltimateAttackShifting, this, &AAIEnemyForTraining::StartShifting, 0.8f, false, 0.8f);
		ResetInjury();
		if (bUltimateSkillLoop)
		{
			bUltimateSkillLoop = false;
			AnimInstance->Montage_Play(UltimateAttackMontages[0]);
		}
		else
		{
			bUltimateSkillLoop = true;
			AnimInstance->Montage_Play(UltimateAttackMontages[1]);
			LaunchCharacter(FVector(0, 0, 700.f), true, true);
		}
	}
}

void AAIEnemyForTraining::StopTheGame()
{
	if (AIController)
	{
		AIController->StopMovement();
		UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIController->BrainComponent);
		if (BTComp)
		{
			BTComp->StopLogic(TEXT("This round is over"));
		}	
	}
	GetWorld()->GetTimerManager().ClearTimer(_DelayRandomStrafeMovement);
	GetWorld()->GetTimerManager().ClearTimer(_DelayRandomUltimateAttack);
	GetWorld()->GetTimerManager().ClearTimer(_DelayUltimateAttackShifting);
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->StopAllMontages(0.1f);
	}
	ResetAllConditions();

}

void AAIEnemyForTraining::ExecuteDeath()
{
	if (!bTrainingMode)
	{
		StopTheGame();
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		GetMesh()->SetSimulatePhysics(true);
		SetActorTickEnabled(false);
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

}

void AAIEnemyForTraining::ResetInjury()
{
	bInjury = false;
}


void AAIEnemyForTraining::ResetTarget()
{
	GetWorld()->GetTimerManager().ClearTimer(_DelayRandomStrafeMovement);
	GetWorld()->GetTimerManager().ClearTimer(_DelayRandomUltimateAttack);
	GetWorld()->GetTimerManager().ClearTimer(_DelayUltimateAttackShifting);
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->StopAllMontages(0.1f);
	}

	ApplyCurriculumScalars(CurriculumHPScale, CurriculumDamageScale);
	CurrentHP = MaxHP;
	bDead = false;
	bAttacking = false;
	bRolling = false;
	bDoding = false;
	bTrainingMode = true;
	bEquip = false;
	bInjury = false;
	if (bTrainingMode && GetWorld())
	{
		SpawnProtectionUntilTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, SpawnProtectionSeconds);
	}
	else
	{
		SpawnProtectionUntilTime = 0.0f;
	}
	ResetAllConditions();
	SetBlackboardBoolSafe(AIController, TEXT("Strafe"), false);
	SetBlackboardBoolSafe(AIController, TEXT("StrafeDoOnce"), false);
	SetBlackboardBoolSafe(AIController, TEXT("Attack"), false);
	SetBlackboardBoolSafe(AIController, TEXT("Run"), false);
	if (AIController)
	{
		UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIController->BrainComponent);
		if (BTComp)
		{
			BTComp->RestartLogic();
		}
	}
	GetCharacterMovement()->MaxWalkSpeed = 235.f;
	SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorTickEnabled(true);

}


void AAIEnemyForTraining::ResetAttacking()
{
	bAttacking = false;
	bUltimateAttacking = false;
}

void AAIEnemyForTraining::ResetDoding()
{
	bDoding = false;
}

void AAIEnemyForTraining::ResetRoll()
{
	bRolling = false;
}

void AAIEnemyForTraining::ResetAllConditions()
{
	ResetAttacking();
	ResetDoding();
	ResetRoll();
}

void AAIEnemyForTraining::ReceiveDamage(float IDamage)
{
	if (!bDead && !bInjury)
	{
		if (bTrainingMode && GetWorld() && GetWorld()->GetTimeSeconds() < SpawnProtectionUntilTime)
		{
			return;
		}

		CurrentHP -= IDamage;
		bInjury = true;
		ResetAllConditions();
		if (CurrentHP <= 0)
		{
			bDead = true;
			ExecuteDeath();
		}
		if (!bRolling && !bDoding)
		{
			if (GetMesh()->GetAnimInstance())
			{
				int CurrentAttackIndex = UKismetMathLibrary::RandomIntegerInRange(0, 2);
				GetMesh()->GetAnimInstance()->Montage_Play(DamageMontages[CurrentAttackIndex]);
			}
		}
	}
}

void AAIEnemyForTraining::AttackTrace()
{

	if (!bUltimateAttacking)
	{
		FVector ArrowLocation = WeaponArrowComponent->GetComponentLocation();
		FVector ArrowForwardLocation = WeaponArrowComponent->GetForwardVector() * 100;

		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(this);

		TArray<FHitResult> HitResults;

		const bool bHit = UKismetSystemLibrary::SphereTraceMulti(
			GetWorld(),
			ArrowLocation,
			ArrowLocation + ArrowForwardLocation,
			60.f,
			UEngineTypes::ConvertToTraceType(ECC_Pawn),
			false,
			ActorsToIgnore,
			EDrawDebugTrace::None,
			HitResults,
			true
		);
		if (bHit)
		{
			for (const FHitResult& Hit : HitResults)
			{
				AActor* HitActor = Hit.GetActor();
				if (HitActor && HitActor != this)
				{
					ABaseRole* Target = Cast<ABaseRole>(HitActor);
					if (Target)
					{
						Target->ReceiveDamage(Damage, false);
					}
				}
			}
		}
	}
	else
	{
		if (EnemyTarget)
		{
			EnemyTarget->ReceiveDamage(Damage, bUltimateAttacking);
		}
	}
}


void AAIEnemyForTraining::EnableEquip()
{
	bEquip = true;
}

void AAIEnemyForTraining::ResetUltimateAttack()
{
	bUltimateAttacking = false;
}

void AAIEnemyForTraining::UltimateAttackShifting(float Lerp)
{
	FVector LerpLocation = UKismetMathLibrary::VLerp(GetActorLocation(), TargetLocation, Lerp);
	SetActorLocation(FVector(LerpLocation.X, LerpLocation.Y, GetActorLocation().Z));
}

// Called when the game starts or when spawned
void AAIEnemyForTraining::BeginPlay()
{
	Super::BeginPlay();
	RefreshEnemyTarget(true);
	CurrentHP = MaxHP;
	AIController = Cast<AAIEnemyController>(GetController());
	if (AIController)
	{
		AIController->GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), EnemyTarget);
	}
	EnemyTargetRefreshCooldown = 0.0f;
	SpawnProtectionUntilTime = bTrainingMode && GetWorld()
		? GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, SpawnProtectionSeconds)
		: 0.0f;
	SetActorTickEnabled(false);
	InitialTransform = GetActorTransform();
}

// Called every frame
void AAIEnemyForTraining::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	EnemyTargetRefreshCooldown = FMath::Max(0.0f, EnemyTargetRefreshCooldown - DeltaTime);
	if (!EnemyTarget)
	{
		RefreshEnemyTarget(false);
	}
	StrafeMovement(RandomStrafeValue);
	
}

// Called to bind functionality to input
void AAIEnemyForTraining::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
