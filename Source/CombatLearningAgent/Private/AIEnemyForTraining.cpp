                              // Fill out your copyright notice in the Description page of Project Settings.


#include "AIEnemyForTraining.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BaseRole.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
// Sets default values
AAIEnemyForTraining::AAIEnemyForTraining()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	WeaponArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("WeaponArrow"));
	WeaponArrowComponent->SetupAttachment(GetMesh());
	// Initialize Variables
	MaxHP = 200.f;
	Damage = 15.f;
	bDead = false;
	bAttacking = false;
	bRolling = false;
	bTrainingMode = false;
	bEquip = false;
	RandomStrafeValue = 1;
	bUltimateAttacking = false;
	bUltimateSkillLoop = false;
	CurrentHP = MaxHP;
	GetCharacterMovement()->MaxWalkSpeed = 235.f;
}

void AAIEnemyForTraining::DrawSword()
{
	if (GetMesh()->GetAnimInstance())
	{
		if (!bEquip)
		{
			EnemyTarget = Cast<ABaseRole>(UGameplayStatics::GetActorOfClass(GetWorld(), InstanceOfEnemy));
			UAnimInstance* AnimInstace = GetMesh()->GetAnimInstance();
			AnimInstace->Montage_Play(EquipMontage);
			bEquip = true;
			if (AIController)
			{
				AIController->GetBlackboardComponent()->SetValueAsBool(TEXT("Equip"), true);
				GetWorld()->GetTimerManager().SetTimer(_DelayRandomStrafeMovement, this, &AAIEnemyForTraining::EnableStrafe,
					UKismetMathLibrary::RandomIntegerInRange(10, 15), true, UKismetMathLibrary::RandomIntegerInRange(10, 15));
				GetWorld()->GetTimerManager().SetTimer(_DelayRandomUltimateAttack, this, &AAIEnemyForTraining::UltimateAttack,
					UKismetMathLibrary::RandomIntegerInRange(10, 20), true, UKismetMathLibrary::RandomIntegerInRange(10, 20));
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
		AIController->GetBlackboardComponent()->SetValueAsBool("Strafe", false);
		SetActorTickEnabled(false);
		AIController->GetBlackboardComponent()->SetValueAsBool("StrafeDoOnce", false);
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = 235.f;
	}
}

void AAIEnemyForTraining::EnableStrafe()
{
	AIController->GetBlackboardComponent()->SetValueAsBool(TEXT("Strafe"), true);
	RunningMovement(false);
	AIController->GetBlackboardComponent()->SetValueAsBool("Run", false);
	if (UKismetMathLibrary::RandomBool())
	{
		RandomStrafeValue = 1;
	}
	else
	{
		RandomStrafeValue = -1;
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
	if (GetMesh()->GetAnimInstance())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		ResetAllConditions();
		if (AIController)
		{
			AIController->GetBlackboardComponent()->SetValueAsBool("Strafe", false);
			AIController->GetBlackboardComponent()->SetValueAsBool("StrafeDoOnce", false);
			AIController->GetBlackboardComponent()->SetValueAsBool("Attack", false);
			AIController->GetBlackboardComponent()->SetValueAsBool("Run", false);
		}
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
	ResetAllConditions();

}

void AAIEnemyForTraining::ExecuteDeath()
{
	StopTheGame();
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);
	SetActorTickEnabled(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

}

void AAIEnemyForTraining::ResetInjury()
{
	bInjury = false;
}


void AAIEnemyForTraining::ResetTarget(ABaseRole * target)
{
	TSubclassOf<AAIEnemyForTraining> Target;
	FTransform SpawnTransform = ReturnSpawnTransform();
	TargetPawn = Cast<AAIEnemyForTraining>(UAIBlueprintHelperLibrary::SpawnAIFromClass(GetWorld(), Target, BehaviorTree, SpawnTransform.GetLocation(), SpawnTransform.Rotator()));
	TargetPawn->EnemyTarget = target;
	this->Destroy();
}

FTransform AAIEnemyForTraining::ReturnSpawnTransform()
{
	TSubclassOf<APlayerStart> Start;
	Start = APlayerStart::StaticClass();
	TArray<AActor*> SpawnPoints;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), Start, FName("TargetStart"), SpawnPoints);
	if (SpawnPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No PlayerStart found with tag PlayerStart!"));
		return FTransform();
	}
	int RandomSpawnPoints = UKismetMathLibrary::RandomIntegerInRange(0, SpawnPoints.Num() - 1);
	return  SpawnPoints[RandomSpawnPoints]->GetTransform();
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
	if (!bDead && !bInjury && !bUltimateAttacking)
	{
		CurrentHP -= IDamage;
		bInjury = true;
		ResetAllConditions();
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Black, FString::SanitizeFloat(CurrentHP));
		if (CurrentHP <= 0)
		{
			bDead = true;
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Black, TEXT("Win"));
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
		ActorsToIgnore.Add(GetOwner());

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
				if (HitActor && HitActor != GetOwner())
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
		EnemyTarget->ReceiveDamage(Damage, bUltimateAttacking);
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
	EnemyTarget = Cast<ABaseRole>(UGameplayStatics::GetActorOfClass(GetWorld(), InstanceOfEnemy));
	CurrentHP = MaxHP;
	AIController = Cast<AAIEnemyController>(GetController());
	if (AIController)
	{
		AIController->GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), EnemyTarget);
	}
	SetActorTickEnabled(false);
}

// Called every frame
void AAIEnemyForTraining::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	StrafeMovement(RandomStrafeValue);
}

// Called to bind functionality to input
void AAIEnemyForTraining::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

