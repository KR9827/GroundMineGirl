// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/AnimNotify/AN_EnemyFireAttack.h"
#include "Battle/Components/EnemyRangedCombatComponent.h"

void UAN_EnemyFireAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);	
	
	if (MeshComp && MeshComp->GetOwner())
	{
		// 敵のコンポーネンをキャスト
		if (UEnemyRangedCombatComponent* EnemyRangedCombat = MeshComp->GetOwner()->FindComponentByClass<UEnemyRangedCombatComponent>())
		{
			EnemyRangedCombat->FireRangedAttack();
			
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Fire!!!"));
		}
	}
}
