// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Battle/Components/EnemyCombatComponent.h"
#include "EnemyRangedCombatComponent.generated.h"

/**
 * 
 */

// 前方宣言
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class PROJECTJ_API UEnemyRangedCombatComponent : public UEnemyCombatComponent
{
	GENERATED_BODY()
	
public:
	UEnemyRangedCombatComponent();
	
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void ExecuteAttack() override;
	
	virtual bool DoAttackTrace_Implementation(FName DamageSourceBone) override;
	
	virtual void ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType) override;
	
	/** ANで呼ぶ関数 */
	// 射撃実行
    void FireRangedAttack();
	
	// エイム開始の関数
	void StartAiming();
	
	// 照準線の描画
	void UpdateAimLine(float DeltaTime);
	
	// 追従するプレイヤーの位置を固定
	void LockAim();
	

	// エイム時のエフェクトの色を変更する関数
	UFUNCTION(BlueprintCallable)
	void SetAimLaserColor(const FLinearColor& Color);
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ranged")
	TArray<UAnimMontage*> AttackMontage;
	
	virtual bool HandleAttackMontageEnd(UAnimMontage* Montage) override;
	
	// 照準追従時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	float AimDuration = 1.5f;
	
	// 射撃飛距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	float AttackRange = 2000.0f;
	
	// 発射するソケット名
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	FName MuzzleSocketName = TEXT("hand_l");
	
	// プレイヤーを向く速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	float AimRotationSpeed = 10.0f;
	
	// 与えるダメージ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	float RangedDamage = 40.0f;
	
	// 攻撃するアニメーションで手がプレイヤーを向くように体の向きを動かす角度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	float HandYawOffset = 0.0f;
	
	// エディタでセットするナイアガラ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	TObjectPtr<UNiagaraSystem> LaserNiagaraSystem;
	
	// 発射したときに生成するナイアガラ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	TObjectPtr<UNiagaraSystem> FireLaserNiagaraSystem;
	
	// 攻撃予兆時にレンズを光らせるナイアガラ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	TObjectPtr<UNiagaraSystem> LensGlowNiagaraSystem;
	
	// ナイアガラの始点パラメータ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	FName BeamStartParamName = FName("User.Beam Start");
	
	// ナイアガラの終点パラメータ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	FName BeamEndParamName = FName("User.Beam End");
	
	// 当たり判定に接触したあとの延長距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	float AimLineExtensionDistance = 50.0f;
	
	// 固定された発射先の位置
	FVector LockedTargetPos = FVector::ZeroVector;
	
	
private:
	// エイム中フラグ
	bool bIsAiming = false;
	
	// エイム計測用タイマー
	FTimerHandle AimTimerHandle;
	
	// 生成したナイアガラコンポーネントの保持
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedLaserComp;
	
	// 生成したレンズの光のナイアガラコンポネントの保持
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedLensGlowComp;
	
	// エイム状態や予兆UIを安全にクリアする
	void CancelAiming();
	
	// レーザーエフェクトを安全に停止・破棄する
	void StopLaserEffect();
};
