// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/AnimNotify/AN_EnemyDownEnd.h"
#include "Battle/Components/EnemyCombatComponent.h"

void UAN_EnemyDownEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (MeshComp && MeshComp->GetOwner())
	{
		// 敵のコンポーネンをキャスト
		if (UEnemyCombatComponent* EnemyCombat = MeshComp->GetOwner()->FindComponentByClass<UEnemyCombatComponent>())
		{
			// 状態をRecoveryに遷移
			EnemyCombat->HandleDownEnd();
		}
	}
}
