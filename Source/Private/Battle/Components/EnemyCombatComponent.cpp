#include "Battle/Components/EnemyCombatComponent.h"

#include <gsl/pointers>

#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Battle/Interfaces/DamageableInterface.h"
#include "AIController.h"
#include "Battle/Characters/BtlEnemyCharacter.h"
#include "Battle/Characters/EnemyAIController.h"
#include "Battle/DataAsset/HitReactionDataAsset.h"
#include "Battle/Enum/HitReactionType.h"
#include "Battle/Frameworks/BtlGameMode.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

class UMotionWarpingComponent;

UEnemyCombatComponent::UEnemyCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    
    OwnerChar = Cast<ACharacter>(GetOwner());
    if (OwnerChar)
    {
       // コントローラーとブラックボードをキャッシュ
       EAIC = Cast<AEnemyAIController>(OwnerChar->GetController());
       if (EAIC)
       {
          BB = EAIC->GetBlackboardComponent();
       }
       
       UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance();
       if (AnimInstance)
       {
          AnimInstance->OnMontageEnded.AddUniqueDynamic(this, &UEnemyCombatComponent::OnMontageEnded);
          AnimInstance->OnMontageBlendingOut.AddUniqueDynamic(this, &UEnemyCombatComponent::OnMontageBlendingOut);
       }
    }
    
    PlayerChar=Cast<ABtlPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(),0));
}

void UEnemyCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UEnemyCombatComponent::ExecuteAttack()
{
    
}

bool UEnemyCombatComponent::DoAttackTrace_Implementation(FName DamageSourceBone)
{
   
    return true;
}

float UEnemyCombatComponent::SelectAttackByDistance()
{
   
   return 200.0f;
}

void UEnemyCombatComponent::ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType)
{   
    // ガード
    if (!HitReactionData || !OwnerChar) return;
    
    bool bIsTakedown = (HitReactionType == EHitReactionType::GroundTakedown1 || 
                   HitReactionType == EHitReactionType::GroundTakedown2);
    ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(OwnerChar);
    
    if (bIsDead && !bIsTakedown) return;
    
    if (EnemyChar)
    {
       // 状態がDownもしくはDeathの場合攻撃を受け付けない
       EEnemyState CurrentState = EnemyChar->GetEnemyState();
       if ((CurrentState == EEnemyState::Down || CurrentState == EEnemyState::Death) && !bIsTakedown) return;
    }
       
    // 攻撃してきた相手をメンバ変数へ保存
    LastDamageCauser = DamageCauser;
    
    // 受け取ったヒットリアクションを記録
    CurrentHitReactionType = HitReactionType;
    
    Super::ApplyDamage_Implementation(Damage, DamageCauser, HitReactionType);
    
   if (HitSound && OwnerChar)
   {
      // 30%の確率で被弾ボイス再生
      if (FMath::RandRange(1, 100) <= 30)
      {
         UGameplayStatics::PlaySoundAtLocation(this, HitSound, OwnerChar->GetActorLocation());
      }
   }
   
    // カウンターの窓がオープンのとき強制的に閉じる
    if (bIsCounterRange)
    {
       SwitchCounterWindow(false);
    }
   
   // 回避の窓がオープンのとき強制的に閉じる
   if (bIsDodgeRange)
   {
      SwitchDodgeWindow(false);
   }
    
    // タイマーの初期化
    GetWorld()->GetTimerManager().ClearTimer(RecoveryTimerHandle);
    bIsAttacking = false;
    
    // 要求に応じた条件分岐
    switch (HitReactionType)
    {
    case EHitReactionType::Combo:
       PlayComboHit();
       break;
       
    case EHitReactionType::Normal:
       PlayNormalHit();
       break;
       
    case EHitReactionType::GroundTakedown1:
       PlayGroundTakedownHit(0);
       break;
       
    case EHitReactionType::GroundTakedown2:
       PlayGroundTakedownHit(1);
       break;
    }
    
    // ダメージUIを表示する
    if (EnemyChar)
    {
       EnemyChar->ShowDamagePopup(Damage);
    }
    
    // デバッグログ
    UKismetSystemLibrary::PrintString(
       this,
       FString::Printf(TEXT("Hit! Damage: %.1f -> HP: %.1f / %.1f"), Damage, CurrentHp, MaxHp),
       true, // 画面表示
       true, // ログ出力
       FLinearColor::Red,
       2.0f // 表示時間
    );
    UKismetSystemLibrary::PrintString(this, TEXT("STATE: HIT (4)"), true, true, FLinearColor::Green, 2.0f);
}

void UEnemyCombatComponent::OnDeath()
{
    Super::OnDeath();
    
    // 通常やられなどを完全に止める
    bIsAttacking = false;
    
    // 敵の状態をDeathにする
    EAIC->ChangeEnemyState(EEnemyState::Death);
    //UpdateAIState(EEnemyState::Death);
    
    // プレイヤーへインターフェース経由で死亡を通知
    if (PlayerChar)
    {
       IDamageableInterface::Execute_OnKilledEnemy(PlayerChar->GetCombatComponent());
    }
    
    // ゲームモード等へ敵が倒されたことを通知
    if (GetWorld())
    {
        if (AGameModeBase* CurrentGM = UGameplayStatics::GetGameMode(GetWorld()))
        {
           // あなたのゲームモードクラス（ABtlGameMode）にキャスト
           if (ABtlGameMode* BtlGM = Cast<ABtlGameMode>(CurrentGM))
           {
              // 所有者(ABtlEnemyCharacter)からScoreRewardを取得して渡す
              if (ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(OwnerChar))
              {
                 BtlGM->EnemyEliminated(EnemyChar->GetScoreReward());
              }
              else
              {
                 // 万が一キャストできない場合のフォールバック（デフォルト値100）
                 BtlGM->EnemyEliminated(100);
              }
           }
        }
    }
    
    // ------------------
    // テイクダウンの場合
    // ------------------
    bool bIsTakedown = (CurrentHitReactionType == EHitReactionType::GroundTakedown1 ||
                   CurrentHitReactionType == EHitReactionType::GroundTakedown2);
    
    if (bIsTakedown)
    {
       // 滑り防止（移動の慣性を消す）
       if (OwnerChar && OwnerChar->GetCharacterMovement())
       {
          OwnerChar->GetCharacterMovement()->StopMovementImmediately();
       }
       
       return;
    }
    
    // --------------
    // それ以外の場合
    // --------------  
    // アニメーションを止める    
    if (UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance())
    {
       AnimInstance->Montage_Stop(0.1f);
    }     
    
    // ラグドールとインパルスの処理
    USkeletalMeshComponent* Mesh = OwnerChar->GetMesh();
    UCapsuleComponent* Capsule = OwnerChar->FindComponentByClass<UCapsuleComponent>();
    if (Mesh && Capsule)
    {
       // カプセルの衝突判定を消す(床抜けや暴れ防止)
       Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
       
       // メッシュをラグドールに切り替える
       Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
       Mesh->SetSimulatePhysics(true);
       
       // インパルスの方向計算
       // デフォルトはキャラの真後ろ
       FVector ImpulseDir = -OwnerChar->GetActorForwardVector();
       
       // 攻撃者がいる場合は、攻撃者からキャラへのベクトルを計算
       if (LastDamageCauser)
       {
          ImpulseDir = OwnerChar->GetActorLocation() - LastDamageCauser->GetActorLocation();
          ImpulseDir.Z = 0.0f;      // 上下方向の傾きをリセットして純粋な横方向にする
          ImpulseDir.Normalize();       // 正規化する
       }
       
       // 横方向の力に、設定された強さをかける
       FVector FinalImpulse = ImpulseDir * ImpulseStrength;
       
       // 上方向への打ち上げ力を追加
       FinalImpulse.Z += UpwardForceMultiplier;
       
       // 力の適用（質量無視のとどめインパルス）
       Mesh->AddImpulse(FinalImpulse, NAME_None, true);
    }
    
    // AIコントローラーを引きはがして、思考を停止させる
    if (OwnerChar->GetController())
    {
        OwnerChar->GetController()->UnPossess();
    }
    
    // キャラクターの移動物理を完全に停止し、カプセルが下に落ちていくのを防ぐ
    if (OwnerChar->GetCharacterMovement())
    {
       OwnerChar->GetCharacterMovement()->DisableMovement();
    }
    
    // 指定秒のタイマーをかけ、その後に消滅する
    GetWorld()->GetTimerManager().SetTimer(
       DeathDestroyTimerHandle,
       this,
       &UEnemyCombatComponent::OnDeathDestroyTimeout,
       DeathDestroyDelay,
       false
       );
}

void UEnemyCombatComponent::OnDeathDestroyTimeout()
{
    if (ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(OwnerChar))
    {
       EnemyChar->Eliminate();
    }
}

void UEnemyCombatComponent::PlayComboHit()
{
    if (bIsDead) return;
    
    const TArray<UAnimMontage*>& RandomSet = HitReactionData->LightHitSettings.RandomMontages;
    if (RandomSet.Num() > 0)
    {
       int32 RandomIndex = FMath::RandRange(0, RandomSet.Num() - 1);
       UAnimMontage* SelectedMontage = RandomSet[RandomIndex];
       
       if (SelectedMontage)
       {
          // アニメーションが再生成功時のみ状態を変更する
          float Duration = OwnerChar->PlayAnimMontage(SelectedMontage);
          if (Duration > 0.0f)
          {
             EAIC->ChangeEnemyState(EEnemyState::TakeHit);
             //UpdateAIState(EEnemyState::TakeHit);
          }
       }
    }
}

void UEnemyCombatComponent::PlayNormalHit()
{
    if (bIsDead) return;
    
    const TArray<UAnimMontage*>& Sequence = HitReactionData->NormalHitSettings.MontageSequence;
    if (Sequence.Num() > 0)
    {
       CurrentNormalHitIndex = 0;
       
       UAnimMontage* FirstMontage = Sequence[CurrentNormalHitIndex];
       if (FirstMontage)
       {
          // アニメーションが再生成功時のみ状態を変更する
          float Duration = OwnerChar->PlayAnimMontage(FirstMontage);
          if (Duration > 0.0f)
          {
             EAIC->ChangeEnemyState(EEnemyState::Down);
          }
       }
    }
}

void UEnemyCombatComponent::ApplyCounterDamage_Implementation(float Damage, AActor* DamageCauser, ERelativeDirection HitDirection)
{
    // ガード
    if (BB)
    {
       // 状態がDownもしくはDeathの場合攻撃を受け付けない
       EEnemyState CurrentState = static_cast<EEnemyState>(BB->GetValueAsEnum(TEXT("CurrentState")));
       if ((CurrentState == EEnemyState::Down || CurrentState == EEnemyState::Death)) return;
    }
    if (bIsDead) return;
    if (!HitReactionData || !OwnerChar) return;
    
    // データアセットに対象の方向のモンタージュがあるかチェック
    if (!HitReactionData->CounterHitMap.Contains(HitDirection))
    {
       UE_LOG(LogTemp, Warning, TEXT("CounterHitMap does not contain direction: %d"), static_cast<int32>(HitDirection));
       return;
    }

    UAnimMontage* CounterMontage = HitReactionData->CounterHitMap[HitDirection];
    if (!CounterMontage) return;
    
    // 親クラスの基本処理を実行
    Super::ApplyCounterDamage_Implementation(Damage, DamageCauser, HitDirection);
    
    // カウンター/回避の窓とUIを閉じる(アニメーション停止前に呼んで、Tick等による二重判定を防ぐ)
    SwitchCounterWindow(false);
    SwitchDodgeWindow(false);
    
    // 攻撃状態を強制終了
    bIsAttacking = false;
    
    // 敵の現在のアニメーションを強制終了
    UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance();
    if (AnimInstance)
    {
       AnimInstance->Montage_Stop(0.05f);
    }
    
    // 方向に応じたカウンター被弾アニメーションを再生
    float Duration = OwnerChar->PlayAnimMontage(CounterMontage);
    if (Duration > 0.0f)
    {
       // 状態をCounteredに変える
       EAIC->ChangeEnemyState(EEnemyState::Countered);
       //UpdateAIState(EEnemyState::Countered);
    }
}

void UEnemyCombatComponent::HandleDownEnd()
{
    if (!OwnerChar) return;
    if (bIsDead) return;
    
    EAIC->ChangeEnemyState(EEnemyState::Recovery);
    //UpdateAIState(EEnemyState::Recovery);
    UKismetSystemLibrary::PrintString(this, TEXT("Down Ended via Notify -> RECOVERY"), true, true, FLinearColor::Green, 2.0f);
}

void UEnemyCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // ガード
    if (bIsDead) return;                        // 死んでたらアニメーション終了処理は飛ばす
    if (!OwnerChar || !HitReactionData) return;
    
    // カウンターやダメージで中断された場合はRecoveryに遷移せずに、アニメーションに処理を譲渡
    if (bInterrupted) return;
    
    // どれか処理が走ったらそこで終了
    if (HandleAttackMontageEnd(Montage))   return;
    if (HandleCounterMontageEnd(Montage))  return;
    if (HandleStandUpMontageEnd(Montage))  return;
    if (HandleTakedownMontageEnd(Montage)) return;
}

/* ----------------------
 * OnMontageEndedを小分け
 *-----------------------
 */
bool UEnemyCombatComponent::HandleAttackMontageEnd(UAnimMontage* Montage)
{
    
    return false;
}


bool UEnemyCombatComponent::HandleCounterMontageEnd(UAnimMontage* Montage)
{
    // ダウンポーズが終わった後の処理 
    //const TArray<UAnimMontage*>& NormalSequence = HitReactionData->NormalHitSettings.MontageSequence;
    //
    //if (NormalSequence.IsValidIndex(1) && Montage == NormalSequence[1])
    //{
    // // 起き上がりアセットが空の時の保険
    // UpdateAIState(EEnemyState::Recovery);
    // return true;
    //}    
    
    return false;
}

bool UEnemyCombatComponent::HandleStandUpMontageEnd(UAnimMontage* Montage)
{   
    // コンボ被弾のアニメーションが終わった時
    if (HitReactionData->LightHitSettings.RandomMontages.Contains(Montage))
    {
       // 古いタイマーが残ってたらクリアする
       GetWorld()->GetTimerManager().ClearTimer(ComboInterruptionTimerHandle);
       
       // 状態遷移する前に猶予タイマーを開始する
       float BreakWindow = 2.0f;           // 猶予：2秒
       GetWorld()->GetTimerManager().SetTimer(
          ComboInterruptionTimerHandle,
          this,
          &UEnemyCombatComponent::OnComboInterruptionTimeout,
          BreakWindow,
          false     // ループしない
       );
       
       UKismetSystemLibrary::PrintString(this, TEXT("Combo Stopped! Waiting for recovery timer..."), true, true, FLinearColor::Green, 2.0f);
       return true;
    }
    
    return false;
}

// タイマーが切れたら呼ばれる関数
void UEnemyCombatComponent::OnComboInterruptionTimeout()
{
    // 猶予時間内に次の攻撃を食らわなかったらRecoveryに遷移する
    if (BB)
    {
       EEnemyState CurrentState = static_cast<EEnemyState>(BB->GetValueAsEnum(TEXT("CurrentState")));
       if (CurrentState == EEnemyState::TakeHit)
       {
          EAIC->ChangeEnemyState(EEnemyState::Recovery);
          //UpdateAIState(EEnemyState::Recovery);
          UKismetSystemLibrary::PrintString(this, TEXT("Timer Timeout -> RECOVERY"), true, true, FLinearColor::Green, 2.0f);
       }
    }
}


bool UEnemyCombatComponent::HandleTakedownMontageEnd(UAnimMontage* Montage)
{
    const FHitSequenceSet& TakedownSettings = HitReactionData->GroundTakedownSettings;
    if (TakedownSettings.MontageSequence.IsValidIndex(1) && TakedownSettings.MontageSequence[1] == Montage)
    {
       UE_LOG(LogTemp, Warning, TEXT("GroundTakedown 2 finished. Eliminating enemy."));
       OnEliminated.Broadcast();
       //OwnerChar->Destroy();
       // キャラクターからキャラクターを消す処理を呼ぶ
       if (ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(OwnerChar))
       {
          EnemyChar->Eliminate();
       }     
       return true;
    }
    return false;
}

void UEnemyCombatComponent::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
    if (bIsDead) return;
    if (!OwnerChar || !HitReactionData) return;    
    
    if (bInterrupted) return;
    
    // 通常やられ   
    const TArray<UAnimMontage*>& NormalSequence = HitReactionData->NormalHitSettings.MontageSequence;
    // 現在ブレンドアウトしたモンタージュがシーケンス内の現在のインデックスと同じか
    if (NormalSequence.IsValidIndex(CurrentNormalHitIndex) && NormalSequence[CurrentNormalHitIndex] == Montage )
    {
        CurrentNormalHitIndex++;
        // 次があれば再生
        if (NormalSequence.IsValidIndex(CurrentNormalHitIndex) && NormalSequence[CurrentNormalHitIndex])
        {
           OwnerChar->PlayAnimMontage(NormalSequence[CurrentNormalHitIndex]);
           return;
        }
        // StandUpMontageを再生
        else if (HitReactionData->StandUpMontage)
        {
           OwnerChar->PlayAnimMontage(HitReactionData->StandUpMontage);
           return;
        }
    }
    
    // カウンターを食らったら倒れてるポーズへ移行
    if (IsCounterHitMontage(Montage))
    {     
       if (NormalSequence.IsValidIndex(1) && NormalSequence[1])
       {
          float Duration = OwnerChar->PlayAnimMontage(NormalSequence[1]);
          if (Duration > 0.0f)
          {
             // 状態もフライングで遷移する
             EAIC->ChangeEnemyState(EEnemyState::Down);
             //UpdateAIState(EEnemyState::HitStun);
          }
             
          UKismetSystemLibrary::PrintString(this, TEXT("Counter -> Down"), true, true, FLinearColor::Yellow, 2.0f);
       }     
    }
    
    // ゴロゴロから起き上がりアニメーションへ移行
    if (NormalSequence.IsValidIndex(1) && Montage == NormalSequence[1])
    {
       if (HitReactionData->StandUpMontage)
       {
          OwnerChar->PlayAnimMontage(HitReactionData->StandUpMontage);
          
          UKismetSystemLibrary::PrintString(this, TEXT("Down BlendingOut -> StandUp (Keep Down)"), true, true, FLinearColor::Yellow, 2.0f);
       }
    }
}

void UEnemyCombatComponent::CheckCounterDistance(AActor* PlayerActor)
{
    if (!PlayerActor || !OwnerChar) return;
    
    // 今の状態を取得
    ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(OwnerChar);
    // 状態がAttacking以外ならreturn
    if (!EnemyChar || EnemyChar->GetEnemyState() != EEnemyState::Attacking)
    {
       SwitchCounterWindow(false);
       return;
    }
       
    float Distance = FVector::Dist(GetOwner()->GetActorLocation(), PlayerActor->GetActorLocation());
    // 距離が設定値以下か判断
    bool bInDistance = (Distance <= CounterAcceptableDistance);
    
    // カウンター可能なとき
    if (bCanBeCountered)
    {
       if (bInDistance && !bIsCounterRange)
       {
          SwitchCounterWindow(true);
       }
       else if (!bInDistance && bIsCounterRange)
       {
          SwitchCounterWindow(false);
       }
    }
    // カウンター不可のとき
    else
    {
       SwitchCounterWindow(false);
    }
    
    UKismetSystemLibrary::PrintString(this,bIsCounterRange ? TEXT("true") : TEXT("false"), true, true, FLinearColor::Green, 2.0f);
}

void UEnemyCombatComponent::NotifyCounterWindowState(bool bOpen)
{
    // インターフェース経由で窓を開け閉めする
    if (PlayerChar)
    {
       if (bOpen) ITargetInterface::Execute_OpenCounterWindow(PlayerChar->GetCombatComponent(), GetOwner());
       else      ITargetInterface::Execute_CloseCounterWindow(PlayerChar->GetCombatComponent(), GetOwner());
    }
    
    // オーナーについてるタグ名を検索して表示を切り替える
    ABtlEnemyCharacter* Enemy = Cast<ABtlEnemyCharacter>(OwnerChar);
    if (Enemy && Enemy->GetCounterWidget())
    {
       Enemy->GetCounterWidget()->SetVisibility(bOpen);
    }
}

void UEnemyCombatComponent::SwitchCounterWindow(bool bSwitch)
{
    // すでにその状態なら何もしない（無駄な通知やウィジェット更新を防ぐ）
    if (bIsCounterRange == bSwitch) return;
    
    bIsCounterRange = bSwitch;
    NotifyCounterWindowState(bIsCounterRange);
}

void UEnemyCombatComponent::CheckDodgeDistance(AActor* PlayerActor)
{
    if (!PlayerActor || !OwnerChar) return;
    
    // 今の状態を取得
    ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(OwnerChar);
    // 状態がAttacking以外ならreturn
    if (!EnemyChar || EnemyChar->GetEnemyState() != EEnemyState::Attacking)
    {
       SwitchDodgeWindow(false);
       return;
    }
       
    float Distance = FVector::Dist(GetOwner()->GetActorLocation(), PlayerActor->GetActorLocation());
    // 距離が設定値以下か判断
    bool bInDistance = (Distance <= DodgeAcceptableDistance);
    
    // 回避可能な時
    if (bCanBeDodged)
    {
       if (bInDistance && !bIsDodgeRange)
       {
          SwitchDodgeWindow(true);
       }
       else if (!bInDistance && bIsDodgeRange)
       {
          SwitchDodgeWindow(false);
       }
    }
   // 回避不可の時
    else
    {
       SwitchDodgeWindow(false);
    }
    
    UKismetSystemLibrary::PrintString(this,bIsDodgeRange ? TEXT("true") : TEXT("false"), true, true, FLinearColor::Green, 2.0f);
}

void UEnemyCombatComponent::SwitchDodgeWindow(bool bSwitch)
{
	if (bSwitch)
	{
		// すでに一度閉じられている場合は、BTSからの再度表示を無視する
		if (!bCanOpenDodgeWindow) return;
	}
	else
	{
		// 窓を閉じる時は次の攻撃開始まで開かないようにする
		bCanOpenDodgeWindow = false;
	}
	
	if (bIsDodgeRange == bSwitch) return;
	
	bIsDodgeRange = bSwitch;
	NotifyDodgeWindowState(bIsDodgeRange);
}

void UEnemyCombatComponent::NotifyDodgeWindowState(bool bOpen)
{
    if (PlayerChar)
    {
       if (bOpen)  ITargetInterface::Execute_OpenDodgeWindow(PlayerChar->GetCombatComponent(), GetOwner());
       else      ITargetInterface::Execute_CloseDodgeWindow(PlayerChar->GetCombatComponent(), GetOwner());
    }
    
    // 回避用のUIをだす処理など
    ABtlEnemyCharacter* Enemy = Cast<ABtlEnemyCharacter>(OwnerChar);
    if (Enemy && Enemy->GetDodgeWidget())
    {
       Enemy->GetDodgeWidget()->SetVisibility(bOpen);
    }
}

void UEnemyCombatComponent::SwitchJustDodgeWindow(bool bSwitch)
{
    if (bIsJustDodgeRange == bSwitch) return;
    
    bIsJustDodgeRange = bSwitch;
    NotifyJustDodgeWindowState(bIsJustDodgeRange);
}

void UEnemyCombatComponent::NotifyJustDodgeWindowState(bool bOpen)
{
    if (PlayerChar)
    {
       if (bOpen) ITargetInterface::Execute_OpenJustDodgeWindow(PlayerChar->GetCombatComponent(), GetOwner());
       else      ITargetInterface::Execute_CloseJustDodgeWindow(PlayerChar->GetCombatComponent(), GetOwner());
    }
    
    // ジャスト回避用のUIがあれはここで出す処理を書く
}

void UEnemyCombatComponent::PlayGroundTakedownHit(int32 Index)
{
    if (!HitReactionData || !OwnerChar) return;
    
    // テイクダウンの演出が入る時は、通常の死亡タイマーをクリアする
    GetWorld()->GetTimerManager().ClearTimer(DeathDestroyTimerHandle);
    
    bIsAttacking = false;
    
    CurrentGroundTakedownIndex = Index;
    const FHitSequenceSet& TargetSequences = HitReactionData->GroundTakedownSettings;
    
    if (TargetSequences.MontageSequence.IsValidIndex(CurrentGroundTakedownIndex))
    {
       UAnimMontage* TakedownMontage = TargetSequences.MontageSequence[CurrentGroundTakedownIndex];
       if (TakedownMontage)
       {
          float Duration = OwnerChar->PlayAnimMontage(TakedownMontage);
          if (Duration > 0.0f)
          {
             EAIC->ChangeEnemyState(EEnemyState::Down);
             //UpdateAIState(EEnemyState::HitStun);
          }
       }
    }
}

void UEnemyCombatComponent::ResetHitSequence()
{
    CurrentGroundTakedownIndex = 0;
}

bool UEnemyCombatComponent::IsCounterHitMontage(const UAnimMontage* Montage) const
{
    if (!Montage || !HitReactionData) return false;

    // 起き上がりモーションは絶対にカウンター被弾ではないので、最初に除外
    if (Montage == HitReactionData->StandUpMontage) return false;
    
    if (HitReactionData && HitReactionData->CounterHitMap.Num() > 0)
    {
       TArray<UAnimMontage*> OutValues;
       HitReactionData->CounterHitMap.GenerateValueArray(OutValues);
       return OutValues.Contains(Montage);
    }
    
    return (
       Montage == CounterHitFront || 
       Montage == CounterHitBack  || 
       Montage == CounterHitLeft  || 
       Montage == CounterHitRight
       );
}

//void UEnemyCombatComponent::UpdateAIState(EEnemyState NewState)
//{
//  // ガード
//  if (bIsDead && NewState != EEnemyState::Death) return;
//  
//  if (BB)
//  {
//     // Enumを使用して状態を更新
//     BB->SetValueAsEnum(TEXT("CurrentState"), static_cast<uint8>(NewState));
//  }
//  
//  if (OwnerChar)
//  {
//     if (AEnemyAIController* EAIC = Cast<AEnemyAIController>(OwnerChar->GetController()))
//     {
//        EAIC->ChangeEnemyState(NewState);
//     }
//  }
//}

void UEnemyCombatComponent::StartChaseTimer()
{
    UKismetSystemLibrary::PrintString(this, TEXT("StartChaseTimer"), true, true, FColor::Blue, 2.0f);
    
    if (GetWorld()->GetTimerManager().IsTimerActive(ChaseTimeoutTimerHandle)) return;
    
    GetWorld()->GetTimerManager().ClearTimer(ChaseTimeoutTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
       ChaseTimeoutTimerHandle,
       this,
       &UEnemyCombatComponent::OnChaseTimeout,
       MaxChaseDuration,
       false
       );
}

void UEnemyCombatComponent::ClearChaseTimer()
{
    GetWorld()->GetTimerManager().ClearTimer(ChaseTimeoutTimerHandle);
}

void UEnemyCombatComponent::OnChaseTimeout()
{
    if (!OwnerChar) return;
    
    ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(OwnerChar);
    if (EnemyChar && EnemyChar->GetEnemyState() == EEnemyState::Attacking)
    {
       // デバログ
       UKismetSystemLibrary::PrintString(this, TEXT("Timeout! Attack!"), true, true, FColor::Blue, 2.0f);
          
       // 攻撃を実行
       ExecuteAttack();
    }
}

bool UEnemyCombatComponent::IsPlayerInAttackDistance() const
{
    if (!OwnerChar || !PlayerChar)
    {
       UE_LOG(LogTemp, Warning, TEXT("Nullptr"));
       return false;
    }
    
    // プレイヤーとの距離を計算
    float Distance  = FVector::Dist(OwnerChar->GetActorLocation(), PlayerChar->GetActorLocation());
    
    UE_LOG(
       LogTemp,
       Warning,
       TEXT("Distance=%f Min=%f Max=%f"),
       Distance,
       MinAttackTokenDistance,
       MaxAttackTokenDistance);
    
    // 範囲内ならtrueを返す
    return (Distance >= MinAttackTokenDistance) && (Distance <= MaxAttackTokenDistance);
}
