// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/Components/EnemyRangedCombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Battle/Characters/EnemyAIController.h"
#include "Battle/Interfaces/DamageableInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Battle/Characters/BtlEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"

UEnemyRangedCombatComponent::UEnemyRangedCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;		// tick関係
}

void UEnemyRangedCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 攻撃をカウンター不可にする
	bCanBeCountered = false;
}

// Tickで毎フレーム更新して、プレイヤーへ赤線を描画する
void UEnemyRangedCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UEnemyRangedCombatComponent::ExecuteAttack()
{
	if (bIsAttacking || bIsAiming) return;
	if (!OwnerChar || !PlayerChar) return;
	
	// デバ文字
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("ExecuteAttackが呼ばれたよ"));
	
	bIsAttacking = true;
	bCanOpenDodgeWindow = true;			// 回避窓をオープンにする許可
	
	// 攻撃アニメーションの再生
	if (AttackMontage.Num() > 0)
	{
		int32 Index = FMath::RandRange(0, AttackMontage.Num() - 1);
		UAnimMontage* SelectedMontage = AttackMontage[Index];
		if (SelectedMontage)
		{
			OwnerChar->PlayAnimMontage(SelectedMontage);
		}
	}
}

void UEnemyRangedCombatComponent::UpdateAimLine(float DeltaTime)
{
	if (!OwnerChar || !PlayerChar) return;
	
	// プレイヤーの方向に向きを変える
	FVector MyLoc = OwnerChar->GetActorLocation();
	FVector TargetPos = (LockedTargetPos != FVector::ZeroVector) ? LockedTargetPos : PlayerChar->GetActorLocation();
	
	FVector TargetLoc = TargetPos;
	TargetLoc.Z = MyLoc.Z;
	
	// 角度のずれを修正
	FRotator TargetRot = UKismetMathLibrary::FindLookAtRotation(MyLoc, TargetLoc);
	TargetRot.Yaw += HandYawOffset;
	
	FRotator CurrentRot = OwnerChar->GetActorRotation();
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, AimRotationSpeed);
	OwnerChar->SetActorRotation(NewRot);
	
	// 始点からプレイヤーの胸元へ赤線を描画
	USkeletalMeshComponent* Mesh = OwnerChar->FindComponentByClass<USkeletalMeshComponent>();
	FVector StartPos = Mesh ? Mesh->GetSocketLocation(MuzzleSocketName) : MyLoc;
	
	FVector Direction = (TargetPos - StartPos).GetSafeNormal();
	FVector EndPos = StartPos + (Direction * AttackRange);
	
	TArray<AActor*> AllEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABtlEnemyCharacter::StaticClass(), AllEnemies);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(AllEnemies);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartPos,
		EndPos,
		ECC_Pawn,
		QueryParams
		);
	
	// ナイアガラに始点と終点を送信
	FVector ActualEndPos = bHit ? (HitResult.ImpactPoint + (Direction * AimLineExtensionDistance)) : EndPos;
	
	if (SpawnedLaserComp)
	{
		SpawnedLaserComp->SetVariablePosition(BeamStartParamName, StartPos);
		SpawnedLaserComp->SetVariablePosition(BeamEndParamName, ActualEndPos);
		
		// デバ表示
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("エイム追従中：ナイアガラ座標更新中"));
	}
}

void UEnemyRangedCombatComponent::FireRangedAttack()
{
	// エイム終了
	bIsAiming = false;
	
	// 予兆UIを消す
	StopLaserEffect();
	
	// ヒット判定の実行
	DoAttackTrace_Implementation(MuzzleSocketName);
}

void UEnemyRangedCombatComponent::SetAimLaserColor(const FLinearColor& Color)
{
	if (!SpawnedLaserComp)
	{
		return;
	}

	SpawnedLaserComp->SetVariableLinearColor(
		TEXT("User.EffectColor"),
		Color
	);
}

bool UEnemyRangedCombatComponent::DoAttackTrace_Implementation(FName DamageSourceBone)
{
	if (!OwnerChar || !PlayerChar) return false;

	USkeletalMeshComponent* Mesh = OwnerChar->FindComponentByClass<USkeletalMeshComponent>();
	FVector StartPos = Mesh ? Mesh->GetSocketLocation(DamageSourceBone) : OwnerChar->GetActorLocation();
	
	// 固定したプレイヤー位置に向かってレイを飛ばす
	FVector TargetPos = (LockedTargetPos != FVector::ZeroVector) ? LockedTargetPos : (PlayerChar ? PlayerChar->GetActorLocation() : StartPos);
	FVector ShootDir = (TargetPos - StartPos).GetSafeNormal();
	FVector EndPos = StartPos + (ShootDir * AttackRange);
	
	// ワールド内の全ての敵を取得
	TArray<AActor*> AllEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABtlEnemyCharacter::StaticClass(), AllEnemies);
	
	// その全ての敵を無視リストへ登録
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(AllEnemies);
	
	// トレースパラメータの設定
	FHitResult HitResult;
	// ラインTrace（即着型光線・弾丸判定）
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartPos,
		EndPos,
		ECC_Pawn,
		QueryParams
	);
	
	// 何もさえぎるものが無ければ最大射程まで
	FVector ActualEndPos = bHit ? (HitResult.ImpactPoint + (ShootDir * AimLineExtensionDistance)) : EndPos;

	if (bHit)
	{		
		AActor* HitActor = HitResult.GetActor();
		
		if (HitActor && HitActor == PlayerChar)
		{
			// インターフェース経由でのダメージ適用 
       		UActorComponent* Comp = HitActor->FindComponentByInterface(UDamageableInterface::StaticClass());		
       		if (IDamageableInterface* Damageable = Cast<IDamageableInterface>(Comp))
       		{
       			IDamageableInterface::Execute_ApplyDamage(
       				Comp,
       				RangedDamage,
       				GetOwner(),
       				EHitReactionType::Normal
       			);
       		}
		}		
	}
	
	// 射撃のナイアガラ
	if (FireLaserNiagaraSystem)
	{
		UNiagaraComponent* FireFXComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			FireLaserNiagaraSystem,
			StartPos
			);
		
		if (FireFXComp)
		{		
			// 始点と終点をナイアガラに送る
			FireFXComp->SetVariablePosition(BeamStartParamName, StartPos);
			FireFXComp->SetVariablePosition(BeamEndParamName, ActualEndPos);
			
			// デバ表示
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("ナイアガラ!!!"));
		}
	}
	
	return true;
}

bool UEnemyRangedCombatComponent::HandleAttackMontageEnd(UAnimMontage* Montage)
{
	if (AttackMontage.Contains(Montage))
	{
		bIsAttacking = false;
		bIsAiming = false;
		
		// 攻撃モーション終了時にもレーザーを消去
		StopLaserEffect();
		
		// 攻撃終わったら強制的に回避窓を閉じる
		if (bIsDodgeRange)
		{
			SwitchDodgeWindow(false);
		}
		
		EAIC->ChangeEnemyState(EEnemyState::Recovery);
		return true;
	}
	
	return false;
}

void UEnemyRangedCombatComponent::ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType)
{
	// 攻撃を受けたらエイムタイマーや予兆UIを中断
	CancelAiming();
	
	Super::ApplyDamage_Implementation(Damage, DamageCauser, HitReactionType);
}

void UEnemyRangedCombatComponent::StartAiming()
{
	bIsAiming = true;
	LockedTargetPos = FVector::ZeroVector;
	
	if (SpawnedLaserComp || SpawnedLensGlowComp || !OwnerChar) return;
	
	// レーザーの生成
	if (LaserNiagaraSystem)
	{
		if (USkeletalMeshComponent* Mesh = OwnerChar->FindComponentByClass<USkeletalMeshComponent>())
		{
			SpawnedLaserComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				LaserNiagaraSystem,
				Mesh,
				MuzzleSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true
				);
		}
	}
	
	// 携帯のレンズ
	if (LensGlowNiagaraSystem)
	{
		if (UStaticMeshComponent* PhoneMesh = OwnerChar->FindComponentByClass<UStaticMeshComponent>())
		{
			SpawnedLensGlowComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				LensGlowNiagaraSystem,
				PhoneMesh,
				TEXT("CameraLens"),
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true
				);
		}
	}
}

void UEnemyRangedCombatComponent::CancelAiming()
{
	if (bIsAiming)
	{
		// デバ文字
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("CancelAimingでエイムが解除されたよ！！！"));
		
		bIsAiming = false;
		LockedTargetPos = FVector::ZeroVector;
		
		// キャンセルのタイミングでレーザーを消す
		StopLaserEffect();
		
		if (bIsDodgeRange)
		{
			SwitchDodgeWindow(false);
		}
	}
}

void UEnemyRangedCombatComponent::StopLaserEffect()
{
	// デバ文字
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("StopLaserEffectがよばれたよ！！！"));
	
	if (SpawnedLaserComp)
	{
		SpawnedLaserComp->Deactivate();
		SpawnedLaserComp->DestroyComponent();
		SpawnedLaserComp = nullptr;
	}
	
	// レンズの光
	if (SpawnedLensGlowComp)
	{
		SpawnedLensGlowComp->Deactivate();
		SpawnedLensGlowComp->DestroyComponent();
		SpawnedLensGlowComp = nullptr;
	}
}

void UEnemyRangedCombatComponent::LockAim()
{
	if (!PlayerChar) return;
	
	// この関数が呼ばれた時点のプレイヤーの位置を保持
	LockedTargetPos = PlayerChar->GetActorLocation();
}
