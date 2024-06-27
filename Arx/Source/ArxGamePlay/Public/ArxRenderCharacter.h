#pragma once 

#include "CoreMinimal.h"
#include "ArxRenderActor.h"

#include "ArxRenderCharacter.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARXGAMEPLAY_API AArxRenderCharacter : public AArxRenderActor
{
    GENERATED_UCLASS_BODY()
public: 
    virtual void LinkEntity(ArxEntity* Entity) override;
    virtual void UnlinkEntity() override;
    virtual void Tick(float DeltaTime) override;

private:
    Rp3dTransform CachedTrans;
};