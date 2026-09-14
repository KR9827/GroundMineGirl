// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseCombatComponent.h"
#include "Battle/Characters/BtlPlayerCharacter.h"
#include "Battle/Frameworks/EnemyState.h"
#include "EnemyCombatComponent.generated.h"

class ACharacter;
class UAnimMontage;
class AEnemyAIController;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTJ_API UEnemyCombatComponent : public UBaseCombatComponent
{
	GENERATED_BODY()

public:	
	UEnemyCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
protected:
	virtual void BeginPlay() override;
	
	// === キャッシュ用 ===
	UPROPERTY()
	TObjectPtr<ACharacter> OwnerChar;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Player")
	ABtlPlayerCharacter* PlayerChar = nullptr;
	
	//UPROPERTY()
	//class AAIController* AIC;
	
	UPROPERTY()
	AEnemyAIController* EAIC;
	
	UPROPERTY()
   	class UBlackboardComponent* BB;	
	
protected:
	
	// === 更新処理 ===
	//UFUNCTION(BlueprintCallable, Category="Combat | Rotation")
	//void UpdateFacingRotation(float DeltaTime);
	
	// === 設定用パラメータ ====
	/** 攻撃時にプレイヤーの方向に向く速度 */
	//UPROPERTY(EditAnywhere, Category = "Combat")
	//float AttackRotationSpeed = 20.f;
	
	/** 前方攻撃のモンタージュリスト */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	//TArray<UAnimMontage*> AttackFrontMontage;
	
	///** 軽い攻撃をくらうモンタージュ */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	//TObjectPtr<UAnimMontage> LightTakeHitMontage;
	//
	///** ノックダウンのモンタージュ */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	//TObjectPtr<UAnimMontage> KnockDownMontage;
	//
	///** ノックダウンのモンタージュ */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	//TObjectPtr<UAnimMontage> DownMontage;
	//
	///** ノックダウンのモンタージュ */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	//TObjectPtr<UAnimMontage> StandUpMontage;
	
	/** ヒット時のボイス */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* HitSound;
	
	/** 起き上がり後の硬直時間 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	float StiffeningTime = 0.7f;
	
	/** カウンター被弾時のモンタージュ */
	UPROPERTY(EditAnywhere, Category = "Combat|Counter")
	UAnimMontage* CounterHitFront;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Counter")
	UAnimMontage* CounterHitBack;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Counter")
	UAnimMontage* CounterHitLeft;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Counter")
	UAnimMontage* CounterHitRight;
	
	/** タイマーハンドル */
	FTimerHandle RecoveryTimerHandle;
	
	/** カウンターを受け付ける距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Counter")
	float CounterAcceptableDistance = 200.0f;
	
	/** 現在カウンター可能かどうかの内部フラグ */
	bool bIsCounterRange = false;
	
	// この敵の攻撃がカウンター可能かどうか(可能：true、不可：false)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Counter")
	bool bCanBeCountered = true;
	
	// 回避を受け付ける距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Counter")
	float DodgeAcceptableDistance = 200.0f;
	
	// 現在回避可能かどうかのフラグ
	bool bIsDodgeRange = false;
	
	// この敵の攻撃が回避可能かどうか(可能：true、不可：false)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Counter")
	bool bCanBeDodged = true;
	
	// 現在ジャスト回避可能かどうかのフラグ
	bool bIsJustDodgeRange = false;
	
	/** 回避窓のオープンを許可するかのフラグ */
	bool bCanOpenDodgeWindow = true;
	
	// コンボ中断時の猶予用タイマーハンドル
	FTimerHandle ComboInterruptionTimerHandle;
	
	// タイマーが切れたら呼び出す関数
	void OnComboInterruptionTimeout();
	
	/** 死亡時に関すること */
	// 横方向への吹き飛びの強さ
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Death", meta = (AllowPrivateAccess = true))
	float ImpulseStrength = 1500.0f;
	
	// 上方向への吹き飛びの強さ
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Death", meta = (AllowPrivateAccess = true))
	float UpwardForceMultiplier = 500.0f;
	
	// 死体が地面に残る時間
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Death")
	float DeathDestroyDelay = 3.0f;
	
	// 死体消去用のタイマーハンドル
	FTimerHandle DeathDestroyTimerHandle;
	
	// タイマーが切れた時に死体を消す関数
	void OnDeathDestroyTimeout();
	
	// 追跡時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	float MaxChaseDuration = 4.0f;
	
	// 追跡タイムアウト管理用
	FTimerHandle ChaseTimeoutTimerHandle;
	
	// タイムアウトした時に呼ばれる関数
	void OnChaseTimeout();
	
	// 攻撃権取得できる最大距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Distance")
	float MaxAttackTokenDistance = 1000.0f;
	
	// 攻撃権を取得できる最小距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Distance")
	float MinAttackTokenDistance = 300.0f;
	
	/** 敵AIの状態を遷移する */
   	//void UpdateAIState(EEnemyState NewState);
	
public: 
	/** 攻撃アクション実行 */
	UFUNCTION(BlueprintCallable, Category="Attack")
	virtual void ExecuteAttack();
	
	// === インターフェイス ===
	/** 物攻撃判定（トレース）を実行 */
	virtual bool DoAttackTrace_Implementation(FName DamageSourceBone) override;
	
	/** 他のアクターからダメージを受けた際の処理 */
	virtual void ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType) override;
	
	/** 他のアクターからカウンターダメージを受けた際の処理 */
	virtual void ApplyCounterDamage_Implementation(float Damage, AActor* DamageCauser, ERelativeDirection HitDirection) override;
	
	/** 死亡時の処理 */
	virtual void OnDeath() override;
	
	// 攻撃開始時にプレイヤーとの距離を計算する関数（BTT用）
	virtual float SelectAttackByDistance();
	
	// 被弾アニメーションのデータアセットへの参照
	UPROPERTY(EditAnywhere, Category = "Combat | Data")
	TObjectPtr<class UHitReactionDataAsset> HitReactionData;
	
	/** カウンター/回避を通知する関連 */	
	// カウンターを受け付ける距離かチェック
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckCounterDistance(AActor* PlayerActor);
	
	// プレイヤーへの通知とUI表示を一括で行う関数
	void NotifyCounterWindowState(bool bOpen);
	
	// カウンターの窓をスイッチする関数
	UFUNCTION(BlueprintCallable, Category="Combat")
	void SwitchCounterWindow(bool bSwitch);
	
	// 回避を受け付ける距離かチェック
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckDodgeDistance(AActor* PlayerActor);
	
	// プレイヤーへの通知とUI表示を一括で行う関数
	void NotifyDodgeWindowState(bool bOpen);
	
	// 回避の窓をスイッチする関数
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SwitchDodgeWindow(bool bSwitch);
	
	// プレイヤーへの通知
	void NotifyJustDodgeWindowState(bool bOpen);
	
	// ジャスト回避の窓をスイッチする
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SwitchJustDodgeWindow(bool bSwitch);
	
	// === GroundTakedown ===
	/** 現在のGroundTakedownのヒットインデックス */
	int32 CurrentGroundTakedownIndex = 0;

	/** GroundTakedown用のデータセットを取得して再生する関数 */
	//void PlayGroundTakedownHit(EHitReactionType HitReactionType);
	void PlayGroundTakedownHit(int32 Index);

	/** ヒットリアクションの状態をリセット（必要に応じて呼び出し） */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ResetHitSequence();
	
	// 攻撃時にプレイヤーを追いかけるタイマーを開始する関数
	void StartChaseTimer();
	void ClearChaseTimer();
	
	// プレイヤーが攻撃権を取得できる範囲にいるかチェックする関数
	UFUNCTION(BlueprintCallable, Category = "Combat|Distance")
	bool IsPlayerInAttackDistance() const;
	
	/** AN (ANS) から呼び出す関数 */
	// DownからRecoveryに状態を遷移する
   	UFUNCTION(BlueprintCallable, Category = "Combat")
   	void HandleDownEnd();
	
private:
	/** モンタージュ再生終了時のコールバック */
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	/** モンタージュ再生終了時のコールバック */
	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	
	/** 再生中のアニメがカウンターのものか判定するヘルパー関数 */
	bool IsCounterHitMontage(const UAnimMontage* Montage) const;
	
	/** 各リアクションタイプごとの再生ロジック */
	void PlayComboHit();
	void PlayNormalHit();	
	
	/** OnMontageEndedを分割 */
	virtual bool HandleAttackMontageEnd(UAnimMontage* Montage);
	bool HandleCounterMontageEnd(UAnimMontage* Montage);
	bool HandleStandUpMontageEnd(UAnimMontage* Montage);
	bool HandleTakedownMontageEnd(UAnimMontage* Montage);
	
	// シーケンス再生の状態を管理する内部カウンタ
	int32 CurrentNormalHitIndex = 0;
	
	// 最後に攻撃してきた相手を保持する変数
	UPROPERTY()
	TObjectPtr<AActor> LastDamageCauser = nullptr;
	
	// 現在受けているヒットリアクションを保持する関数
	EHitReactionType CurrentHitReactionType = EHitReactionType::Normal;
};