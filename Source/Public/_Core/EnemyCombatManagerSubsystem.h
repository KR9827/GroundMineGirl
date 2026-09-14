// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyCombatManagerSubsystem.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	None,
	Melee,			// 近接
	Ranged,			// 遠距離
	Shield,			// 盾
	Charge,			// 突進
};


UCLASS()
class PROJECTJ_API UEnemyCombatManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// 攻撃できる枠があるか調べる（空きがあればtrueを返して枠を一つ埋める）
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool RequestAttackToken(EEnemyType EnemyType, int32 MaxAttackers);
	
	// 攻撃が終わったら枠を空ける
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ReleaseAttackToken(EEnemyType EnemyType);
	
private:
	TMap<EEnemyType, int32> CurrentAttackersMap;
};
