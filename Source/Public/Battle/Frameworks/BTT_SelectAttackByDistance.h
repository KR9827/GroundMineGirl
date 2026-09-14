// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_SelectAttackByDistance.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTJ_API UBTT_SelectAttackByDistance : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTT_SelectAttackByDistance();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
