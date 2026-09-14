#include "Battle/Frameworks/BTT_Attack.h"
#include "AIController.h"
#include "Battle/Components/EnemyCombatComponent.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"

UBTT_Attack::UBTT_Attack()
{
	NodeName = "BTT_Attack";
}

EBTNodeResult::Type UBTT_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;
	
	ACharacter* Enemy = Cast<ACharacter>(AIController->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;
	
	// 敵のCombatComponentを取得
	UEnemyCombatComponent* CombatComp = Enemy->FindComponentByClass<UEnemyCombatComponent>();
    
	if (CombatComp)
	{
		// コンポーネント側の攻撃関数を呼ぶ
		CombatComp->ExecuteAttack();
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
