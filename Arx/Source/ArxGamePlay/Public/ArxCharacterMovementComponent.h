#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"


#include "ArxCharacterMovementComponent.generated.h"

class URp3dCapsuleComponent;

UCLASS()
class ARXGAMEPLAY_API UArxCharacterMovementComponent: public UPawnMovementComponent
{
    GENERATED_UCLASS_BODY()

public:
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    void SetCapsuleComponent(URp3dCapsuleComponent* Capsule);

private:
    URp3dCapsuleComponent* CapsuleComponent;
};