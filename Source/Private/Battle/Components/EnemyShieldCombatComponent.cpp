// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/Components/EnemyShieldCombatComponent.h"

#include "Battle/Characters/BtlEnemyCharacter.h"
#include "Battle/Characters/EnemyAIController.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Battle/Interfaces/DamageableInterface.h"
#include "Battle/Interfaces/TargetInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "Kismet/GameplayStatics.h"

UEnemyShieldCombatComponent::UEnemyShieldCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyShieldCombatComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// 攻撃中かつプレイヤーが存在する場合のみ表示
	if ((bIsForcedAttacking || bIsAttacking) && OwnerChar && PlayerChar)
	{
		UpdateWarpTarget();
	}
}

void UEnemyShieldCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentShieldHp = MaxShieldHp;
	bIsShieldBroken = false;
	
	// EAICの初期化処理
	if (AActor* Owner = GetOwner())
	{
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			EAIC = Cast<AEnemyAIController>(Pawn->GetController());
		}
	}
	
	// 攻撃をカウンター不可にする
	bCanBeCountered = false;
}

void UEnemyShieldCombatComponent::ExecuteAttack()
{
	// 配列が空、または攻撃中の場合は何もしない
	if (AttackFrontMontage.Num() == 0 || bIsAttacking) return;
	// ガード
	if (!OwnerChar || !PlayerChar) return;
	
	// 攻撃前にプレイヤーの方を向かせる
	FVector LookDir = PlayerChar->GetActorLocation() - OwnerChar->GetActorLocation();
	LookDir.Z = 0.0f;
	if (!LookDir.IsNearlyZero())
	{
		OwnerChar->SetActorRotation(LookDir.Rotation());
		
	}
	
	UAnimMontage* SelectedMontage = nullptr;
	
	// 攻撃AMを再生
	// 強制攻撃のとき
	if (bIsForcedAttacking)
	{
		// 強制攻撃は3番目の攻撃を再生する
		SelectedMontage = AttackFrontMontage[2];
	}
	// 通常攻撃（距離に応じて再生するAMを選ぶ）
	else
	{
		// 配列の要素範囲内かチェック
		if (AttackFrontMontage.IsValidIndex(SelectAttackIndex))
		{
			SelectedMontage = AttackFrontMontage[SelectAttackIndex];
		}
	}
 
	if (SelectedMontage)
	{
		// 中を空にする
		AlreadyHitActors.Empty();
		
		bIsAttacking = true;			// 攻撃中フラグセット	
		
		bCanOpenDodgeWindow = true;		// 攻撃が新しく始まったから許可する
		
		// 強制攻撃の場合は、ターゲット位置を設定
		UpdateWarpTarget();
		
		// モンタージュ再生のみ。
		OwnerChar->PlayAnimMontage(SelectedMontage);
	}
}

void UEnemyShieldCombatComponent::ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType)
{
	// ガード
	if (!HitReactionData || !OwnerChar) return;
	
	bool bIsTakedown = (HitReactionType == EHitReactionType::GroundTakedown1 || 
						HitReactionType == EHitReactionType::GroundTakedown2);
	if (bIsDead && !bIsTakedown) return;
	
	ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(OwnerChar);
	if (!EnemyChar) return;

	// 状態がDownもしくはDeathの場合攻撃を受け付けない
	EEnemyState CurrentState = EnemyChar->GetEnemyState();
	if ((CurrentState == EEnemyState::Down || CurrentState == EEnemyState::Death) && !bIsTakedown) return;
	
	// 強制攻撃中は盾ダメージだけ計算する
	if (bIsForcedAttacking)
	{		
		// 盾の耐久度を減らす
		CurrentShieldHp = FMath::Clamp(CurrentShieldHp - Damage, 0.0f, MaxShieldHp);
		
		// 耐久度が0になったとき
		if (CurrentShieldHp <= 0.0f)
		{
			BreakShield();
			CurrentGuardHitCount = 0;		// 盾破壊時にカウントをリセット
		
			// 状態がスタンになる
			ApplyStun();
		}
		
		// ガードSEを再生
		if (GuardSound && OwnerChar)
		{
			UGameplayStatics::PlaySoundAtLocation(this, GuardSound, OwnerChar->GetActorLocation());
		}
		
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("強制攻撃中！ ひるみません！"));
		return;
	}
	
	// スタンのタイマーが動いてるか
	bool bIsStunTimerActive = GetWorld()->GetTimerManager().IsTimerActive(StunTimerHandle);
	
	// -------------
	// 状態がStunの時
	// -------------
	if (CurrentState == EEnemyState::Stun || bIsStunTimerActive)
	{
		Super::ApplyDamage_Implementation(Damage, DamageCauser, HitReactionType);
		
		// スタンのタイマー期間中は状態をStunに戻す
		if (EAIC)
		{
			EAIC->ChangeEnemyState(EEnemyState::Stun);
		}
		
		return;
	}	
	
	// --------------------
	// 防御するとき
	// --------------------	
	// 盾が破壊されていたら攻撃を受ける
	if (bIsShieldBroken)
	{
		Super::ApplyDamage_Implementation(Damage, DamageCauser, HitReactionType);
		return;
	}
	
	// 盾の耐久度を減らす
	CurrentShieldHp = FMath::Clamp(CurrentShieldHp - Damage, 0.0f, MaxShieldHp);
	
	// デバ表示
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(TEXT("盾耐久度: %.1f / %.1f"), CurrentShieldHp, MaxShieldHp));
	
	// 耐久度が0になったとき
	if (CurrentShieldHp <= 0.0f)
	{
		BreakShield();
		CurrentGuardHitCount = 0;		// 盾破壊時にカウントをリセット
		
		// 状態がスタンになる
		ApplyStun();
		
		return;
	}
	
	// 耐久度が残っている場合
	if (GuardMontage && OwnerChar)
	{		
		// 攻撃フラグと回避窓をリセット
		bIsAttacking = false;
		if (bIsDodgeRange) SwitchDodgeWindow(false);
		
		// 状態をFightingに遷移
		if (EAIC)
		{
			EAIC->ChangeEnemyState(EEnemyState::Fighting);
		}
		
		// キャラの物理的な移動や慣性を止める
		if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}
		
		// AIControllerが出してるMoveToの命令をクリアする
		if (AAIController* AICon = Cast<AAIController>(OwnerChar->GetController()))
		{
			AICon->StopMovement();
		}
		
		// 攻撃してきたプレイヤーの方を向く
		AActor* TargetActor = DamageCauser ? DamageCauser : PlayerChar;
		if (TargetActor && OwnerChar)
		{
			FVector Direction = TargetActor->GetActorLocation() - OwnerChar->GetActorLocation();
			Direction.Z = 0.0f;		// 高低差による上下の傾きを無しにする
			
			if (!Direction.IsNearlyZero())
			{
				OwnerChar->SetActorRotation(Direction.Rotation());
			}
		}
		
		// ガード回数を加算
        CurrentGuardHitCount++;
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, FString::Printf(TEXT("ガード回数: %d / %d"), CurrentGuardHitCount, MaxGuardHitCount));
        
        // 規定回数に達したら強制攻撃へ移行
        if (CurrentGuardHitCount >= MaxGuardHitCount)
        {
        	CurrentGuardHitCount = 0;		// カウントリセット
        	bIsForcedAttacking = true;		// 無敵フラグON
        	
        	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("【3回ガード】強制反撃発動！！"));
        	
        	// backstep開始
        	StartForcedAttackSequence();
        	return;					// ガードアニメーションへいかずに終了
        }
		
		// 防御アニメーションを再生して、終わったら呼び出す関数を登録
		if (UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance())
		{
			float Duration = AnimInstance->Montage_Play(GuardMontage);		// モンタージュの再生
			// 再生に成功したとき
			if (Duration > 0.0f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UEnemyShieldCombatComponent::OnGuardMontageEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, GuardMontage);
			}
		}
		
		// ガードSEを再生
		if (GuardSound && OwnerChar)
		{
			UGameplayStatics::PlaySoundAtLocation(this, GuardSound, OwnerChar->GetActorLocation());
		}
	}
	
	// デバ表示
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("ガード！！"));
}

bool UEnemyShieldCombatComponent::DoAttackTrace_Implementation(FName DamageSourceBone)
{
	if (!OwnerChar) return false;
	
	USceneComponent* TraceComponent = nullptr;
	
	// キャラにアタッチされてる全てのStaticMeshComponentを取得
	TArray<UStaticMeshComponent*> StaticMeshes;
	OwnerChar->GetComponents<UStaticMeshComponent>(StaticMeshes);
	
	// 指定のソケット名があるメッシュを探す
	for (UStaticMeshComponent* Mesh : StaticMeshes)
	{
		if (Mesh && Mesh->DoesSocketExist(DamageSourceBone))
		{
			TraceComponent = Mesh;
			break;
		}
	}
	
	// 見つからなければ失敗
	if (!TraceComponent) return false;
    
	// 今のフレームのソケット開始地点を取得
    FVector CurrentSocketLocation = TraceComponent->GetSocketLocation(DamageSourceBone);
	
	// トレースの開始地点と終了地点
	FVector StartPos;
	FVector EndPos = CurrentSocketLocation;
	
	// 最初のフレームは動かさない
	if (bIsFirstTraceFrame)
	{
		StartPos = CurrentSocketLocation;
		bIsFirstTraceFrame = false;
	}
	else
	{
		StartPos = PrevSocketLocation;
	}
	
	// 次のフレームのために今の位置を保存
	PrevSocketLocation = CurrentSocketLocation;
    
    // オブジェクトタイプの配列
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	// トレース対象にPawnとWorldDynamicを登録
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));
    
    // 自身をトレース対象から除外
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(OwnerChar);
	
	// 球体トレースによる範囲判定の実行
    TArray<FHitResult> OutHits;
    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
       GetWorld(),
       StartPos,
       EndPos,
       MeleeTraceRadius,
       ObjectTypes,
       false,
       ActorsToIgnore,
       EDrawDebugTrace::None,	// デバッグ用で表示する
       OutHits,
       true,
       FLinearColor::Blue,
       FLinearColor::Yellow,
       2.0f
    );
	
	// カウンターの窓を閉じる
	if (PlayerChar && PlayerChar->GetCombatComponent())
	{
		ITargetInterface::Execute_CloseCounterWindow(PlayerChar->GetCombatComponent(),GetOwner());
		ITargetInterface::Execute_CloseDodgeWindow(PlayerChar->GetCombatComponent(),GetOwner());
		ITargetInterface::Execute_CloseJustDodgeWindow(PlayerChar->GetCombatComponent(),GetOwner());
	}
	SwitchCounterWindow(false);
	SwitchDodgeWindow(false);
	SwitchJustDodgeWindow(false);
	
	// ダメージ処理
	if (bHit)
	{
		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
        
			// 有効なアクターかつ、このループ内ですでにダメージを与えていないかチェック
			if (!HitActor || AlreadyHitActors.Contains(HitActor)) continue;
			
			// プレイヤー以外にはダメージを与えない
			if (HitActor != PlayerChar) continue;

			// インターフェース経由でのダメージ適用 
			UActorComponent* Comp = HitActor->FindComponentByInterface(UDamageableInterface::StaticClass());

			if (IDamageableInterface* Damageable = Cast<IDamageableInterface>(Comp))
			{
				// インターフェース経由でダメージ処理を実行
				IDamageableInterface::Execute_ApplyDamage(
					Comp,
					MeleeDamage,
					GetOwner(),
					EHitReactionType::Normal // 仮
				);

				// ヒット済みリストに追加
				AlreadyHitActors.Add(HitActor);
			}
		}
    }
	return true;
}

bool UEnemyShieldCombatComponent::HandleAttackMontageEnd(UAnimMontage* Montage)
{
	if (AttackFrontMontage.Contains(Montage))
	{
		bIsAttacking = false;
		
		// 攻撃が終わったら強制攻撃フラグを解除
		bIsForcedAttacking = false;
		
		// 攻撃終わったら強制的にカウンター窓を閉じる
		if (bIsCounterRange)	SwitchCounterWindow(false);
		if (bIsDodgeRange)		SwitchDodgeWindow(false);
		if (bIsJustDodgeRange)	SwitchJustDodgeWindow(false);
		
		EAIC->ChangeEnemyState(EEnemyState::Recovery);
		return true;
	}
	return false;
}

void UEnemyShieldCombatComponent::OnJustDodgeSuccess_Implementation(AActor* Defender)
{
	Super::OnJustDodgeSuccess_Implementation(Defender);
	
	// ジャスト回避されたらスタン状態へ
	if (bIsJustDodgeRange)
	{
		ApplyStun();
	}
}

void UEnemyShieldCombatComponent::ApplyStun()
{
	if (bIsDead || !OwnerChar) return;
	
	// 念のため
	bIsAttacking = false;
	bIsForcedAttacking = false;
	if (bIsDodgeRange)		SwitchDodgeWindow(false);
	if (bIsJustDodgeRange)	SwitchJustDodgeWindow(false);
	
	// 状態をStunにする
	if (EAIC)
	{
		EAIC->ChangeEnemyState(EEnemyState::Stun);
	}
	
	// キャラの物理的な移動や慣性を止める
	if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}
		
	// AIControllerが出してるMoveToの命令をクリアする
	if (AAIController* AICon = Cast<AAIController>(OwnerChar->GetController()))
	{
		AICon->StopMovement();
	}
	
	// スタン用AMを再生
	if (StunMontage)
	{
		OwnerChar->PlayAnimMontage(StunMontage);
	}
	
	// 指定した時間を過ぎたらStunを解除
	GetWorld()->GetTimerManager().SetTimer(
		StunTimerHandle,
		this,
		&UEnemyShieldCombatComponent::ClearStun,
		StunDuration,
		false
		);
	
	// デバ表示
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("スタン状態！！"));
}

void UEnemyShieldCombatComponent::ClearStun()
{
	// タイマーを解除する
	GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);
	
	if (bIsDead) return;
	
	// スタン用モンタージュを停止
	if (StunMontage && OwnerChar)
	{
		OwnerChar->StopAnimMontage(StunMontage);
	}
	
	// 状態をRecoveryへ遷移
	if (EAIC)
	{
		EAIC->ChangeEnemyState(EEnemyState::Recovery);
	}
	
	// デバ表示
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("スタン解除！！　Recoveryへ"));
}

void UEnemyShieldCombatComponent::ResetTraceLocation()
{
	bIsFirstTraceFrame = true;
	AlreadyHitActors.Empty();
}

void UEnemyShieldCombatComponent::OnGuardMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// ガード
	if (bIsDead || !OwnerChar) return;
	
	// アニメーションが中断された場合は状態は変更されない
	if (bInterrupted) return;
	
	// 防御AMが終わったら状態をFightingに戻す
	if (EAIC)
	{
		EAIC->ChangeEnemyState(EEnemyState::Fighting);
	}
}

void UEnemyShieldCombatComponent::BreakShield()
{
	bIsShieldBroken = true;
	
	// 盾を破壊されたSEを再生
	if (ShieldBreakSound && OwnerChar)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShieldBreakSound, OwnerChar->GetActorLocation());
	}
	
	// デバ表示
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("盾破壊！！　ガード不可状態になりました！"));
	
	// BP側に通知
	if (OnShieldBroken.IsBound())
	{
		OnShieldBroken.Broadcast();
	}
}

void UEnemyShieldCombatComponent::StartForcedAttackSequence()
{
	if (!OwnerChar) return;
	
	// バックステップ前にプレイヤーの方を向かせる
	if (PlayerChar)
	{
		FVector LookDir = PlayerChar->GetActorLocation() - OwnerChar->GetActorLocation();
		LookDir.Z = 0.0f;
		if (!LookDir.IsNearlyZero())
		{
			OwnerChar->SetActorRotation(LookDir.Rotation());
		}
	}
	
	// バックステップ用AMが設定されてたら再生
	if (BackstepMontage)
	{
		if (UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance())
		{
			float Duration = AnimInstance->Montage_Play(BackstepMontage);
			if (Duration > 0.0f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UEnemyShieldCombatComponent::OnBackstepEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, BackstepMontage);
				return;
			}
		}
	}
	
	// もしバックステップが設定されてない/再生失敗した場合、そのまま攻撃に移る
	ExecuteAttack();
}

void UEnemyShieldCombatComponent::OnBackstepEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted)
	{
		bIsForcedAttacking = false;		// 強制攻撃状態を解除
		return;
	}
	
	ExecuteAttack();
}

void UEnemyShieldCombatComponent::UpdateWarpTarget()
{
	if (!OwnerChar || !PlayerChar) return;
	
	if (UMotionWarpingComponent* MotionWarpComp = OwnerChar->FindComponentByClass<UMotionWarpingComponent>())
	{
		FVector EnemyLoc = OwnerChar->GetActorLocation();
		FVector PlayerLoc = PlayerChar->GetActorLocation();
		
		// 敵からプレイヤーへの方向を計算
		FVector DirToPlayer = (PlayerLoc - EnemyLoc).GetSafeNormal2D();
		if (DirToPlayer.IsNearlyZero()) return;
		
		// 毎回追従する位置を更新する
		FMotionWarpingTarget Target;									// 構造体の設定
		Target.Name = FName("AttackTarget");
		Target.Location = PlayerLoc - (DirToPlayer * StopOffset);		// 位置の計算
		Target.Rotation = DirToPlayer.Rotation();						// 向きの計算
		Target.bFollowComponent = false;								// コンポーネントの追従をオフにする
			
		// 作った構造体を渡す
		MotionWarpComp->AddOrUpdateWarpTarget(Target);
	}
}

float UEnemyShieldCombatComponent::SelectAttackByDistance()
{
	if (!OwnerChar || !PlayerChar) return CloseAttackRange;
	
	float Distance = FVector::Dist2D(OwnerChar->GetActorLocation(), PlayerChar->GetActorLocation());
	SelectAttackIndex = (Distance <= CloseRangeDistance) ? 0 : (Distance <= MediumRangeDistance) ? 1 : 2;
	
	// デバ表示
   	FString RangeStr = (SelectAttackIndex == 0) ? TEXT("近距離") : (SelectAttackIndex == 1) ? TEXT("中距離") : TEXT("遠距離");
   	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("プレイヤー距離: %.1f -> [%s] 攻撃を再生"), Distance, *RangeStr));
	
	// 選ばれたIndex毎に間合いを返す
	if (SelectAttackIndex == 1) return MediumAttackRange;
	if (SelectAttackIndex == 2) return RangedAttackRange;
	return CloseAttackRange;	
}
