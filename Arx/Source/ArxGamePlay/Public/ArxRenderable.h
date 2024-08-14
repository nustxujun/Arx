#pragma once 

#include "CoreMinimal.h"
#include "ArxGamePlayCommon.h"

#include "ArxRenderable.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class UArxRenderable : public UInterface
{
    GENERATED_BODY()


};

class ARXGAMEPLAY_API IArxRenderable 
{
    GENERATED_BODY()
public:
    virtual void LinkEntity(ArxEntityId EId, ArxWorld* World) ;
    virtual void UnlinkEntity() ;
    virtual AActor* GetActor();

    virtual void OnFrame_Async(int FrameId) = 0;

    ArxEntityId GetEntityId();
    ArxWorld& GetArxWorld(){return *World;}
protected:
    ArxEntityId EntityId = NON_PLAYER_CONTROL;
    ArxWorld* World = nullptr;
};