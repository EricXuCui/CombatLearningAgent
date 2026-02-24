// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseRole.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AIEnemyForTraining.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	MaxHP = 100.f;
	Damage = 15.f;
	MaxStamina = 100.f;
	CurrentStamina = 0.f;
	bDead = false;
	bAttacking = false;
	bRunning = false;
	bRolling = false;
	bDefend = false;
	bDoding = false;
	bTrainingMode = false;
	bEquip = false;
	bInjury = false;
	GetCharacterMovement()->MaxWalkSpeed = 200.f;
	// Initialize Character Settings
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	BaseRoleCameraArm->bUsePawnControlRotation = true;
	BaseRoleCameraArm->bEnableCameraLag = true;
	BaseRoleCameraArm->TargetArmLength = 400.f;
}

void ABaseRole::MoveForward(float Val)
{
	ForwardVal = Val;
	const FRotator LocalRotator = GetControlRotation();
	const FVector LocalInputVector = FRotationMatrix(FRotator(0, LocalRotator.Yaw, 0)).GetUnitAxis(EAxis::X);
	AddMovementInput(LocalInputVector, Val);
}

void ABaseRole::MoveRight(float Val)
{
	RightVal = Val;
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
	if (!bRolling & !bAttacking &!bDoding & bEquip)
	{
		CameraFovExtend(true);
		if (bDefend)
		{
			StopDefending();
		}
		bRunning = true;
		GetCharacterMovement()->MaxWalkSpeed = 500.f;
	}
	else
	{
		StopRunning();
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
	if (!bRolling && !bAttacking && !bDoding && bEquip && CurrentStamina >= 15)
	{
		if (GetMesh()->GetAnimInstance())
		{
			CurrentStamina -= 15;
			if (bRunning)
			{
				StopRunning();
			}
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			bDoding = true;
			bDefend = false;
			float DodingPlayRate = 1.2f;
			if (ForwardVal == 1)
			{
				//Forward
				AnimInstance->Montage_Play(DodgeAnimMontages[0], DodingPlayRate);

			}
			else if (ForwardVal == 0)
			{
				if (RightVal == 1)
				{
					//Right
					AnimInstance->Montage_Play(DodgeAnimMontages[1], DodingPlayRate);
				}
				else if (RightVal == -1)
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

		}

	}
}

void ABaseRole::Roll()
{
	if (!bRolling && !bAttacking && !bDoding && bEquip && CurrentStamina >= 15)
	{
		if (GetMesh()->GetAnimInstance())
		{
			if (bRunning)
			{
				StopRunning();
			}
			CurrentStamina -= 15;
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			bRolling = true;
			bDefend = false;
			float RollingPlayRate = 1.2f;
			if (ForwardVal == 1)
			{
					//Forward
					AnimInstance->Montage_Play(RollAnimMontages[0], RollingPlayRate);
			
			}
			else if (ForwardVal == 0)
			{
				if (RightVal == 1)
				{
					//Right
					AnimInstance->Montage_Play(RollAnimMontages[1], RollingPlayRate);
				}
				else if (RightVal == -1)
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
			
		}

	}
}

void ABaseRole::Attack()
{
	if (!bRolling && !bAttacking && !bDoding && bEquip && CurrentStamina >= 15 && !bInjury)
	{
		if (GetMesh()->GetAnimInstance())
		{
			if (bRunning)
			{
				StopRunning();
			}
			CurrentStamina -= 15;
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

		}
	}
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
	if (!bRolling && !bAttacking && !bDoding && bEquip && !bInjury)
	{
		if (bRunning)
		{
			StopRunning();
		}
		bDefend = true;
		StopRunning();
	}
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
	ActorsToIgnore.Add(GetOwner());

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
			if (HitTarget.GetActor() && HitTarget.GetActor() != GetOwner())
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
	SetTrainingTarget();
	MaxHP = 100.f;
	CurrentHP = 100.f;
	Damage = 15.f;
	MaxStamina = 100.f;
	CurrentStamina = 100.f;
	bDead = false;
	bAttacking = false;
	bRunning = false;
	bRolling = false;
	bDefend = false;
	bDoding = false;
	bTrainingMode = true;
	bEquip = false;
	bInjury = false;
	GetCharacterMovement()->MaxWalkSpeed = 200.f;
	if (EnemyTarget)
	{
		EnemyTarget->ResetTarget();
	}
	SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
	DrawSword();
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
		if (GetMesh()->GetAnimInstance())
		{
			ResetAllConditions();
			bInjury = true;

			if (bDefend)
			{
				if (bUltimateAttack)
				{
					GetMesh()->GetAnimInstance()->Montage_Play(DefenseHeavyDamageMontages);
					if (CurrentStamina >= 15)
					{
						CurrentStamina -= 15.f;
					}
					else
					{
						CurrentStamina = 0.f;
						CurrentHP -= IDamage * 1.5;
					}
				}
				else
				{
					if (CurrentStamina >= 5)
					{
						CurrentStamina -= 5.f;
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
					CurrentStamina -= 15.f;
					CurrentHP -= IDamage * 1.5;
				}
				else
				{
					if (!bRolling && !bDoding)
					{
						int CurrentAttackIndex = UKismetMathLibrary::RandomIntegerInRange(0, 2);
						GetMesh()->GetAnimInstance()->Montage_Play(DamageMontages[CurrentAttackIndex]);
					}
					CurrentHP -= IDamage;
					CurrentStamina -= 5.f;
				}
			}
			if (CurrentHP <= 0)
			{
				bDead = true;
				CurrentStamina = 0;
				CurrentHP = 0;
				ExecuteDeath();
			}
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
			AnimInstace->Montage_Play(EquipMontage);
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
	ResetDoding();
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
	if (Rinterp & !bTrainingMode &!bLock)
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

// Called when the game starts or when spawned
void ABaseRole::BeginPlay()
{
	Super::BeginPlay();
	SetTrainingTarget();
	DrawSword();
	DisableInput(Cast<APlayerController>(GetController()));
	CurrentStamina = MaxStamina;
	CurrentHP = MaxHP;
	InitialTransform = GetActorTransform();
}

// Called every frame
void ABaseRole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
		FRotator FocusRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), EnemyTarget->GetActorLocation());
		//Optional only used for agent training
		if (bTrainingMode)
		{
			if (EnemyTarget)
			{
				const FRotator FocusRot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), EnemyTarget->GetActorLocation());
				const FRotator NewRot =FMath::RInterpTo(GetActorRotation(), FocusRot, DeltaTime, 5.0f);
				SetActorRotation(FRotator(GetActorRotation().Pitch, NewRot.Yaw, GetActorRotation().Roll));
			}
		}
		if (bLock)
		{
			FRotator RinterpFocusRotation = UKismetMathLibrary::RInterpTo(GetController()->GetControlRotation(), FocusRotation, GetWorld()->GetDeltaSeconds(), 15);
			GetController()->SetControlRotation(FRotator(FMath::Clamp(RinterpFocusRotation.Pitch-5,-30,30), RinterpFocusRotation.Yaw, RinterpFocusRotation.Roll));
			SetActorRotation(FRotator(GetActorRotation().Pitch,RinterpFocusRotation.Yaw, GetActorRotation().Roll));
		}
	}
	else
	{
		SetTrainingTarget();
	}
	//Stamina mechanism
	if (CurrentStamina < 100 && !bDefend && !bAttacking)
	{
		if (GetVelocity().Size() <= 200)
		{
			CurrentStamina += 0.25;
		}
		else
		{
			CurrentStamina += 0.15;
			CurrentStamina += 0.15;
		}
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

