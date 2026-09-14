#include "Battle/Frameworks/BtlEnemyAICalculations.h"

FEnemyMoveResult UBtlEnemyAICalculations::GetCircleLocation(FVector TargetLocation, FVector CurrentLocation, FVector CurrentForward, float Radius, float AngleDegree, float MaxWalkSpeed)
{
	FEnemyMoveResult Result;
	
	// ======= 移動先の座標を計算 =======
	// ターゲットから自分へのベクトル
	FVector ToCurrent = CurrentLocation - TargetLocation;
	// 高低差を考えないようにする
	ToCurrent.Z = 0;	
	// ToCurrentを指定した角度だけ回転させる
	FVector RotatedVector = ToCurrent.RotateAngleAxis(AngleDegree, FVector(0, 0, 1));	// FVector(0, 0, 1)：Z軸を中心に回転させる
	// ベクトルの長さを指定の半径に調整する
	RotatedVector.Normalize();			// 正規化
	RotatedVector *= Radius;
	// Resultに反映
	FVector GoalLocation = TargetLocation + RotatedVector;
	Result.Location = GoalLocation;
	
	// ======= 移動方向の角度 =======
	// 現在地から目標地点へのベクトル
	FVector MoveVec = (GoalLocation - CurrentLocation).GetSafeNormal2D();
	// キャラクターの正面と移動方向の差を出す
	FVector Axis = FVector::CrossProduct(CurrentForward, MoveVec);
	float Dot = FMath::Clamp(FVector::DotProduct(CurrentForward, MoveVec), -1.0f, 1.0f);
	float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));
	Result.MoveAngle = (Axis.Z < 0) ? -Angle : Angle;
	
	// ======= 速度の計算 =======
	Result.Speed = MaxWalkSpeed;
	
	return Result;
}
