#pragma once 

#include "CoreMinimal.h"

#include "Containers/RingBuffer.h"
#include "ArxSmoothMoveComponent.generated.h"

UCLASS()
class ARXGAMEPLAY_API UArxSmoothMoveComponent : public UActorComponent
{
    GENERATED_UCLASS_BODY()
public: 
    void OnFrame(int Frame, FTransform Trans);
    void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);

private:
    TRingBuffer<FTransform> Transforms;
    float TotalTime = 0;
};