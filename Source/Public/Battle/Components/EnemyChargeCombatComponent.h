// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Battle/Components/EnemyCombatComponent.h"
#include "EnemyChargeCombatComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTJ_API UEnemyChargeCombatComponent : public UEnemyCombatComponent
{
	GENERATED_BODY()
	
public:
	UEnemyChargeCombatComponent();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void ExecuteAttack() override;	
	virtual bool DoAttackTrace_Implementation(FName DamageSourceBone) override;
	virtual void ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType) override;
	
	// 攻撃開始時にプレイヤーとの距離を計算する関数（BTT用）
	UFUNCTION(BlueprintCallable, Category = "Combat|Charge")
	virtual float SelectAttackByDistance() override;
	
	// 突進中にプレイヤーとの距離を計算する関数
	UFUNCTION(BlueprintCallable, Category = "Combat|Charge")
	void UpdateChargingPlayerToDistance(float DeltaTime);
	
protected:
	virtual bool HandleAttackMontageEnd(UAnimMontage* Montage) override;
	
	// MotionWarpingの目標地点を動的に更新する関数
	void WarpTarget();
	
	/** 攻撃AM */
	// 近距離
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charge")
	TObjectPtr<UAnimMontage> CloseAttackMontage;
	
	// 突進前の予備動作
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charge")
	TObjectPtr<UAnimMontage> PreparatoryActionMontage;
	
	// 突進（中・遠距離）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charge")
	TObjectPtr<UAnimMontage> ChargeMontage;
	
	// 突進後攻撃
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charge")
	TObjectPtr<UAnimMontage> ChargeAttackMontage;
	
	/** 攻撃AMの切り替えの距離、攻撃を開始する距離 */
	// この値以下の時近距離
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Distance")
	float CloseRangeDistance = 300.0f;
	
	/** 攻撃時のAttackRange */
	// 近距離
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Distance")
	float CloseAttackRange = 200.0f;
	
	// 突進時
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Distance")
	float ChargeAttackRange = 1000.0f;
	
	// 突進後攻撃をするプレイヤーとの距離
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Distance")
	float ChargeAttackStartDistance = 300.0f;
	
	// 突進の維持時間
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charge")
    float ChargeDuration = 5.0f;
	
	// 突進時にプレイヤーの方向を向く速度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charge")
	float RotationInterpolationSpeed = 5.0f;
	
	// 突進時の速度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charge")
	float ChargeMoveSpeed = 1200.0f;
	
private:
	// 近距離攻撃時の処理
	void CloseAttack();
	
	// 突進攻撃時の処理
	void ChargeAttack();
	
	// 突進開始関数
	void StartCharge(UAnimMontage* Montage, bool bInterrupted);
	
	// 突進後の攻撃の関数
	void StartChargeAttack();
	
	// 突進攻撃かどうか
	bool bIsChargeAttack = false;
	
	// 突進経過時間を保持する変数
	float CurrentChargeTime = 0.0f;
	
	// 突進用のタイマー
	FTimerHandle ChargeTimerHandle;
};
