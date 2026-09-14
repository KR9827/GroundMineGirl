// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_EnemyDownEnd.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTJ_API UAN_EnemyDownEnd : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	// 指定のポイントに来たときに呼ばれる関数
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
