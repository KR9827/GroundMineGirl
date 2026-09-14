// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Battle/Frameworks/EnemyState.h"
#include "_Core/EnemyCombatManagerSubsystem.h"
#include "BtlEnemyCharacter.generated.h"

// 前方宣言
class UEnemyCombatComponent;
class UWidgetComponent;
class AEnemyAIController;
class UBehaviorTree;

UCLASS()
class PROJECTJ_API ABtlEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    // コンストラクタ
    ABtlEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());   
    
    /** 状態フラグ */
    // 攻撃動作中かどうか
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="State")
    bool bIsAttacking = false;
    
    // ダウン状態かどうか
    UFUNCTION(BlueprintCallable, Category = "State")
    bool IsDown() const;

    // リカバリー状態かどうか
    UFUNCTION(BlueprintCallable, Category = "State")
    bool IsRecovery() const;
    
    // 死亡状態かどうか
    UFUNCTION(BlueprintCallable, Category = "State")
    bool IsDeath() const;

    // 行動不能かどうか
    UFUNCTION(BlueprintCallable, Category = "State")
    bool IsDisabled() const;
    
    // 値を書き込む関数
    UFUNCTION(BlueprintCallable, Category = "AI|Animation")
    void SetMovementParams(float NewSpeed, float NewDirection);
    
    // 敵（自分）をワールドから消滅させる関数
    void Eliminate();
    
    // UIを表示する関数
    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "UI")
    void ShowDamagePopup(float Damage);
    
    /** ゲッター関数 */
    // コンポーネント
    FORCEINLINE UEnemyCombatComponent* GetCombatComponent() const { return CombatComponent;}
    FORCEINLINE UWidgetComponent* GetCounterWidget() const { return CounterWidget;}
    FORCEINLINE UWidgetComponent* GetDodgeWidget() const { return DodgeWidget; }
    // 各距離
    FORCEINLINE float GetCombatRangeMin() const { return CombatRangeMin;}
    FORCEINLINE float GetCombatRangeMax() const { return CombatRangeMax;}
    FORCEINLINE float GetAttackRange() const { return AttackRange;}
    // 各移動速度
    FORCEINLINE float GetChaseMoveSpeed() const { return ChaseMoveSpeed;}
    FORCEINLINE float GetFightingMoveSpeed() const { return FightingMoveSpeed;}
    FORCEINLINE float GetAttackingMoveSpeed() const { return AttackingMoveSpeed;}
    // 状態を取得
    FORCEINLINE EEnemyState GetEnemyState() const { return CurrentEnemyState;}
    // キャラのBTを取得
    FORCEINLINE UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }
    // 敵の攻撃タイプを取得
    FORCEINLINE EEnemyType GetEnemyType() const {return EnemyType;}
    // 敵の攻撃権取得できる人数
    FORCEINLINE int32 GetMaxAttackers() const { return MaxAttackers; }
    // 撃破時に獲得できるポイントを取得
    FORCEINLINE int32 GetScoreReward() const { return ScoreReward; }
    
    /** セッター関数 */
    // 状態を設定
    void SetEnemyState(EEnemyState State);
    
    // 攻撃警告用のオーバーレイマテリアルを設定
    UFUNCTION(BlueprintCallable, Category = "Effects")
    void SetAttackOverlayEnabled(bool bEnable);
    
protected:
    virtual void BeginPlay() override;
    
    /** コンポーネント */
    // 敵の敵の戦闘ロジックを管理するコンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    UEnemyCombatComponent* CombatComponent;
    
    // カウンターUIを表示するためのコンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* CounterWidget;
    
    // 回避UIを表示するためのコンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* DodgeWidget;
    
    /** BT */
    // この敵キャラが使うBT
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    UBehaviorTree* BehaviorTreeAsset;
    
    /** ステータス関連 */
    // 最大体力
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status")
    float MaxHp = 100.0f;
    
    // 現在の体力
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status")
    float CurrentHp;

    // 撃破時にプレイヤーが得るスコア（デフォルト: 100pt）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status")
    int32 ScoreReward = 100;
    
    // Fighting時の移動する速度
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float AnimSpeed = 0.0f;

    // Fighting時の移動する角度
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float AnimMoveAngle = 0.0f;
    
    // Chase状態時の移動速度
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement")
    float ChaseMoveSpeed = 500.0f;
        
    // Fighting状態時の移動速度
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement")
    float FightingMoveSpeed = 150.0f;  
    
    // Attacking状態時の移動速度
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement")
    float AttackingMoveSpeed = 800.0f;
    
    /** 状況によるプレイヤーとの距離 */  
    // Fighting時のプレイヤーとの距離の最小値
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Distance")
    float CombatRangeMin = 1000.0f;
    
    // Fighting時のプレイヤーとの距離の最大値
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Distance")
    float CombatRangeMax = 1500.0f;
    
    // 攻撃をする距離
    UPROPERTY(EditAnywhere, Category = "AI|Distance")
    float AttackRange = 200.0f;
    
    // 現在の状態
    UPROPERTY(EditAnywhere, Category = "AI|State")
    EEnemyState CurrentEnemyState = EEnemyState::Idle;
    
    /** キャラの攻撃権 */
    // キャラの攻撃タイプを選択
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    EEnemyType EnemyType = EEnemyType::None;
    
    // キャラの攻撃権取得できる人数
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    int32 MaxAttackers = 1;
    
    // オーバーレイマテリアルの参照
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
    TObjectPtr<UMaterialInterface> AttackOverlayMaterial;
};