// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BtlEnemyAICalculations.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FEnemyMoveResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	FVector Location;
	
	UPROPERTY(BlueprintReadWrite)
	float MoveAngle;
	
	UPROPERTY(BlueprintReadWrite)
	float Speed;
};

UCLASS()
class PROJECTJ_API UBtlEnemyAICalculations : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:	
	/// ターゲットの周りの移動目標地点を計算する
	/// @param TargetLocation ターゲットのいる位置（移動する円の中心座標）
	/// @param CurrentLocation 自分の位置
	/// @param CurrentForward キャラクターの正面方向
	/// @param Radius ターゲットからの距離
	/// @param AngleDegree 左右に何度回転した位置に移動するか（プラスは時計周り、マイナスは反時計回り）
	/// @param MaxWalkSpeed 移動速度
	/// @return 移動する位置
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	static FEnemyMoveResult GetCircleLocation(FVector TargetLocation, FVector CurrentLocation, FVector CurrentForward, float Radius, float AngleDegree, float MaxWalkSpeed);
};
