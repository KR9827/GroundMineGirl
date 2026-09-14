// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/Frameworks/BTT_RequestAttackToken.h"

#include "Battle/Characters/BtlEnemyCharacter.h"
#include "Battle/Characters/EnemyAIController.h"

UBTT_RequestAttackToken::UBTT_RequestAttackToken()
{
	NodeName = TEXT("Request Attack Token");
}

EBTNodeResult::Type UBTT_RequestAttackToken::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* EAIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!EAIC) return EBTNodeResult::Failed;
	
	ABtlEnemyCharacter* Enemy = Cast<ABtlEnemyCharacter>(EAIC->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;
	
	UEnemyCombatManagerSubsystem* Manager = GetWorld()->GetSubsystem<UEnemyCombatManagerSubsystem>();
	if (!Manager) return EBTNodeResult::Failed;
	
	EEnemyType Type = Enemy->GetEnemyType();
	int32 MaxCount = Enemy->GetMaxAttackers();
	
	// 攻撃権を申請
	bool bGetToken = Manager->RequestAttackToken(Type, MaxCount);
	
	// 申請が通れば攻撃へ
	if (bGetToken)
	{
		EAIC->ChangeEnemyState(EEnemyState::Attacking);
		return EBTNodeResult::Succeeded;
	}
	
	return EBTNodeResult::Failed;
}
