// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/Frameworks/BTT_SelectAttackByDistance.h"
#include "Battle/Characters/BtlEnemyCharacter.h"
#include "Battle/Characters/EnemyAIController.h"
#include "Battle/Components/EnemyCombatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTT_SelectAttackByDistance::UBTT_SelectAttackByDistance()
{
	NodeName = TEXT("Select Attack By Distance");
}

EBTNodeResult::Type UBTT_SelectAttackByDistance::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// AIControllerを取得
	AEnemyAIController* EAIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!EAIC) return EBTNodeResult::Failed;
	
	// キャラの取得
	ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(EAIC->GetPawn());
	if (!EnemyChar) return EBTNodeResult::Failed;
	
	// CombatComponentの取得
	UEnemyCombatComponent* CombatComp = Cast<UEnemyCombatComponent>(EnemyChar->GetCombatComponent());
	if (!CombatComp) return EBTNodeResult::Failed;
	
	float SelectedRange = CombatComp->SelectAttackByDistance();
	
	// AIControllerにAttackRangeの変更を伝える
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsFloat(EAIC->GetAttackRangeKeyName(), SelectedRange);
		return EBTNodeResult::Succeeded;
	}
	
	return EBTNodeResult::Failed;
}
