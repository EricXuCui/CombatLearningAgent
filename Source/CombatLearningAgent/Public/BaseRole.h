// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "BaseRole.generated.h"

class AAIEnemyForTraining;//Declare enemy forward to avoid header file circular dependencies

UCLASS()
class COMBATLEARNINGAGENT_API ABaseRole : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseRole();
	// CPP Components
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPComponent")
	USpringArmComponent* BaseRoleCameraArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPComponent")
	UCameraComponent* BaseRoleCamera;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPComponent")
	UArrowComponent * WeaponArrowComponent;
	// CPP Variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	TSubclassOf<class AAIEnemyForTraining>  InstanceOfEnemy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	AAIEnemyForTraining * EnemyTarget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	float MaxHP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	float CurrentHP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	float Damage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bDead;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bDefend;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bLock;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bTrainingMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bEquip;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	float MaxStamina;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	float CurrentStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bRunning;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bAttacking;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bRolling;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bDoding;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bInjury;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	TSubclassOf<ACharacter> CharacterToSpawn;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CPPVariables")
	ABaseRole* PlayerPawn;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CPPVariables")
	FRotator DesireRotation;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CPPVariables")
	FTransform InitialTransform;

	//CPP Animation Montages
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	TArray<UAnimMontage*> RollAnimMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	TArray<UAnimMontage*> AttackAnimMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	TArray<UAnimMontage*> DodgeAnimMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	UAnimMontage * EquipMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	UAnimMontage * DefendAnimMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	TArray<UAnimMontage*>  DamageMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	UAnimMontage* HeavyDamageMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	UAnimMontage* DefenseHeavyDamageMontages;
		
	// Character Movements
	UFUNCTION(BlueprintCallable)
	void MoveForward(float Val);

	UFUNCTION(BlueprintCallable)
	void MoveRight(float Val);

	void MouseX(float Val);
	void MouseY(float Val);

	UFUNCTION(BlueprintCallable)
	void Running();
	UFUNCTION(BlueprintCallable)
	void StopRunning();

	//Mechanisms
	UFUNCTION(BlueprintCallable)
	void Dodge();
	UFUNCTION(BlueprintCallable)
	void Roll();
	UFUNCTION(BlueprintCallable)
	void Attack();
	UFUNCTION(BlueprintCallable)
	void Defend();
	UFUNCTION(BlueprintCallable)
	void StopDefending();

	void LockCameraToTarget();
	void ReceiveDamage(float IDamage,bool bUltimateAttack);
	void DrawSword();
	void ResetAllConditions();
	void ExecuteDeath();
	FRotator CalculateDesireRotation();


	UFUNCTION(BlueprintCallable)
	void ResetRoll();
	UFUNCTION(BlueprintCallable)
	void ResetAttacking();
	UFUNCTION(BlueprintCallable)
	void ResetDoding();
	UFUNCTION(BlueprintCallable)
	void RinterpRotation(bool Rinterp);
	UFUNCTION(BlueprintCallable)
	void EnableEquip();
	UFUNCTION(BlueprintCallable)
	void AttackTrace();
	UFUNCTION(BlueprintCallable)
	void ResetInjury();
	UFUNCTION(BlueprintImplementableEvent)
	void CameraFovExtend(bool bEnable);
	UFUNCTION(BlueprintCallable)
	void ResetBaseRoleAgent();
	UFUNCTION(BlueprintCallable)
	void ApplyCurriculumScalars(float InHPScale, float InDamageScale, float InStaminaScale);
	UFUNCTION(BlueprintCallable)
	void ApplyCurriculumForEpisode(int32 EpisodeIndex);
	UFUNCTION(BlueprintPure, Category = "Learning|Reward")
	bool IsTouchingWall(float CheckDistance = 18.0f) const;
	UFUNCTION(BlueprintPure, Category = "Learning|Observation")
	FVector2D GetEnemyRelativeLocationLocal2D(float LocationScale = 10000.0f) const;
	UFUNCTION(BlueprintPure, Category = "Learning|Observation")
	float GetEnemyDistanceNormalized(float MaxDistance = 2000.0f) const;
	UFUNCTION(BlueprintPure, Category = "Learning|Observation")
	void GetWallDistanceObservations(float TraceDistance, float& FrontNorm, float& RightNorm, float& BackNorm, float& LeftNorm) const;
	UFUNCTION(BlueprintPure, Category = "Learning|Reward")
	bool IsLikelyStuck(float SpeedThreshold = 20.0f, float EnemyDistanceThreshold = 140.0f, float WallCheckDistance = 18.0f) const;
	UFUNCTION(BlueprintPure, Category = "Learning|Reward")
	float GetMovementJitterScore() const;
	void RefreshEnemyTarget(bool bForce = false);

	FTransform ReturnSpawnTransform();
	//Internal CPP Variables
	float ForwardVal;
	float RightVal;
	float LastForwardVal;
	float LastRightVal;
	float MovementJitterAccumulator;
	int AttackIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum")
	float BaseMaxHP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum")
	float BaseDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum")
	float BaseMaxStamina;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum")
	float CurriculumHPScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum")
	float CurriculumDamageScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum")
	float CurriculumStaminaScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum Scheduler")
	int32 CurriculumWarmupEpisodes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum Scheduler")
	int32 CurriculumRampEpisodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum Scheduler")
	float WarmupPlayerHPScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum Scheduler")
	float WarmupPlayerDamageScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum Scheduler")
	float WarmupPlayerStaminaScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum Scheduler")
	float WarmupEnemyHPScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curriculum Scheduler")
	float WarmupEnemyDamageScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning|Performance")
	float EnemyTargetRefreshInterval;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning|Performance")
	bool bEnableWallObservationCache;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning|Performance")
	float WallObservationCacheInterval;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning|Performance")
	float TrainingFocusUpdateInterval;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning|Performance")
	float SpawnProtectionSeconds;

private:
	float EnemyTargetRefreshCooldown;
	float TrainingFocusCooldown;
	mutable float CachedWallFrontNorm;
	mutable float CachedWallRightNorm;
	mutable float CachedWallBackNorm;
	mutable float CachedWallLeftNorm;
	mutable float WallObservationCacheCooldown;
	float SpawnProtectionUntilTime;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
