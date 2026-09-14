// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/Components/EnemyChargeCombatComponent.h"
#include "Battle/Characters/EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UEnemyChargeCombatComponent::UEnemyChargeCombatComponent()
{
	
}

void UEnemyChargeCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// EAICの初期化処理
	if (AActor* Owner = GetOwner())
	{
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			EAIC = Cast<AEnemyAIController>(Pawn->GetController());
		}
	}
}

void UEnemyChargeCombatComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	
}

void UEnemyChargeCombatComponent::ExecuteAttack()
{
	// 攻撃中は何もしない
	if (bIsAttacking) return;	
	// ガード
	if (!OwnerChar || !PlayerChar) return;
	
	bIsAttacking = true;
	
	// 攻撃前にプレイヤーの方を向かせる
	FVector LookDir = PlayerChar->GetActorLocation() - OwnerChar->GetActorLocation();
	LookDir.Z = 0.0f;
	if (!LookDir.IsNearlyZero())
	{
		OwnerChar->SetActorRotation(LookDir.Rotation());
	}
	
	//UAnimMontage* SelectMontage = nullptr;
	
	if (bIsChargeAttack)
	{
		ChargeAttack();
		bCanOpenDodgeWindow = true;
	}
	else
	{
		CloseAttack();
	}	
}

bool UEnemyChargeCombatComponent::DoAttackTrace_Implementation(FName DamageSourceBone)
{
	if (!OwnerChar) return false;

    USkeletalMeshComponent* Mesh = OwnerChar->FindComponentByClass<USkeletalMeshComponent>();
    if (!Mesh) return false;
    
	// トレースの開始地点
    FVector StartPos = Mesh->GetSocketLocation(DamageSourceBone);
    
	// 前方ベクトルに基づいた終了地点の計算
    FVector ForwardVector = OwnerChar->GetActorForwardVector();
    FVector EndPos = StartPos + (ForwardVector * MeleeTraceDistance); 
    
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
       EDrawDebugTrace::None,
       OutHits,
       true,
       FLinearColor::Blue,
       FLinearColor::Yellow,
       2.0f
    );

	if (PlayerChar && PlayerChar->GetCombatComponent())
	{
		Execute_CloseCounterWindow(PlayerChar->GetCombatComponent(),GetOwner());
	}
	
	NotifyCounterWindowState(false);
	NotifyDodgeWindowState(false);
	
	if (bHit)
	{
		// 同一の攻撃で同じアクターに複数回ヒットしないための重複チェック用リスト
		TArray<AActor*> AlreadyHitActors;

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

void UEnemyChargeCombatComponent::ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType)
{
	Super::ApplyDamage_Implementation(Damage, DamageCauser, HitReactionType);
}

bool UEnemyChargeCombatComponent::HandleAttackMontageEnd(UAnimMontage* Montage)
{
	if (Montage == CloseAttackMontage || Montage == ChargeAttackMontage)
	{
		bIsAttacking = false;
		
		// 攻撃終わったら強制的にカウンター窓を閉じる
		if (bIsCounterRange)	SwitchCounterWindow(false);
		if (bIsDodgeRange)		SwitchDodgeWindow(false);
		if (bIsJustDodgeRange)	SwitchJustDodgeWindow(false);
		
		if (EAIC)
		{
			EAIC->ChangeEnemyState(EEnemyState::Recovery);
		}
		return true;
	}
	
	return false;
}

void UEnemyChargeCombatComponent::CloseAttack()
{
	if (!OwnerChar) return;
	
	if (CloseAttackMontage)
	{
		OwnerChar->PlayAnimMontage(CloseAttackMontage);
	}
}

void UEnemyChargeCombatComponent::ChargeAttack()
{	
	if (!OwnerChar) return;
	
	// 予備動作用AMが設定されてたら再生
	if (PreparatoryActionMontage)
	{		
		if (UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance())
		{
			float Duration = AnimInstance->Montage_Play(PreparatoryActionMontage);
			if (Duration > 0.0f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UEnemyChargeCombatComponent::StartCharge);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, PreparatoryActionMontage);
				return;
			}
		}
	}
	
	// もし予備動作が無ければ、突進開始
	StartCharge(nullptr, false);
}

void UEnemyChargeCombatComponent::StartCharge(UAnimMontage* Montage, bool bInterrupted)
{
	// 中断された場合は突進しない
	if (bInterrupted)
	{
		bIsAttacking = false;
		
		// 回避窓を閉じる
		if (bIsDodgeRange) SwitchDodgeWindow(false);
		
		return;
	}
	
	if (!OwnerChar) return;
	
	// 突進時の速度を変更
	if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = ChargeMoveSpeed;
	}
	
	// 突進開始時に追従するプレイヤーの位置等を登録
	WarpTarget();
	
	if (ChargeMontage)
	{
		OwnerChar->PlayAnimMontage(ChargeMontage);
	}
}

void UEnemyChargeCombatComponent::UpdateChargingPlayerToDistance(float DeltaTime)
{
	// ガード
	if (!OwnerChar || !PlayerChar) return;
	
	CurrentChargeTime += DeltaTime;
	
	// 突進時にプレイヤーを追従する処理
	FVector EnemyLoc = OwnerChar->GetActorLocation();
	FVector PlayerLoc = PlayerChar->GetActorLocation();
	
	FVector Direction = (PlayerLoc - EnemyLoc).GetSafeNormal2D();
	float Distance = FVector::Dist2D(EnemyLoc, PlayerLoc);
	
	// プレイヤーの方向へ回転
	if (!Direction.IsNearlyZero())
	{
		FRotator TargetRotation = Direction.Rotation();
		FRotator NewRotation = FMath::RInterpTo(OwnerChar->GetActorRotation(), TargetRotation, DeltaTime, RotationInterpolationSpeed);		// 回転補間ノード
		OwnerChar->SetActorRotation(NewRotation);
	}
	
	// 前方へ移動
	OwnerChar->AddMovementInput(OwnerChar->GetActorForwardVector(), 1.0f);
	
	// 指定範囲内に入ったら、もしくは経過時間が過ぎたら攻撃へ移行
	if (Distance <= ChargeAttackStartDistance || CurrentChargeTime >= ChargeDuration)
	{
		StartChargeAttack();
	}
}

void UEnemyChargeCombatComponent::StartChargeAttack()
{
	if (!OwnerChar) return;
	
	// 突進の経過時間を初期化
	CurrentChargeTime = 0.0f;
	
	// タイマーを解除する
	//GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);
	
	// 必要な時（移動/慣性を止める）
	//OwnerChar->GetCharacterMovement()->StopMovementImmediately();
	
	if  (ChargeAttackMontage)
	{
		OwnerChar->PlayAnimMontage(ChargeAttackMontage);
	}
}

void UEnemyChargeCombatComponent::WarpTarget()
{
	if (!OwnerChar || !PlayerChar) return;
	
	if (UMotionWarpingComponent* MotionWarpComp = OwnerChar->FindComponentByClass<UMotionWarpingComponent>())
	{
		FVector EnemyLoc = OwnerChar->GetActorLocation();
		FVector PlayerLoc = PlayerChar->GetActorLocation();
		
		// 敵からプレイヤーへの方向を計算
		FVector DirToPlayer = (PlayerLoc - EnemyLoc).GetSafeNormal2D();
		
		// 毎回追従する位置を更新する
		FMotionWarpingTarget Target;																				// 構造体の設定
		Target.Name = FName("AttackTarget");
		Target.Location = PlayerLoc;																				// 位置の計算
		Target.Rotation = DirToPlayer.IsNearlyZero() ? OwnerChar->GetActorRotation() : DirToPlayer.Rotation();		// 向きの計算
		Target.bFollowComponent = true;																				// コンポーネントの追従をonにする
			
		// 作った構造体を渡す
		MotionWarpComp->AddOrUpdateWarpTarget(Target);
	}
}

float UEnemyChargeCombatComponent::SelectAttackByDistance()
{
	if (!OwnerChar || !PlayerChar) return CloseAttackRange;
	
	float Distance = FVector::Dist2D(OwnerChar->GetActorLocation(), PlayerChar->GetActorLocation());
	
	if (Distance <= CloseRangeDistance)
	{
		bIsChargeAttack = false;		// 突進攻撃フラグをオフ
		bCanBeCountered = true;			// カウンター窓をオン
		bCanBeDodged = false;			// 回避窓をオフ
		return CloseAttackRange;
	}
	else
	{
		bIsChargeAttack = true;			// 突進攻撃フラグをオン
		bCanBeCountered = false;		// カウンター窓をオフ
		bCanBeDodged = true;			// 回避窓をオン
		return ChargeAttackRange;
	}
}
