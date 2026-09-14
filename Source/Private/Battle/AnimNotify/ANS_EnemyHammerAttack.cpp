// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/AnimNotify/ANS_EnemyHammerAttack.h"
#include "Battle/Interfaces/AttackerInterface.h"
#include "Battle/Components/EnemyShieldCombatComponent.h"

void UANS_EnemyHammerAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (MeshComp && MeshComp->GetOwner())
	{
		if (UEnemyShieldCombatComponent* ShieldCombatComponent = MeshComp->GetOwner()->FindComponentByClass<UEnemyShieldCombatComponent>())
		{
			// 初期化する
			ShieldCombatComponent->ResetTraceLocation();
		}
	}
}

void UANS_EnemyHammerAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	if (MeshComp && MeshComp->GetOwner())
	{
		if (UEnemyShieldCombatComponent* ShieldCombatComponent = MeshComp->GetOwner()->FindComponentByClass<UEnemyShieldCombatComponent>())
		{
			// 攻撃
			IAttackerInterface::Execute_DoAttackTrace(ShieldCombatComponent, SocketName);
		}
	}
}