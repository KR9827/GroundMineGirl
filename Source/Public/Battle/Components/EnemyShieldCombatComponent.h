// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Battle/Components/EnemyCombatComponent.h"
#include "EnemyShieldCombatComponent.generated.h"


// デリゲートの宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShieldBrokenSignature);

/**
 * 
 */
UCLASS()
class PROJECTJ_API UEnemyShieldCombatComponent : public UEnemyCombatComponent
{
	GENERATED_BODY()
	
public:
	UEnemyShieldCombatComponent();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void ExecuteAttack() override;	
	virtual bool DoAttackTrace_Implementation(FName DamageSourceBone) override;
	virtual void ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType) override;
	
	// ジャスト回避成功時の関数
	virtual void OnJustDodgeSuccess_Implementation(AActor* Defender) override;
	
	// スタン開始・解除用関数
	UFUNCTION(BlueprintCallable, Category = "Combat|Shield")
	void ApplyStun();
	UFUNCTION(BlueprintCallable, Category = "Combat|Shield")
	void ClearStun();
	
	// ANSで呼ぶ初期化関数
	UFUNCTION(BlueprintCallable, Category = "Combat|Shield")
	void ResetTraceLocation();
	
	// 攻撃開始時にプレイヤーとの距離を計算する関数（BTT用）
	UFUNCTION(BlueprintCallable, Category = "Combat|Shield")
	virtual float SelectAttackByDistance() override;
	
	// 盾が破壊された時にBP側で演出を行うためのイベント
	UPROPERTY(BlueprintAssignable, Category = "Combat|Shield")
	FOnShieldBrokenSignature OnShieldBroken;
	
protected:
	virtual bool HandleAttackMontageEnd(UAnimMontage* Montage) override;
	
	// ガードAMが終わった時によばれる関数
	UFUNCTION()
	void OnGuardMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	// 攻撃用AM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Shield")
	TArray<UAnimMontage*> AttackFrontMontage;
	
	// 防御用AM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Shield")
	UAnimMontage* GuardMontage;
	
	// スタン用AM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Shield")
	UAnimMontage* StunMontage;
	
	// スタンの維持時間
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Shield")
	float StunDuration = 5.0f;
	
	// ダメージを受けたアクターを保持する関数
	UPROPERTY()
	TArray<AActor*> AlreadyHitActors;
	
	// 前フレームのハンマーの頭の位置
	FVector PrevSocketLocation;
	
	// トレース開始後の最初のTickか
	bool bIsFirstTraceFrame;
	
	/** 盾関係 */
	// 盾の最大耐久度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Shield")
	float MaxShieldHp = 100.0f;
	
	// 盾の現在耐久度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Shield")
	float CurrentShieldHp;
	
	// 盾が破壊されているか
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Shield")
	bool bIsShieldBroken = false;
	
	// 盾が壊れた時に呼ばれる関数
	virtual void BreakShield();
	
	/** 強制攻撃系 */
	// 強制攻撃に移行するまでのガード回数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Shield")
	int32 MaxGuardHitCount = 3;
	
	// バックステップ用AM
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Shield")
	TObjectPtr<UAnimMontage> BackstepMontage;
	
	// 強制攻撃する際に、追従する場所のオフセット
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Shield")
	float StopOffset = 100.0f;
	
	// 現在のガード数
	int32 CurrentGuardHitCount = 0;
	
	// 強制攻撃中かどうか
	bool bIsForcedAttacking = false;
	
	// 選択された攻撃のインデックスを保持
	int32 SelectAttackIndex = 0;
	
	// バックステップの再生
    void StartForcedAttackSequence();
    	
    // バックステップ終了後に呼ばれる関数
    void OnBackstepEnded(UAnimMontage* Montage, bool bInterrupted);
	
	// MotionWarpingの目標地点を動的に更新する関数
	void UpdateWarpTarget();
	
	/** 攻撃AMの切り替えの距離 */
	// この値以下の時近距離
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Distance")
	float CloseRangeDistance = 300.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Distance")
	float MediumRangeDistance = 600.0f;
	
	// 近距離攻撃時のAttackRange
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Distance")
	float CloseAttackRange = 200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Distance")
	float MediumAttackRange = 400.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Distance")
	float RangedAttackRange = 600.0f;
	
	/** サウンド関係 */
	// ガード時のSE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> GuardSound;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> ShieldBreakSound;
	
private:
	// スタン計測用のタイマー
	FTimerHandle StunTimerHandle;
	
};
