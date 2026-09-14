// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/AnimNotify/ANS_CheckChargeDistance.h"
#include "Battle/Components/EnemyChargeCombatComponent.h"

void UANS_CheckChargeDistance::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	if (MeshComp && MeshComp->GetOwner())
	{
		if (UEnemyChargeCombatComponent* ChargeCombatComponent = MeshComp->GetOwner()->FindComponentByClass<UEnemyChargeCombatComponent>())
		{
			ChargeCombatComponent->UpdateChargingPlayerToDistance(FrameDeltaTime);
		}
	}	
}
