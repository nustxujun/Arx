#pragma once 

#include "CoreMinimal.h"
#include "ArxGamePlayCommon.h"

#include "ArxRenderActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARXGAMEPLAY_API AArxRenderActor : public AActor
{
    GENERATED_BODY()
public:
    virtual void LinkEntity(ArxEntity* Ent) ;
    virtual void UnlinkEntity() ;

    ArxEntity* GetEntity();

protected:
    ArxWorld* World = 0;
    ArxEntityId EntityId = NON_PLAYER_CONTROL;
};