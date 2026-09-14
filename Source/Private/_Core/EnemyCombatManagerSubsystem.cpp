// Fill out your copyright notice in the Description page of Project Settings.


#include "_Core/EnemyCombatManagerSubsystem.h"

bool UEnemyCombatManagerSubsystem::RequestAttackToken(EEnemyType EnemyType, int32 MaxAttackers)
{
	// マップから現在の人数を取得(なければ0)
	int32& CurrentCount = CurrentAttackersMap.FindOrAdd(EnemyType, 0);		// &：コピーをつくらずにTMapの中に直接変更を加える
	if (CurrentCount < MaxAttackers)
	{
		CurrentCount++;
		return true;
	}	
	
	return false;
}

void UEnemyCombatManagerSubsystem::ReleaseAttackToken(EEnemyType EnemyType)
{
	if (int32* CurrentCount = CurrentAttackersMap.Find(EnemyType))				// Findの戻り値がポインタだから
	{
		*CurrentCount = FMath::Max(0, *CurrentCount - 1);
	}
}