#include "ArxSmoothMoveComponent.h"
#include "ArxGamePlayCommon.h"
#include "DrawDebugHelpers.h"


UArxSmoothMoveComponent::UArxSmoothMoveComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UArxSmoothMoveComponent::OnFrame(int Frame, FTransform Trans)
{
	Transforms.Add( Trans);
}


void UArxSmoothMoveComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	constexpr float Interval = (float)ArxConstants::TimeStep;


	DeltaTime *= FMath::Max(Transforms.Num(), 3) / 3;

	TotalTime += DeltaTime;

	if (Transforms.Num() == 0)
		return;


	while ( Transforms.Num() >= 2)
	{
		auto& T1 = Transforms[0];
		auto& T2 = Transforms[1];

		if (TotalTime < Interval)
		{
			auto Alpha = TotalTime / Interval;

			auto Pos = FMath::Lerp(T1.GetLocation(), T2.GetLocation(), Alpha);
			auto Rot = FMath::Lerp(T1.GetRotation(), T2.GetRotation(), Alpha);
			GetOwner()->SetActorLocationAndRotation(Pos, Rot.Rotator());
			//GetOwner()->SetActorLocation(Pos);
			return;
		}
		TotalTime -= Interval;
		Transforms.PopFront();
	}
	
	{
		GetOwner()->SetActorLocationAndRotation(Transforms[0].GetLocation(), Transforms[0].GetRotation().Rotator());
		//GetOwner()->SetActorLocation(Transforms[0].GetLocation());

		TotalTime = 0;
		return;
	}
}