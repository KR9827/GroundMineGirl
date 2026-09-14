// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/Characters/BtlEnemyCharacter.h"

#include "Battle/Characters/EnemyAIController.h"
#include "Battle/Components/EnemyCombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ABtlEnemyCharacter::ABtlEnemyCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // スコア報酬の初期設定（デフォルト値: 雑魚キャラ用100pt）
    ScoreReward = 100;

    // Tickをoff
    PrimaryActorTick.bCanEverTick = false;
    // プレイヤーからの入力を受け付けない
    AutoReceiveInput = EAutoReceiveInput::Disabled;
    // AIが自動で乗り移る設定
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;    

    /** コンポーネントの生成 */
    // CombatComponentの生成
    CombatComponent = ObjectInitializer.CreateDefaultSubobject<UEnemyCombatComponent>(this, TEXT("EnemyCombatComponent"));
    // CounterWidgetの生成とアタッチ
    CounterWidget = ObjectInitializer.CreateDefaultSubobject<UWidgetComponent>(this, TEXT("CounterWidget"));
    CounterWidget->SetupAttachment(RootComponent);                            // ルートにアタッチ
    CounterWidget->SetWidgetSpace(EWidgetSpace::Screen);                           // 配置の設定（常にカメラを向く）
    CounterWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));         // 位置の設定（頭上）
    CounterWidget->SetVisibility(false);                                       // 初めは非表示
    // DodgeWidgetの生成とアタッチ
    DodgeWidget = ObjectInitializer.CreateDefaultSubobject<UWidgetComponent>(this, TEXT("DodgeWidget"));
    DodgeWidget->SetupAttachment(RootComponent);                                 // ルートにアタッチ
    DodgeWidget->SetWidgetSpace(EWidgetSpace::Screen);                            // 常にカメラを向く
    DodgeWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));          // 位置は頭上
    DodgeWidget->SetVisibility(false);                                        // 始めは非表示
    
    // カプセルサイズ
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // コントローラーの向きに体を強制同期させるか
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 移動方向に体を向けるか
    GetCharacterMovement()->bOrientRotationToMovement = true;
    // 回転速度
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    
    // 各種パラメータ 
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
    
    
}

void ABtlEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    CurrentHp = MaxHp;
}

bool ABtlEnemyCharacter::IsDown() const
{
    if (CurrentEnemyState == EEnemyState::Down) return true;
    
    return false;
}

bool ABtlEnemyCharacter::IsRecovery() const
{
    if (CurrentEnemyState == EEnemyState::Recovery) return true;
    
    return false;
}

bool ABtlEnemyCharacter::IsDeath() const
{
    if (CurrentEnemyState == EEnemyState::Death) return true;
    
    return false;
}

bool ABtlEnemyCharacter::IsDisabled() const
{
    return IsDown()/** || IsRecovery()*/ || IsDeath();
}

void ABtlEnemyCharacter::SetMovementParams(float NewSpeed, float NewDirection)
{
    AnimSpeed = NewSpeed;
    AnimMoveAngle = NewDirection;
}

void ABtlEnemyCharacter::Eliminate()
{
    // 死亡エフェクトなど
    
    // 消す
    Destroy();
}

void ABtlEnemyCharacter::SetEnemyState(EEnemyState State)
{
    if (CurrentEnemyState == State) return;
    
    // 敵の状態を更新
    CurrentEnemyState = State;
}

void ABtlEnemyCharacter::SetAttackOverlayEnabled(bool bEnable)
{   
    // キャラについてる全てのスケルタルメッシュコンポーネントを取得
    TArray<USkeletalMeshComponent*> SkelMeshes;
    GetComponents<USkeletalMeshComponent>(SkelMeshes);
    
    // もしなければエラー
    if (SkelMeshes.Num() == 0)
    {
       GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("エラー: メッシュが見つかりません！"));
       return;
    }
    
    // マテリアルnullチェック
    if (bEnable && !AttackOverlayMaterial)
    {
       GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("エラー: AttackOverlayMaterial が NULL です！"));
       return;
    }
    
    // 全てのスケルタルメッシュに対して Overlay Material を適用・解除
    UMaterialInterface* MaterialToApply = bEnable ? AttackOverlayMaterial : nullptr;
    
    for (UMeshComponent* SkelMesh : SkelMeshes)
    {
       if (SkelMesh)
       {
          SkelMesh->SetOverlayMaterial(MaterialToApply);
       }
    }
}