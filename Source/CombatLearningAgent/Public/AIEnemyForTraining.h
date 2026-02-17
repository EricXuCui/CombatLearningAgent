// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/ArrowComponent.h"
#include "AIEnemyController.h"
#include "TimerManager.h"
#include "GameFramework/PlayerStart.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "AIEnemyForTraining.generated.h"


class ABaseRole;//Declare enemy forward to avoid header file circular dependencies
UCLASS()
class COMBATLEARNINGAGENT_API AAIEnemyForTraining : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAIEnemyForTraining();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	float MaxHP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	float Damage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bDead;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	TSubclassOf<ABaseRole> InstanceOfEnemy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	ABaseRole * EnemyTarget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bTrainingMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	bool bEquip;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	float CurrentHP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPComponent")
	UArrowComponent* WeaponArrowComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	FVector TargetLocation;
	AAIEnemyController* AIController;

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
	AAIEnemyForTraining* TargetPawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPVariables")
	UBehaviorTree * BehaviorTree;

	int AttackIndex;
	int RandomStrafeValue;
	bool bUltimateSkillLoop;
	FTimerHandle _DelayRandomStrafeMovement;
	FTimerHandle _DelayRandomUltimateAttack;
	FTimerHandle _DelayUltimateAttackShifting;
	//CPP Animation Montages
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	TArray<UAnimMontage*> AttackAnimMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	TArray<UAnimMontage*> RolllAndDodgeAnimMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	UAnimMontage* EquipMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	TArray<UAnimMontage*>  DamageMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	TArray<UAnimMontage*> UltimateAttackMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CPPAnimations")
	bool bUltimateAttacking;

	void DrawSword();
	void Attack();
	void RunningMovement(bool Run);
	void EnableStrafe();
	void StrafeMovement(int RandomStrafeDirections);
	void UltimateAttack();
	void StopTheGame();
	void ExecuteDeath();
	UFUNCTION(BlueprintImplementableEvent)
	void StartShifting();
	UFUNCTION(BlueprintCallable)
	void ResetAttacking();
	UFUNCTION(BlueprintCallable)
	void ResetDoding();
	UFUNCTION(BlueprintCallable)
	void ResetRoll();
	UFUNCTION(BlueprintCallable)
	void ResetAllConditions();
	UFUNCTION(BlueprintCallable)
	void EnableEquip();
	UFUNCTION(BlueprintCallable)
	void ResetUltimateAttack();
	UFUNCTION(BlueprintCallable)
	void UltimateAttackShifting(float Lerp);
	UFUNCTION(BlueprintCallable)
	void ReceiveDamage(float IDamage);
	UFUNCTION(BlueprintCallable)
	void AttackTrace();
	UFUNCTION(BlueprintCallable)
	void ResetInjury();
	void ResetTarget(ABaseRole * target);
	FTransform ReturnSpawnTransform();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
