// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Battle/Components/EnemyCombatComponent.h"
#include "EnemyMeleeCombatComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTJ_API UEnemyMeleeCombatComponent : public UEnemyCombatComponent
{
	GENERATED_BODY()
	
public:
	UEnemyMeleeCombatComponent();
	
	virtual void ExecuteAttack() override;
	
	virtual bool DoAttackTrace_Implementation(FName DamageSourceBone) override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Melee")
	TArray<UAnimMontage*> AttackFrontMontage;
	
	virtual bool HandleAttackMontageEnd(UAnimMontage* Montage) override;
};
