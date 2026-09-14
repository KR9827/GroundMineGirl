// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_RequestAttackToken.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTJ_API UBTT_RequestAttackToken : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTT_RequestAttackToken();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
