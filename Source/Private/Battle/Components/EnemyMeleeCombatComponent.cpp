// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/Components/EnemyMeleeCombatComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Battle/Interfaces/DamageableInterface.h"
#include "Battle/Interfaces/TargetInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Battle/Characters/EnemyAIController.h"

UEnemyMeleeCombatComponent::UEnemyMeleeCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyMeleeCombatComponent::ExecuteAttack()
{
	if (AttackFrontMontage.Num() == 0) return;
	
	// 攻撃中は何もしない
	if (bIsAttacking) return;
	
	int32 Index = FMath::RandRange(0, AttackFrontMontage.Num() - 1);
	UAnimMontage* SelectedMontage = AttackFrontMontage[Index];
 
	if (SelectedMontage)
	{
		bIsAttacking = true;  // 攻撃中フラグセット
 
		// モンタージュ再生のみ。
		OwnerChar->PlayAnimMontage(SelectedMontage);
	}
}

bool UEnemyMeleeCombatComponent::DoAttackTrace_Implementation(FName DamageSourceBone)
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

	ITargetInterface::Execute_CloseCounterWindow(PlayerChar->GetCombatComponent(),GetOwner());
	
	NotifyCounterWindowState(false);
	
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

bool UEnemyMeleeCombatComponent::HandleAttackMontageEnd(UAnimMontage* Montage)
{
	if (AttackFrontMontage.Contains(Montage))
	{
		bIsAttacking = false;
		
		// 攻撃終わったら強制的にカウンター窓を閉じる
		if (bIsCounterRange)
		{
			SwitchCounterWindow(false);
		}
		
		EAIC->ChangeEnemyState(EEnemyState::Recovery);
		return true;
	}
	return false;
}
