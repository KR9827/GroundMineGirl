// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/AnimNotify/ANS_JustDodgeWindow.h"
#include "Battle/Components/EnemyCombatComponent.h"

void UANS_JustDodgeWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (MeshComp && MeshComp->GetOwner())
	{
		if (UEnemyCombatComponent* CombatComp = MeshComp->GetOwner()->FindComponentByClass<UEnemyCombatComponent>())
		{
			CombatComp->SwitchJustDodgeWindow(true);
		}
	}
}

void UANS_JustDodgeWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (MeshComp && MeshComp->GetOwner())
	{
		if (UEnemyCombatComponent* CombatComp = MeshComp->GetOwner()->FindComponentByClass<UEnemyCombatComponent>())
		{
			CombatComp->SwitchJustDodgeWindow(false);
		}
	}
}
