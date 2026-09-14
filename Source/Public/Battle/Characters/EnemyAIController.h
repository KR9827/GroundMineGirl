// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Battle/Frameworks/EnemyState.h"
#include "EnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/**
 * 
 */
UCLASS()
class PROJECTJ_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AEnemyAIController(const FObjectInitializer& ObjectInitializer);
	
	// 状態を変更して、BBにも反映させる
	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void ChangeEnemyState(EEnemyState NewState);
	
	// getter
	FORCEINLINE FName GetAttackRangeKeyName() const { return AttackRangeKeyName; }
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	
	virtual void OnUnPossess() override;
	
	// プレイヤーを見つけたり見失ったりした時に呼ばれる通知用の関数
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);
	
	// タイマーから呼ばれる関数
	UFUNCTION()
	void UpdateCombatDistance();
	
	// =========　変数　=========
	// 距離チェックのタイマー用ハンドル
	FTimerHandle DistanceCheckTimerHandle;
	
	// キャラから受け取った数値からランダムな値を保持する変数（ファイティングポーズを取る距離）(BTTのMoveToより長い距離にしないといけない。)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float CombatRange = 300.0f;
	
	// BB上のキー名
	UPROPERTY(EditAnywhere, Category = "AI|Combat")
	FName InRangeKeyName = TEXT("bIsInRange");
	
	// プレイヤーを指すBB上のキー名
	UPROPERTY(EditAnywhere, Category = "AI|Combat")
	FName TargetActorKeyName = TEXT("TargetActor");
	
	// BB上のキー名
	UPROPERTY(EditAnywhere, Category = "AI|Combat")
	FName CurrentStateKeyName = TEXT("CurrentState");
	
	// BB上のキー名
	UPROPERTY(EditAnywhere, Category = "AI|Combat")
	FName AttackRangeKeyName = TEXT("AttackRange");
	
private:
	// 知覚を担うコンポーネント
	UPROPERTY(VisibleAnywhere, Category = "AI")
	UAIPerceptionComponent* AIPerception;
	
	// 視覚の設定（距離や角度）
	UPROPERTY(VisibleAnywhere, Category = "AI")
	UAISenseConfig_Sight* SightConfig;
	
	/** エディタ側でどのBehaviorTreeを使うか選べるようにする */
	// 近接攻撃のBT
	//UPROPERTY(EditAnywhere, Category = "AI|BehaviorTree")
	//UBehaviorTree* BehaviorTreeAssetMelee;
	
	// 遠距離攻撃のBT
	
	
	// 攻撃権を持っているかを記録する変数
	bool bIsHoldingToken = false;
	
	// BBを取得する
	UBlackboardComponent* GetBBChecked();
	
	// プレイヤーとの距離で状態を変化する
	void HandleCombatState(float Distance);
	
	// 状態による動きの変化	
	void ApplyMovementByState(EEnemyState State);
};
