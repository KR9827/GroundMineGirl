// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/AnimNotify/ANS_RangedAimTracking.h"
#include "Battle/Components/EnemyRangedCombatComponent.h"

void UANS_RangedAimTracking::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (MeshComp && MeshComp->GetOwner())
	{
		// 敵のコンポーネンをキャスト
		if (UEnemyRangedCombatComponent* EnemyRangedCombat = MeshComp->GetOwner()->FindComponentByClass<UEnemyRangedCombatComponent>())
		{
			EnemyRangedCombat->StartAiming();
		}
	}
}

void UANS_RangedAimTracking::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	if (MeshComp && MeshComp->GetOwner())
	{
		// 敵のコンポーネンをキャスト
		if (UEnemyRangedCombatComponent* EnemyRangedCombat = MeshComp->GetOwner()->FindComponentByClass<UEnemyRangedCombatComponent>())
		{
			EnemyRangedCombat->UpdateAimLine(FrameDeltaTime);
		}
	}
}

void UANS_RangedAimTracking::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (MeshComp && MeshComp->GetOwner())
	{
		// 敵のコンポーネンをキャスト
		if (UEnemyRangedCombatComponent* EnemyRangedCombat = MeshComp->GetOwner()->FindComponentByClass<UEnemyRangedCombatComponent>())
		{
			EnemyRangedCombat->LockAim();
		}
	}
}
