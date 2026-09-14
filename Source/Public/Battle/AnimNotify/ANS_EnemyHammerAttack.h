// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_EnemyHammerAttack.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTJ_API UANS_EnemyHammerAttack : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	
	void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	
protected:
	// 攻撃場所の中心になるソケット名
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Shield")
	FName SocketName = FName("HammerHead");
};
