// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/Characters/EnemyAIController.h"
#include "Battle/Characters/BtlEnemyCharacter.h"
#include "Battle/Components/EnemyCombatComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "_Core/EnemyCombatManagerSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Navigation/CrowdFollowingComponent.h"

AEnemyAIController::AEnemyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	// 知覚用コンポーネントの作成
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerception");
	// 視覚の設定の作成
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("SightConfig");
	
	// 視界の設定
	//SightConfig->SightRadius = 1000.0f;							// 見える距離
	SightConfig->PeripheralVisionAngleDegrees = 180.0f;				// 視野角（左右180度）→ 全方位見える
	
	// 敵、味方、中立を検知対象にする
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	
	// コンポーネントに視覚設定を登録
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
	
	// 何かに気づいた時に呼ばれる関数を登録
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetDetected);
}

// プレイヤーを見つけた時に実行される
void AEnemyAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;
	if (!GetPawn() || !GetBlackboardComponent()) return;
	
	// 見つけた相手のタグがPlayerのとき追いかける
	if (Actor->ActorHasTag(FName("Player")))
	{
		if ( Stimulus.WasSuccessfullySensed())
		{
			GetBBChecked()->SetValueAsObject(TargetActorKeyName, Actor);
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("Player Locked!"));
			
			// プレイヤーを見つけたら追いかける
			ChangeEnemyState(EEnemyState::Chase);
			
			// 0.2秒毎に距離チェック
			if (!GetWorld()->GetTimerManager().IsTimerActive(DistanceCheckTimerHandle))
			{
			GetWorld()->GetTimerManager().SetTimer(
				DistanceCheckTimerHandle, 
				this, 
				&AEnemyAIController::UpdateCombatDistance, 
				0.2f, 
				true
				);
			}
		}
		else
		{
			// 敵同士でスルー
			UE_LOG(LogTemp, Log, TEXT("It's just a colleague. Ignore."));
		}
	}
}

void AEnemyAIController::UpdateCombatDistance()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	// BBから現在のターゲットを取得
	AActor* Target = Cast<AActor>(GetBBChecked()->GetValueAsObject(TargetActorKeyName));
	if (!Target) return;
		
	// 距離を計算
	float Distance = FVector::Dist(MyPawn->GetActorLocation(), Target->GetActorLocation());

	HandleCombatState(Distance);
}
	

void AEnemyAIController::ChangeEnemyState(EEnemyState NewState)
{
	// 状態を変更
	ABtlEnemyCharacter* Enemy = Cast<ABtlEnemyCharacter>(GetPawn());
	if (!Enemy || Enemy->GetEnemyState() == NewState) return;
	
	Enemy->SetEnemyState(NewState);
	
	// 状態がAttackingなら体を赤マテリアルで覆う
	Enemy->SetAttackOverlayEnabled(NewState == EEnemyState::Attacking);
	
	// BBの状態を変更
	GetBBChecked()->SetValueAsEnum(CurrentStateKeyName, static_cast<uint8>(NewState));
	
	// 移動速度の反映
	ApplyMovementByState(NewState);
	
	// 攻撃権を返す前に状態が遷移した時、攻撃権を強制的に返す
	if (bIsHoldingToken && NewState != EEnemyState::Attacking)
	{
		if (auto* Manager = GetWorld()->GetSubsystem<UEnemyCombatManagerSubsystem>())
		{
			Manager->ReleaseAttackToken(Enemy->GetEnemyType());
			bIsHoldingToken = false;
			// デバッグログ
			UKismetSystemLibrary::PrintString(this, TEXT("Token Released"), true, true, FLinearColor::Green, 2.0f);
		}
	}
	
	// Attackingになったら取得しているフラグを立てる
	if (NewState == EEnemyState::Attacking)
	{
		bIsHoldingToken = true;
	}
	
	// デバログ
	// Enumの文字列を取得
	FString StateStr = UEnum::GetValueAsString(NewState);
	// BBから実際に書き込まれた値を取得
	uint8 BBValue = GetBBChecked()->GetValueAsEnum(CurrentStateKeyName);
	
	// デバッグ用のテキストを作成
	FString DebugMsg = FString::Printf(TEXT("[%s] Change State -> NewState: %s | BB(%s): %d"), 
		*GetNameSafe(Enemy), *StateStr, *CurrentStateKeyName.ToString(), BBValue);
	
	// 画面上に表示
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, DebugMsg);
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_LOG(LogTemp, Log, TEXT("OnPossess!"));
	
	ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(GetPawn());
	if (!EnemyChar) return;
	
	// キャラにセットされているBehaviorTreeAssetを取得して実行する
	if (UBehaviorTree* BT = EnemyChar->GetBehaviorTreeAsset())
	{
		RunBehaviorTree(BT);
	}
	
	// ファイティングポーズをとる距離をランダムに決める
	CombatRange = FMath::FRandRange(EnemyChar->GetCombatRangeMin(), EnemyChar->GetCombatRangeMax());
	
	// キャラにセットされているBehaviorTreeAssetを取得して実行する
	//if (BehaviorTreeAssetMelee)
	//{
	//	RunBehaviorTree(BehaviorTreeAssetMelee);
	//}
	
	GetBBChecked()->SetValueAsFloat(AttackRangeKeyName, EnemyChar->GetAttackRange());
	
	// 確認用
	if (GetPathFollowingComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("PathFollowing: %s"),
			*GetPathFollowingComponent()->GetClass()->GetName());
	}
}

UBlackboardComponent* AEnemyAIController::GetBBChecked()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	check(BB);
	return BB;
}

void AEnemyAIController::HandleCombatState(float Distance)
{
	// CombatRange以内ならtrueを書き込む
	bool bInRange = (Distance <= CombatRange);
	GetBBChecked()->SetValueAsBool(InRangeKeyName, bInRange);
	
	ABtlEnemyCharacter* Enemy = Cast<ABtlEnemyCharacter>(GetPawn());
	if (!Enemy) return;
	
	EEnemyState CurrentState = Enemy->GetEnemyState();
	
	// ChaseかFightingの時
	if (CurrentState == EEnemyState::Idle || CurrentState == EEnemyState::Chase || CurrentState == EEnemyState::Fighting)
	{
		if (bInRange && CurrentState != EEnemyState::Fighting)
		{
			ChangeEnemyState(EEnemyState::Fighting);
		}
		else if (!bInRange && CurrentState != EEnemyState::Chase)
		{
			ChangeEnemyState(EEnemyState::Chase);
		}
	}		
	
	// デバッグ表示（エディタの画面左上に出る）
	FString Msg = bInRange ? TEXT("In Range: FIGHT!") : TEXT("Approaching...");
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, Msg);
}

void AEnemyAIController::ApplyMovementByState(EEnemyState State)
{
	ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(GetPawn());
    if (!EnemyChar) return;

    UCharacterMovementComponent* MoveComp = EnemyChar->GetCharacterMovement();
    if (!MoveComp) return;
	
	UEnemyCombatComponent* CombatComp = EnemyChar->GetCombatComponent();

	// ターゲットを取得する
	AActor* TargetActor = Cast<AActor>(GetBBChecked()->GetValueAsObject(TargetActorKeyName));
	
    switch (State)
    {
    case EEnemyState::Fighting:
        MoveComp->MaxWalkSpeed = EnemyChar->GetFightingMoveSpeed();
        MoveComp->bOrientRotationToMovement = false;		// 移動方向に体を向けない
    	MoveComp->bUseControllerDesiredRotation = true;		// AIの目線に合わせて体を滑らかに回す
    	MoveComp->RotationRate.Yaw = 360.0f;				// 回転速度
    	if (CombatComp) CombatComp->ClearChaseTimer();

    	// プレイヤーを注視する
    	if (TargetActor)
    	{
    		SetFocus(TargetActor);
    	}

        break;
    	
    case EEnemyState::Recovery:
    	MoveComp->MaxWalkSpeed = 0.0f;
    	MoveComp->bOrientRotationToMovement = false;
    	MoveComp->bUseControllerDesiredRotation = true;
    	MoveComp->RotationRate.Yaw = 360.0f;
    	if (CombatComp) CombatComp->ClearChaseTimer();
    	
    	// プレイヤーを見る
    	if (TargetActor)
    	{
    		SetFocus(TargetActor);
    	}
    	
    	break;
    	
    case EEnemyState::Stun:
    	MoveComp->MaxWalkSpeed = 0.0f;
    	ClearFocus(EAIFocusPriority::Gameplay);
    	if (CombatComp) CombatComp->ClearChaseTimer();
    	
    	break;

    case EEnemyState::Attacking:
        MoveComp->MaxWalkSpeed = EnemyChar->GetAttackingMoveSpeed();
        MoveComp->bOrientRotationToMovement = true;			// 移動方向に体を向ける
    	MoveComp->bUseControllerDesiredRotation = false;
    	if (CombatComp) CombatComp->StartChaseTimer();
    	
    	ClearFocus(EAIFocusPriority::Gameplay);					// Focusを解除
    	
        break;

    default:
        MoveComp->MaxWalkSpeed = EnemyChar->GetChaseMoveSpeed();
        MoveComp->bOrientRotationToMovement = true;			// 移動方向に体を向ける
    	MoveComp->bUseControllerDesiredRotation = false;
    	if (CombatComp) CombatComp->ClearChaseTimer();

		ClearFocus(EAIFocusPriority::Gameplay);					// Focusを解除

        break;
    }
}

// AIControllerがポーンのコントロールを解除した時に呼ばれる関数
void AEnemyAIController::OnUnPossess()
{
	// タイマーを止める
	GetWorld()->GetTimerManager().ClearTimer(DistanceCheckTimerHandle);
	
	// もし攻撃権をもってるなら、返却する
	if (bIsHoldingToken)
	{
		ABtlEnemyCharacter* Enemy = Cast<ABtlEnemyCharacter>(GetPawn());
		if (!Enemy) return;
		
		if (auto* Manager = GetWorld()->GetSubsystem<UEnemyCombatManagerSubsystem>())
		{
			Manager->ReleaseAttackToken(Enemy->GetEnemyType());
			bIsHoldingToken = false;
			// デバッグログ
			UKismetSystemLibrary::PrintString(this, TEXT("Token Released"), true, true, FLinearColor::Green, 2.0f);
		}
	}
	
	Super::OnUnPossess();
}
